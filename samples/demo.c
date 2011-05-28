#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include "swf.h"

int main(int argc, char** argv)
{
	int status;
	struct swf_movie movie;
	char* filename;

	filename = "test/sample.swf";

	status = swf_movie_load_from_file(&movie, filename);
	if (status != 0) {
		printf("failed to load swf movie from [%s]\n", filename);
		goto out;
	}

	swf_movie_print_info(&movie);

	swf_movie_advance(&movie);
	swf_movie_render(&movie);

out:
	return status;
}
