#ifndef _SWF_PARSER_H_
#define _SWF_PARSER_H_

#include <stdio.h>
#include <stdint.h>

struct swf_parser_context {
	uint8_t* data;
	uint32_t length;
	uint32_t pos;
	uint8_t bitpos;
};

static inline int swf_parse_gradrec(struct swf_clip* clip,
				    struct swf_gradrec* rec,
				    uint8_t ver)
{
	rec->ratio = swf_read_u8(clip);
	if (ver == 3) {
		swf_parse_rgba(clip, &rec->color);
	} else {
		swf_parse_rgb(clip, &rec->color);
	}
}

static inline int swf_parse_gradient(struct swf_clip* clip,
				     struct swf_gradient* grad,
				     uint8_t ver)
{
	uint8_t i;

	grad->spread_mode = swf_read_ub(clip, 2);
	grad->interp_mode = swf_read_ub(clip, 2);
	grad->ngrads = swf_read_ub(clip, 4);
	
	for (i=0; i<grad->ngrads; i++) {
		swf_parse_gradrec(clip, grad->recs + i, ver);
	}
}

static inline int swf_parse_header(struct swf_clip* clip,
				   struct swf_header* hdr)
{
	swf_parse_rect(clip, &hdr->frm_size);
	hdr->frm_rate = swf_read_u16(clip);
	hdr->frm_cnt = swf_read_u16(clip);
}


static inline int swf_parse_fs(struct swf_clip* clip, uint8_t ver)
{
	uint8_t type;
	struct swf_fillstyle fs;

	type = swf_read_u8(clip);

	switch (type & 0xF0) {
	case 0: // solid color
		if (ver == 3) swf_parse_rgba(clip, &fs.u.color);
		else swf_parse_rgb(clip, &fs.u.color);
		printf("fillstyle: solid color %d,%d,%d,%d\n",
		       fs.u.color.r,
		       fs.u.color.g,
		       fs.u.color.b,
		       fs.u.color.a);
		break;
	case 0x10: // gradient
		swf_parse_matrix(clip, &fs.mtx);
		swf_parse_gradient(clip, &fs.u.grad, ver);
		break;
	case 0x40: // bitmap
		fs.u.bitmapid = swf_read_u16(clip);
		swf_parse_matrix(clip, &fs.mtx);
		break;
	default:
		printf("fillstyle %d not supported\n");
	}

}

static inline int swf_parse_fsarray(struct swf_clip* clip, uint8_t ver)
{
	uint16_t i;
	uint16_t fscnt;

	fscnt = swf_read_u8(clip);
	if (fscnt == 0xFF)
		fscnt = swf_read_u16(clip);

	printf("fillstylearray cnt=%d\n", fscnt);
	for (i=0; i<fscnt; i++) {
		swf_parse_fs(clip, ver);
	}
}

static inline int swf_parse_ls(struct swf_clip* clip, uint8_t ver)
{
	uint16_t width;
	struct swf_color color;

	width = swf_read_u16(clip);

	if (ver == 3)
		swf_parse_rgba(clip, &color);
	else
		swf_parse_rgb(clip, &color);

	printf("linestyle %d %d,%d,%d,%d\n",
	       width,
	       color.r, color.g, color.b, color.a);
}

static inline int swf_parse_lsarray(struct swf_clip* clip, uint8_t ver)
{
	uint16_t i;
	uint16_t lscnt;

	lscnt = swf_read_u8(clip);
	if (lscnt == 0xFF)
		lscnt = swf_read_u16(clip);

	printf("linestylearray cnt=%d\n", lscnt);
	for (i=0; i<lscnt; i++) {
		swf_parse_ls(clip, ver);
	}
}

static inline int swf_parse_line(struct swf_clip* clip)
{
	uint8_t nbits, general, vert = 0;
	int16_t dx, dy;

	nbits = swf_read_ub(clip, 4);
	general = swf_read_ub(clip, 1);
	if (general == 0)
		vert = swf_read_ub(clip, 1);

	dx = dy = 0;
	if (general == 1 || vert == 0)
		dx = swf_read_sb(clip, nbits + 2);
	if (general == 1 || vert == 1)
		dy = swf_read_sb(clip, nbits + 2);

	printf("line %d,%d\n", dx, dy);
}

static inline int swf_parse_curve(struct swf_clip* clip)
{
	uint8_t nbits;
	uint16_t cx, cy, ax, ay;

	nbits = swf_read_ub(clip, 4);
	cx = swf_read_sb(clip, nbits + 2);
	cy = swf_read_sb(clip, nbits + 2);
	ax = swf_read_sb(clip, nbits + 2);
	ay = swf_read_sb(clip, nbits + 2);

	printf("curve %d,%d,%d,%d\n",
	       cx, cy, ax, ay);
}

