#include <stdio.h>
#include <stdarg.h>

#include "log.h"

#define LOGFILENAME	"log.txt"

static FILE * logfile;

int LogInit()
{
	//logfile = fopen(LOGFILENAME, "w");
	return logfile ? 0 : 1;
}

void LogDestroy()
{
	if (logfile)
		fclose(logfile);
}

int LogPrintf(const char *format, ...) // C99
{
	int ret;
	va_list args;
	if (!logfile) return 0;
	va_start(args, format);
	ret = vfprintf(logfile, format, args);
	va_end(args);
	return ret;
}
