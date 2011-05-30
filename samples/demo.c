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

	status = swf_open(&movie, filename);
	if (status != 0) {
		printf("failed to load swf movie from [%s]\n", filename);
		goto out;
	}

	swf_print_info(&movie);

	swf_advance(&movie);
	swf_render(&movie);

out:
	return status;
}