static inline int swf_parse_shapewithstyle(struct swf_clip* clip,
					   uint8_t ver)
{
	uint8_t nfillbits, nlinebits;
	uint8_t type;
	uint8_t end = 0;
	uint16_t fs0, fs1, ls;
	int32_t dx, dy;

	swf_parse_fsarray(clip, ver);
	swf_parse_lsarray(clip, ver);
	nfillbits = swf_read_ub(clip, 4);
	nlinebits = swf_read_ub(clip, 4);

	while (!end) {
		uint8_t is_edge = swf_read_ub(clip, 1);

		if (is_edge) {
			uint8_t is_line = swf_read_ub(clip, 1);
			if (is_line) swf_parse_line(clip);
			else swf_parse_curve(clip);
		} else {
			uint8_t flags;
			flags = swf_read_ub(clip, 5);
			if (flags == 0) end = 1;
			else {
				uint8_t move_bits;
				uint16_t fs0, fs1, ls;
				int16_t dx, dy;

				if (flags & 1) { // stateMoveTo
					move_bits = swf_read_ub(clip, 5);
					dx = swf_read_sb(clip, move_bits);
					dy = swf_read_sb(clip, move_bits);
					printf("move %d,%d\n",
					       dx, dy);
				}
				if (flags & 2) { // fillStyle0
					fs0 = swf_read_ub(clip, nfillbits);
					printf("lfill %d\n", fs0);
				}
				if (flags & 4) { // fillStyle1
					fs1 = swf_read_ub(clip, nfillbits);
					printf("rfill %d\n", fs1);
				}
				if (flags & 8) { // lineStyle
					ls = swf_read_ub(clip, nlinebits);
					printf("linestyle %d\n", ls);
				}
				if (flags & 0x10) { // newStyles
					swf_parse_fsarray(clip, ver);
					swf_parse_lsarray(clip, ver);
					nfillbits = swf_read_ub(clip, 4);
					nlinebits = swf_read_ub(clip, 4);
				}
			}
		}
	}
}

static inline int swf_parse_defineshape2(struct swf_clip* clip)
{
	uint16_t id;
	struct swf_rect bounds;

	id = swf_read_u16(clip);
	swf_parse_rect(clip, &bounds);

	printf("shape id:%d, bounds:%d,%d,%d,%d\n",
	       id,
	       bounds.xmin, bounds.xmax,
	       bounds.ymin, bounds.ymax);

	swf_parse_shapewithstyle(clip, 2);
}

static inline int swf_parse_placeobject2(struct swf_clip* clip)
{
	uint8_t flags;
	uint16_t charid = 0, ratio = 0, depth;
	struct swf_matrix mtx;
	struct swf_cxform cxform;

	flags = swf_read_u8(clip);

	depth = swf_read_u16(clip);
	if (flags & 2) charid = swf_read_u16(clip);
	if (flags & 4) swf_parse_matrix(clip, &mtx);
	if (flags & 8) swf_parse_cxform(clip, &cxform, 0);
	if (flags & 0x10) ratio = swf_read_u16(clip);

	printf("PO %d@%d %x\n", charid, depth, flags);
	printf("\tmtx %d,%d,%d,%d,%d,%d\n",
	       mtx.sx, mtx.sy,
	       mtx.rs0, mtx.rs1,
	       mtx.tx, mtx.ty);
}

static inline int swf_parse_tag(struct swf_clip* clip,
				struct swf_taghdr* hdr)
{
	switch (hdr->code) {
	case 4: // placeObject
		printf("placeObject\n");
		break;
	case 26: // placeObject2
		printf("placeObject2\n");
		swf_parse_placeobject2(clip);
		break;
	case 5: // removeObject
		printf("removeObject\n");
		break;
	case 28: // removeObject2
		printf("removeObject2\n");
		break;
	case 1: // showFrame
		printf("showFrame\n");
		break;
	case 9: // setBackgroundColor
		printf("setBackgroundColor\n");
		break;
	case 0: // end
		printf("end\n");
		break;
	case 2: // defineShape
		printf("defineShape\n");
		break;
	case 22: // defineShape2
		printf("defineShape2\n");
		swf_parse_defineshape2(clip);
		break;
	default:
		printf("ignored tag %d\n", hdr->code);
	}
}

static inline int swf_parse_body(struct swf_clip* clip)
{
	struct swf_taghdr hdr;

	while (clip->pos < clip->length) {
		uint32_t curpos;

		swf_parse_taghdr(clip, &hdr);

		curpos = clip->pos;

		swf_parse_tag(clip, &hdr);

		clip->pos = curpos + hdr.length;
		clip->bitpos = 0;
	}
}

#endif //_SWF_PARSER_H_
