#ifndef _SWF_READER_H_
#define _SWF_READER_H_

#include "swf_types.h"

static inline void swf_align(struct swf_reader* reader)
{
	if (reader->bitpos) {
		reader->bitpos = 0;
		reader->pos++;
	}
}

static inline uint32_t swf_read_ub(struct swf_reader *reader, uint8_t nbits)
{
	uint8_t *data = reader->data;
	uint32_t value;
	uint8_t bits_needed = nbits;
	uint8_t bits_used = reader->bitpos;

	value = 0;

	do
	{
		uint32_t mask_remain = 0xFF >> bits_used;
		uint32_t bits_remain = 8 - bits_used;
		uint8_t byte = data[reader->pos];

		if (bits_needed >= bits_remain)
		{
			bits_needed -= bits_remain;

			value |= ((byte&mask_remain) << bits_needed);
			reader->pos++; bits_used = 0;
		}
		else
		{
			bits_remain -= bits_needed;

			value |= ((byte&mask_remain) >> bits_remain);

			bits_used += bits_needed;

			break;
		}
	}
	while (bits_needed > 0);

	reader->bitpos = bits_used;

	return value;
}

static inline int32_t swf_read_sb(struct swf_reader *reader, uint8_t nbits)
{
	int32_t value = (int32_t) swf_read_ub(reader, nbits);

	if (value & (1 << (nbits - 1))) value |= -1 << nbits;

	return value;
}

static inline uint16_t swf_read_u8(struct swf_reader* reader)
{
	uint8_t *data = reader->data;
	uint8_t value;

	swf_align(reader);
	value = data[reader->pos++];

	return value;
}

static inline uint16_t swf_read_u16(struct swf_reader* reader)
{
	uint8_t *data = reader->data;
	uint16_t value;
	swf_align(reader);

	value = data[reader->pos++];
	value += (data[reader->pos++] << 8);

	return value;
}

static inline uint16_t swf_read_u32(struct swf_reader* reader)
{
	uint8_t *data = reader->data;
	uint32_t value;

	swf_align(reader);
	value = data[reader->pos++];
	value += (data[reader->pos++] << 8);
	value += (data[reader->pos++] << 16);
	value += (data[reader->pos++] << 24);

	return value;
}

static inline int swf_read_rect(struct swf_reader* reader,
				 struct swf_rect* rect)
{
	uint8_t nbits;

	nbits = swf_read_ub(reader, 5);
	rect->xmin = swf_read_sb(reader, nbits);
	rect->xmax = swf_read_sb(reader, nbits);
	rect->ymin = swf_read_sb(reader, nbits);
	rect->ymax = swf_read_sb(reader, nbits);
}

static inline int swf_read_cxform(struct swf_reader* reader,
				   struct swf_cxform* cxform,
				   uint8_t has_alpha)
{
	uint8_t has_mult, has_add;
	uint8_t nbits;

	has_mult = swf_read_ub(reader, 1);
	has_add = swf_read_ub(reader, 1);
	nbits = swf_read_ub(reader, 4);

	cxform->rmt = swf_read_sb(reader, nbits);
	cxform->gmt = swf_read_sb(reader, nbits);
	cxform->bmt = swf_read_sb(reader, nbits);
	if (has_alpha)
		cxform->amt = swf_read_sb(reader, nbits);
	else
		cxform->amt = 1;
	cxform->rat = swf_read_sb(reader, nbits);
	cxform->gat = swf_read_sb(reader, nbits);
	cxform->bat = swf_read_sb(reader, nbits);
	if (has_alpha)
		cxform->aat = swf_read_sb(reader, nbits);
	else
		cxform->aat = 0;
}

static inline int swf_read_matrix(struct swf_reader* reader,
				   struct swf_matrix* mtx)
{
	uint8_t has_scale, has_rotate;
	uint8_t nscalebits, nrotatebits, ntransbits;

	has_scale = swf_read_ub(reader, 1);

	if (has_scale) {
		nscalebits = swf_read_ub(reader, 5);
		mtx->sx = swf_read_ub(reader, nscalebits);
		mtx->sy = swf_read_ub(reader, nscalebits);
	} else {
		mtx->sx = mtx->sy = 0;
	}

	has_rotate = swf_read_ub(reader, 1);

	if (has_rotate) {
		nrotatebits = swf_read_ub(reader, 5);
		mtx->rs0 = swf_read_ub(reader, nrotatebits);
		mtx->rs1 = swf_read_ub(reader, nrotatebits);
	} else {
		mtx->rs0 = mtx->rs1 = 0;
	}

	ntransbits = swf_read_ub(reader, 5);
	mtx->tx = swf_read_sb(reader, ntransbits);
	mtx->ty = swf_read_sb(reader, ntransbits);
}

static inline int swf_read_rgb(struct swf_reader* reader,
				    struct swf_color* color)
{
	color->r = swf_read_u8(reader);
	color->g = swf_read_u8(reader);
	color->b = swf_read_u8(reader);
	color->a = 255;
}


static inline int swf_read_rgba(struct swf_reader* reader,
				    struct swf_color* color)
{
	color->r = swf_read_u8(reader);
	color->g = swf_read_u8(reader);
	color->b = swf_read_u8(reader);
	color->a = swf_read_u8(reader);
}

static inline int swf_read_gradrec(struct swf_reader* reader,
				    struct swf_gradrec* rec,
				    uint8_t ver)
{
	rec->ratio = swf_read_u8(reader);
	if (ver == 3) {
		swf_read_rgba(reader, &rec->color);
	} else {
		swf_read_rgb(reader, &rec->color);
	}
}

static inline int swf_read_gradient(struct swf_reader* reader,
				     struct swf_gradient* grad,
				     uint8_t ver)
{
	uint8_t i;

	grad->spread_mode = swf_read_ub(reader, 2);
	grad->interp_mode = swf_read_ub(reader, 2);
	grad->ngrads = swf_read_ub(reader, 4);
	
	for (i=0; i<grad->ngrads; i++) {
		swf_read_gradrec(reader, grad->recs + i, ver);
	}
}

static inline int swf_read_taghdr(struct swf_reader* reader,
				   struct swf_taghdr* hdr)
{
	uint16_t shorthdr;
	uint8_t len;

	shorthdr = swf_read_u16(reader);
	hdr->code = shorthdr >> 6;
	len = shorthdr & 0x3F;

	if (len == 0x3F) {
		hdr->length = swf_read_u32(reader);
	} else {
		hdr->length = len;
	}
}

#endif // _SWF_READER_H_
