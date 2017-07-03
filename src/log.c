#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "log.h"
//-------------------------------------------------------------------

void log_msg(
	unsigned type,
	const char *filename,
	unsigned line,
	const char *format,
	...)
{
	const char* head = "[WW] %s:%u: ";

	va_list va;
	va_start(va, format);

	if (type == LOG_ERROR)
		head = "\e[0;31m[EE] %s:%u: \e[0m";

	fprintf(stderr, head, filename, line);
	vfprintf(stderr, format, va);

	va_end(va);
}

void log_errno(const char *filename, unsigned line, int e) {
	log_msg(LOG_ERROR, filename, line, "%s\n", strerror(e));
}

//-------------------------------------------------------------------
