#pragma once

enum {
	LOG_WARNING, LOG_ERROR
};

void log_msg(
	unsigned type,
	const char *filename,
	unsigned line,
	const char *format,
	...);

void log_errno(const char *filename, unsigned line, int e);
