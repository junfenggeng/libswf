#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swf.h"

struct swf_chardef *swf_chardef_new()
{
	struct swf_chardef *cd;

	cd = (struct swf_chardef*) malloc(sizeof(struct swf_chardef));

	if (cd) memset(cd, 0, sizeof(struct swf_chardef));

	return cd;
}
