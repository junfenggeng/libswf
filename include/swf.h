#ifndef _SWF_H_
#define _SWF_H_

#include "swf_types.h"
#include "swf_reader.h"

int swf_open(struct swf_movie *movie, const char *filename);
int swf_print_info(struct swf_movie *movie);
int swf_advance(struct swf_movie *movie);
int swf_render(struct swf_movie *movie);
int swf_end(struct swf_movie *movie);

struct swf_chardef *swf_find_chardef_with_id(struct swf_movie *movie,
					     uint16_t id);
int swf_place_object(struct swf_movie *movie,
		     uint16_t id,
		     struct swf_char *_char);
int swf_draw_char(struct swf_movie *movie, struct swf_char *_char);
int swf_draw_shape(struct swf_movie *movie,
		   struct swf_char *_char,
		   struct swf_reader *reader,
		   uint8_t ver);

struct swf_chardef *swf_chardef_new();
struct swf_char *swf_char_new();

#endif //_SWF_H_
