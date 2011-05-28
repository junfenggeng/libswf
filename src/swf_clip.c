#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swf.h"

struct swf_clip *swf_clip_new_with_movie(struct swf_movie *movie)
{
	struct swf_clip *clip;

	clip = (struct swf_clip*) malloc(sizeof(struct swf_clip));

	if (clip == NULL) goto out;

	memset(clip, 0, sizeof(struct swf_clip));
	clip->movie = movie;

out:
	return clip;
}

int swf_clip_runctrl(struct swf_clip *clip)
{
	struct swf_taghdr hdr;
	struct swf_reader reader;
	uint32_t tagpos_next;

	reader.data = clip->movie->data;
	reader.length = clip->movie->length;

	while(clip->frame_next > clip->frame) {
		reader.bitpos = 0;
		reader.pos = clip->pos_curr;

		swf_read_taghdr(&reader, &hdr);
		tagpos_next = reader.pos + hdr.length;

		printf("clip.runctrl reading tag %d\n", hdr.code);
		switch (hdr.code) {
		case 0: // end
			clip->end = 1;
		case 1: // showFrame
			clip->frame++;
			break;
		case 4: // placeObject
			break;
		case 26: // placeObject2
		{
			uint8_t flags;
			struct swf_char *ch;
			uint16_t charid;

			ch = swf_char_new();

			flags = swf_read_u8(&reader);

			ch->depth = swf_read_u16(&reader);
			if (flags & 2) charid = swf_read_u16(&reader);
			else charid = 0;
			if (flags & 4) swf_read_matrix(&reader, &ch->mtx);
			else swf_init_matrix(&ch->mtx);
			if (flags & 8) swf_read_cxform(&reader,
						       &ch->cxform, 0);
			else swf_init_cxform(&ch->cxform);
			if (flags & 0x10) ch->ratio = swf_read_u16(&reader);
			else ch->ratio = 0;

			swf_clip_place_object(clip, charid, ch);
		}
			break;
		case 5: // removeObject
			break;
		case 28: // removeObject2
			break;
		}

		reader.pos = clip->pos_curr = tagpos_next;
	}

}

int swf_clip_runaction(struct swf_clip *clip)
{
}

int swf_clip_advance(struct swf_clip *clip)
{
	clip->frame_next = clip->frame+1;
	swf_clip_runctrl(clip);
	swf_clip_runaction(clip);
}

int swf_clip_render(struct swf_clip *clip)
{
	struct swf_char *ch;

	ch = clip->displaylist;

	while (ch) {
		printf("ch: %d\n", ch->depth);

		swf_clip_draw_char(clip, ch);

		ch = ch->next;
	}
}

int swf_clip_end(struct swf_clip *clip)
{
	return 1;
	return clip->end;
}

int swf_clip_place_object(struct swf_clip *clip,
			  uint16_t id,
			  struct swf_char *_char)
{
	struct swf_chardef *def;

	printf("place %d@%d\n", id, _char->depth);

	def = swf_movie_find_chardef_with_id(clip->movie, id);
	_char->pos = def->pos;

	_char->next = clip->displaylist;
	clip->displaylist = _char;

	return 0;
}

int swf_clip_draw_char(struct swf_clip *clip, struct swf_char *_char)
{
	struct swf_reader reader;
	struct swf_taghdr hdr;

	reader.data = clip->movie->data;
	reader.length = clip->movie->length;
	reader.pos = _char->pos;
	reader.bitpos = 0;

	printf("draw char pos: %d\n", reader.pos);

	swf_read_taghdr(&reader, &hdr);
	switch (hdr.code) {
	case 22: // defineShape2
		swf_clip_draw_shape(clip, _char, &reader, 2);
		break;
	}

	return 0;
}

static inline int read_fillstyle(struct swf_reader* reader, uint8_t ver)
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

static inline int read_fillstylearray(struct swf_reader* reader, uint8_t ver)
{
	uint16_t i;
	uint16_t cnt;
 
	cnt = swf_read_u8(reader);
	if (cnt == 0xFF)
		cnt = swf_read_u16(reader);

	printf("fillstylearray cnt=%d\n", cnt);
	for (i=0; i<cnt; i++) {
		read_fillstyle(reader, ver);
	}
}

static inline int read_linestyle(struct swf_reader* reader, uint8_t ver)
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

static inline int read_linestylearray(struct swf_reader* reader, uint8_t ver)
{
	uint16_t i;
	uint16_t cnt;

	cnt = swf_read_u8(reader);
	if (cnt == 0xFF)
		cnt = swf_read_u16(reader);

	printf("linestylearray cnt=%d\n", cnt);
	for (i=0; i<cnt; i++) {
		read_linestyle(reader, ver);
	}
}

static inline int read_line(struct swf_reader* reader)
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

	printf("line %d,%d\n", dx, dy);
}

static inline int read_curve(struct swf_reader* reader)
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

int swf_clip_draw_shape(struct swf_clip *clip,
			struct swf_char *_char,
			struct swf_reader *reader,
			uint8_t ver)
{
	uint16_t id;
	struct swf_rect bounds;
	uint8_t nfillbits, nlinebits;
	uint8_t end = 0;
	uint16_t fs0, fs1, ls;
	int32_t dx, dy;

	id = swf_read_u16(reader);
	swf_read_rect(reader, &bounds);

	printf("draw shape id: %d, bounds: %d,%d,%d,%d\n",
	       id,
	       bounds.xmin,
	       bounds.xmax,
	       bounds.ymin,
	       bounds.ymax);

	read_fillstylearray(reader, ver);
	read_linestylearray(reader, ver);
	nfillbits = swf_read_ub(reader, 4);
	nlinebits = swf_read_ub(reader, 4);

	while (!end) {
		uint8_t is_edge = swf_read_ub(reader, 1);

		if (is_edge) {
			uint8_t is_line = swf_read_ub(reader, 1);
			if (is_line) read_line(reader);
			else read_curve(reader);
		} else {
			uint8_t flags;
			flags = swf_read_ub(reader, 5);
			if (flags == 0) end = 1;
			else {
				uint8_t move_bits;
				uint16_t fs0, fs1, ls;
				int16_t dx, dy;

				if (flags & 1) { // stateMoveTo
					move_bits = swf_read_ub(reader, 5);
					dx = swf_read_sb(reader, move_bits);
					dy = swf_read_sb(reader, move_bits);
					printf("move %d,%d\n",
					       dx, dy);
				}
				if (flags & 2) { // fillStyle0
					fs0 = swf_read_ub(reader, nfillbits);
					printf("style.left %d\n", fs0);
				}
				if (flags & 4) { // fillStyle1
					fs1 = swf_read_ub(reader, nfillbits);
					printf("style.right %d\n", fs1);
				}
				if (flags & 8) { // lineStyle
					ls = swf_read_ub(reader, nlinebits);
					printf("style.line %d\n", ls);
				}
				if (flags & 0x10) { // newStyles
					read_fillstylearray(reader, ver);
					read_linestylearray(reader, ver);
					nfillbits = swf_read_ub(reader, 4);
					nlinebits = swf_read_ub(reader, 4);
				}
			}
		}
	}
}
