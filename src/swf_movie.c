#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include "swf.h"

int swf_open(struct swf_movie *movie, const char *filename)
{
	int status = 0;
	FILE* fp;
	struct stat filestat;
	size_t filesize;
	uint8_t hdr[8];

	memset(movie, 0, sizeof (struct swf_movie));

	fp = fopen(filename, "rb");
	if (fp == NULL) {
		printf("cannot open [%s]\n", filename);
		status = -1;
		goto out;
	}

	stat(filename, &filestat);
	filesize = filestat.st_size;

	if (fread(hdr, 8, 1, fp) != 1) {
		status = -1; goto out;
	}

	if (hdr[1] != 'W' || hdr[2] != 'S') {
		status = -1; goto out;
	}

	movie->version = hdr[3];
	movie->length = (hdr[4] | (hdr[5]<<8) |
			 (hdr[6]<<16) | (hdr[7]<<24)) - 8;

	switch (hdr[0]) {
	case 'C':
		//todo decompress
		printf("decompress not impl\n");
		status = -1; goto out;
		break;
	case 'F':
		if ((movie->data = (uint8_t*) malloc(movie->length)) == 0) {
			status = -1; goto out;
		}

		if (fread(movie->data, movie->length, 1, fp) != 1) {
			status = -1; goto out;
		}
		break;
	default:
		status = -1;
		goto out;
	}

	swf_load(movie);

out: if (fp) fclose(fp);
	return status;
}

int swf_load(struct swf_movie *movie)
{
	struct swf_taghdr hdr;
	struct swf_reader reader;
	uint32_t tagpos, tagpos_next;
	uint8_t end = 0;

	reader.data = movie->data;
	reader.length = movie->length;
	reader.pos = reader.bitpos = 0;

	swf_read_rect(&reader, &movie->frame_size);
	movie->frame_rate = swf_read_u16(&reader);
	movie->frame_cnt = swf_read_u16(&reader);

	movie->pos = reader.pos;

	while (!end) {
		tagpos = reader.pos;
		swf_read_taghdr(&reader, &hdr);
		tagpos_next = reader.pos + hdr.length;

		printf("movie.load reading tag %d\n", hdr.code);
		switch (hdr.code) {
		case 0: // end
			end = 1;
			break;
		case 9: // setBackgroundColor
			break;
		case 2: // defineShape
		case 22: // defineShape2
		{
			struct swf_chardef *cd;

			cd = swf_chardef_new();
			cd->id = swf_read_u16(&reader);
			cd->pos = tagpos;
			printf("defineShape pos: %d\n", tagpos);
			swf_add_chardef(movie, cd);
		}
			break;
		}

		reader.pos = tagpos_next;
		reader.bitpos = 0;
	}
}

int swf_print_info(struct swf_movie *movie)
{
	printf("Movie Info\n"
	       "\tversion: %d\n"
	       "\tframe_size: <%d,%d,%d,%d>\n"
	       "\tframe_rate: %d.%d\n"
	       "\tframe_count: %d\n"
	       "\ttotal_data_length: %d\n",
	       movie->version,
	       movie->frame_size.xmin,
	       movie->frame_size.xmax,
	       movie->frame_size.ymin,
	       movie->frame_size.ymax,
	       movie->frame_rate >> 8,
	       movie->frame_rate & 0xFF,
	       movie->frame_cnt,
	       movie->length);
}

int swf_advance(struct swf_movie* movie)
{
	struct swf_taghdr hdr;
	struct swf_reader reader;
	uint32_t tagpos_next;

	reader.data = movie->data;
	reader.length = movie->length;
	reader.bitpos = 0;
	reader.pos = movie->pos;

	printf("movie.pos: %d\n", movie->pos);

	movie->frame_next = movie->frame + 1;
	while(movie->frame_next > movie->frame) {
		swf_read_taghdr(&reader, &hdr);
		tagpos_next = reader.pos + hdr.length;

		printf("movie.advance reading tag %d\n", hdr.code);
		switch (hdr.code) {
		case 0: // end
			movie->end = 1;
		case 1: // showFrame
			movie->frame++;
			break;
		case 4: // placeObject
			break;
		case 26: // placeObject2
		{
			uint8_t flags;
			struct swf_char *_char;
			uint16_t id;

			_char = swf_char_new();

			flags = swf_read_u8(&reader);

			_char->depth = swf_read_u16(&reader);
			if (flags & 2) id = swf_read_u16(&reader);
			else id = 0;
			if (flags & 4) swf_read_matrix(&reader, &_char->mtx);
			else swf_init_matrix(&_char->mtx);
			if (flags & 8) swf_read_cxform(&reader,
						       &_char->cxform, 0);
			else swf_init_cxform(&_char->cxform);
			if (flags & 0x10) _char->ratio = swf_read_u16(&reader);
			else _char->ratio = 0;

			swf_place_object(movie, id, _char);
		}
			break;
		case 5: // removeObject
			break;
		case 28: // removeObject2
			break;
		}

		reader.pos = tagpos_next;
		reader.bitpos = 0;
	}

	movie->pos = reader.pos;
}

