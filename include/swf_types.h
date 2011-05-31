#ifndef _SWF_TYPES_H_
#define _SWF_TYPES_H_

#include <stdint.h>
#include <cairo.h>

struct swf_clip;
struct swf_char;
struct swf_movie;

struct swf_rect {
	uint32_t xmin;
	uint32_t xmax;
	uint32_t ymin;
	uint32_t ymax;
};

struct swf_color {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

struct swf_matrix {
	uint32_t sx, sy;
	uint32_t rs0, rs1;
	int32_t tx, ty;
};

static inline void swf_init_matrix(struct swf_matrix *m)
{
	m->sx = m->sy = 1;
	m->rs0 = m->rs1 = 0;
	m->tx = m->ty = 0;
}

struct swf_cxform {
	int16_t rmt, gmt, bmt, amt;
	int16_t rat, gat, bat, aat;
};

static inline void swf_init_cxform(struct swf_cxform *c)
{
	c->rmt = c->gmt = c->bmt = c->amt = 1;
	c->rat = c->gat = c->bat = c->aat = 0;
}

struct swf_linestyle {
	uint16_t width;
	struct swf_color color;
};

struct swf_gradrec {
	uint8_t ratio;
	struct swf_color color;
};

struct swf_gradient {
	uint8_t spread_mode;
	uint8_t interp_mode;
	uint8_t ngrads;
	struct swf_gradrec recs[16];
};

struct swf_fillstyle {
	uint8_t type;
	struct swf_matrix mtx;
	union {
		struct swf_color color;
		struct swf_gradient grad;
		uint16_t bitmapid;
	} u;
};

struct swf_taghdr {
	uint16_t code;
	uint32_t length;
};

struct swf_reader {
	uint8_t *data;
	uint32_t length;
	uint32_t pos;
	uint32_t bitpos;
};

struct swf_graphics {
	cairo_t *cr;

	uint8_t nfillstyles, nlinestyles;
	struct swf_fillstyle *fillstyle;
	struct swf_linestyle *linestyle;

	uint8_t nfillbits, nlinebits;
	uint16_t fs0, fs1, ls;

	uint8_t open:1;
};

struct swf_chardef {
	struct swf_chardef* next;

	uint16_t id;
	uint32_t pos;
};

struct swf_char {
	struct swf_char *next;
	struct swf_clip *clip;
	uint32_t pos;
	uint16_t depth;
	uint16_t ratio;
	struct swf_matrix mtx;
	struct swf_cxform cxform;
};

struct swf_movie {
	uint8_t* data;
	uint32_t length;

	struct swf_chardef* dict;
	struct swf_char *displaylist;

	// timeline
	uint8_t paused:1;
	uint8_t end:1;
	uint16_t frame;
	uint16_t frame_next;
	uint32_t pos;

	uint8_t version;
	struct swf_rect frame_size;
	uint16_t frame_rate; // fixed 8.8
	uint16_t frame_cnt;
};

#endif //_SWF_TYPES_H_
