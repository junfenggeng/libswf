#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swf.h"

struct swf_char *swf_char_new()
{
	struct swf_char *ch;

	ch = (struct swf_char*) malloc(sizeof(struct swf_char));

	if (ch) memset(ch, 0, sizeof(struct swf_char));

	return ch;
}
