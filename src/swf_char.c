#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swf.h"

struct swf_char *swf_char_new()
{
	struct swf_char *_char;

	_char = (struct swf_char*) malloc(sizeof(struct swf_char));

	if (_char) memset(_char, 0, sizeof(struct swf_char));

	return _char;
}
