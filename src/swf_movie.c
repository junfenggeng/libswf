#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include "swf.h"

int swf_movie_load_from_file(struct swf_movie *movie, const char *filename)
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

	swf_movie_load(movie);

out: if (fp) fclose(fp);
	return status;
}

int swf_movie_load(struct swf_movie *movie)
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

	movie->rootchar.clip = &movie->rootclip;
	movie->rootclip.movie = movie;
	movie->rootclip.me = &movie->rootchar;
	movie->rootclip.pos_begin = reader.pos;
	movie->rootclip.pos_curr = reader.pos;

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
			swf_movie_add_chardef(movie, cd);
		}
			break;
		}

		reader.pos = tagpos_next;
		reader.bitpos = 0;
	}
}

int swf_movie_print_info(struct swf_movie *movie)
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

int swf_movie_advance(struct swf_movie* movie)
{
	return swf_clip_advance(&movie->rootclip);
}

int swf_movie_render(struct swf_movie* movie)
{
	return swf_clip_render(&movie->rootclip);
}

int swf_movie_end(struct swf_movie* movie)
{
	return swf_clip_end(&movie->rootclip);
}

int swf_movie_add_chardef(struct swf_movie *movie,
			  struct swf_chardef *cd)
{
	cd->next = movie->dict;
	movie->dict = cd;

	return 0;
}

struct swf_chardef *swf_movie_find_chardef_with_id(struct swf_movie *movie,
				   uint16_t id)
{
	struct swf_chardef *cd;

	cd = movie->dict;

	while (cd) {
		if (cd->id == id) break;
		cd = cd->next;
	}

	return cd;
}