int swf_render(struct swf_movie* movie, struct swf_graphics *gc)
{
	struct swf_char *_char;

	_char = movie->displaylist;

	while (_char) {
		printf("char: %d\n", _char->depth);

		swf_draw_char(movie, gc, _char);

		_char = _char->next;
	}
}

int swf_end(struct swf_movie* movie)
{
	return 1;
}

int swf_add_chardef(struct swf_movie *movie,
			  struct swf_chardef *def)
{
	def->next = movie->dict;
	movie->dict = def;

	return 0;
}

struct swf_chardef *swf_find_chardef_with_id(struct swf_movie *movie,
				   uint16_t id)
{
	struct swf_chardef *def;

	def = movie->dict;

	while (def) {
		if (def->id == id) break;
		def = def->next;
	}

	return def;
}

int swf_place_object(struct swf_movie *movie,
			  uint16_t id,
			  struct swf_char *_char)
{
	struct swf_chardef *def;

	printf("place %d@%d\n", id, _char->depth);

	def = swf_find_chardef_with_id(movie, id);
	_char->pos = def->pos;

	_char->next = movie->displaylist;
	movie->displaylist = _char;

	return 0;
}

int swf_draw_char(struct swf_movie *movie,
		  struct swf_graphics *gc,
		  struct swf_char *_char)
{
	struct swf_reader reader;
	struct swf_taghdr hdr;

	reader.data = movie->data;
	reader.length = movie->length;
	reader.pos = _char->pos;
	reader.bitpos = 0;

	swf_read_taghdr(&reader, &hdr);
	switch (hdr.code) {
	case 22: // defineShape2
		swf_draw_shape(movie, gc, _char, &reader, 2);
		break;
	}

	return 0;
}

static inline int read_fillstyle(struct swf_reader* reader,
				 struct swf_graphics *gc,
				 uint8_t ver)
{
	uint8_t type;
	struct swf_fillstyle fs;

	type = swf_read_u8(reader);

	switch (type & 0xF0) {
	case 0: // solid color
		if (ver == 3) swf_read_rgba(reader, &fs.u.color);
		else swf_read_rgb(reader, &fs.u.color);
		printf("fillstyle: solid color %d,%d,%d,%d\n",
		       fs.u.color.r,
		       fs.u.color.g,
		       fs.u.color.b,
		       fs.u.color.a);
		break;
	case 0x10: // gradient
		swf_read_matrix(reader, &fs.mtx);
		swf_read_gradient(reader, &fs.u.grad, ver);
		break;
	case 0x40: // bitmap
		fs.u.bitmapid = swf_read_u16(reader);
		swf_read_matrix(reader, &fs.mtx);
		break;
	default:
		printf("fillstyle %d not supported\n");
	}

}

static inline int read_fillstylearray(struct swf_reader* reader,
				      struct swf_graphics *gc, 
				      uint8_t ver)
{
	uint16_t i;
	uint16_t cnt;
 
	cnt = swf_read_u8(reader);
	if (cnt == 0xFF)
		cnt = swf_read_u16(reader);

	printf("fillstylearray cnt=%d\n", cnt);
	for (i=0; i<cnt; i++) {
		read_fillstyle(reader, gc, ver);
	}
}

static inline int read_linestyle(struct swf_reader* reader,
				 struct swf_graphics *gc,
				 uint8_t ver)
{
	uint16_t width;
	struct swf_color color;

	width = swf_read_u16(reader);

	if (ver == 3)
		swf_read_rgba(reader, &color);
	else
		swf_read_rgb(reader, &color);

	printf("linestyle %d %d,%d,%d,%d\n",
	       width,
	       color.r, color.g, color.b, color.a);
}

