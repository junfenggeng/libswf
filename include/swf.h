#ifndef _SWF_H_
#define _SWF_H_

#include "swf_types.h"
#include "swf_reader.h"

int swf_movie_load_from_file(struct swf_movie *movie, const char *filename);
int swf_movie_print_info(struct swf_movie *movie);
int swf_movie_advance(struct swf_movie *movie);
int swf_movie_render(struct swf_movie *movie);
int swf_movie_end(struct swf_movie *movie);
struct swf_chardef *swf_movie_find_chardef_with_id(struct swf_movie *movie,
				   uint16_t id);

struct swf_clip *swf_clip_new_with_movie(struct swf_movie *movie);
int swf_clip_advance(struct swf_clip *clip);
int swf_clip_runaction(struct swf_clip *clip);
int swf_clip_advance(struct swf_clip *clip);
int swf_clip_render(struct swf_clip *clip);
int swf_clip_end(struct swf_clip *clip);
int swf_clip_place_object(struct swf_clip *clip,
			  uint16_t charid,
			  struct swf_char *_char);
int swf_clip_draw_char(struct swf_clip *clip, struct swf_char *_char);
int swf_clip_draw_shape(struct swf_clip *clip,
			struct swf_char *_char,
			struct swf_reader *reader,
			uint8_t ver);

struct swf_chardef *swf_chardef_new();
struct swf_char *swf_char_new();

#endif //_SWF_H_
