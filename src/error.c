#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "error.h"
//-------------------------------------------------------------------

void error() {
	fprintf(stderr, "[EE] %s", strerror(errno));
}

//-------------------------------------------------------------------