static inline int read_linestylearray(struct swf_reader* reader,
				      struct swf_graphics *gc,
				      uint8_t ver)
{
	uint16_t i;
	uint16_t cnt;

	cnt = swf_read_u8(reader);
	if (cnt == 0xFF)
		cnt = swf_read_u16(reader);

	printf("linestylearray cnt=%d\n", cnt);
	for (i=0; i<cnt; i++) {
		read_linestyle(reader, gc, ver);
	}
}

static inline int read_shapesetup(struct swf_reader* reader,
				  struct swf_graphics *gc,
				  uint8_t flags,
				  uint8_t ver)
{
	int32_t dx, dy;
	uint8_t move_bits;

	if (gc->open) {
		cairo_stroke(gc->cr);
	}

	gc->open = 1;

	if (flags & 1) { // stateMoveTo
		move_bits = swf_read_ub(reader, 5);
		dx = swf_read_sb(reader, move_bits);
		dy = swf_read_sb(reader, move_bits);
		printf("move %d,%d\n",
		       dx, dy);

		cairo_move_to (gc->cr, dx, dy);
	}
	if (flags & 2) { // fillStyle0
		gc->fs0 = swf_read_ub(reader, gc->nfillbits);
		printf("fs0 %d\n", gc->fs0);
	}
	if (flags & 4) { // fillStyle1
		gc->fs1 = swf_read_ub(reader, gc->nfillbits);
		printf("fs1 %d\n", gc->fs1);
	}
	if (flags & 8) { // lineStyle
		gc->ls = swf_read_ub(reader, gc->nlinebits);
		printf("ls %d\n", gc->ls);
	}
	if (flags & 0x10) { // newStyles
		read_fillstylearray(reader, gc, ver);
		read_linestylearray(reader, gc, ver);
		gc->nfillbits = swf_read_ub(reader, 4);
		gc->nlinebits = swf_read_ub(reader, 4);
	}
}

static inline int read_line(struct swf_reader* reader,
			    struct swf_graphics *gc)
{
	uint8_t nbits, general, vert = 0;
	int16_t dx, dy;

	nbits = swf_read_ub(reader, 4);
	general = swf_read_ub(reader, 1);
	if (general == 0)
		vert = swf_read_ub(reader, 1);

	dx = dy = 0;
	if (general == 1 || vert == 0)
		dx = swf_read_sb(reader, nbits + 2);
	if (general == 1 || vert == 1)
		dy = swf_read_sb(reader, nbits + 2);

	cairo_rel_line_to(gc->cr, dx, dy);
	printf("line %d,%d\n", dx, dy);
}

static inline int read_curve(struct swf_reader *reader,
			     struct swf_graphics *gc)
{
	uint8_t nbits;
	uint16_t cx, cy, ax, ay;

	nbits = swf_read_ub(reader, 4);
	cx = swf_read_sb(reader, nbits + 2);
	cy = swf_read_sb(reader, nbits + 2);
	ax = swf_read_sb(reader, nbits + 2);
	ay = swf_read_sb(reader, nbits + 2);

	printf("curve %d,%d,%d,%d\n",
	       cx, cy, ax, ay);
}

int swf_draw_shape(struct swf_movie *movie,
		   struct swf_graphics *gc,
		   struct swf_char *_char,
		   struct swf_reader *reader,
		   uint8_t ver)
{
	uint16_t id;
	struct swf_rect bounds;
	uint8_t end = 0;
	int32_t dx, dy;

	id = swf_read_u16(reader);
	swf_read_rect(reader, &bounds);

	printf("draw shape id: %d, bounds: %d,%d,%d,%d\n",
	       id,
	       bounds.xmin,
	       bounds.xmax,
	       bounds.ymin,
	       bounds.ymax);

	read_fillstylearray(reader, gc, ver);
	read_linestylearray(reader, gc, ver);
	gc->nfillbits = swf_read_ub(reader, 4);
	gc->nlinebits = swf_read_ub(reader, 4);

	while (!end) {
		uint8_t is_edge = swf_read_ub(reader, 1);

		if (is_edge) {
			uint8_t is_line = swf_read_ub(reader, 1);
			if (is_line) read_line(reader, gc);
			else read_curve(reader, gc);
		} else {
			uint8_t flags;
			flags = swf_read_ub(reader, 5);
			if (flags == 0) end = 1;
			else {
				read_shapesetup(reader, gc, flags, ver);
			}
		}
	}

	if (gc->open) {
		cairo_stroke(gc->cr);
	}
}
