#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#ifndef __MINGW32__
#define off_t __int64
#endif
#else
#include <time.h>
#endif

#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <neaacdec.h>

#include "unicode_support.h"
#include "audio.h"
#include "mp4read.h"

#include "marcus.h"

// #ifdef HAVE_GETOPT_H
// # include <getopt.h>
// #else
// # include "getopt.h"
// # include "getopt.c"
// #endif



static int rescue_media_file(Logger logger, cmdline_options *options)
{
    return 0;
}



static char *LoggerLevelToString(LOGGER_LEVEL level)
{
    switch (level)
    {
        case LOGGER_DISPLAY: return "DISPLAY";
        case LOGGER_ERROR:   return "ERROR";
        case LOGGER_WARNING: return "WARNING";
        case LOGGER_INFO:    return "INFO";
        case LOGGER_DEBUG:   return "DEBUG";
        default: return "UNKNOWN";
    }
}


static LOGGER_LEVEL mylog_level = LOGGER_DEBUG;
void set_logger_level(LOGGER_LEVEL level)
{
    mylog_level = level;
}

static void mylog(LOGGER_LEVEL level, char *format, ...)
{
    if (level > mylog_level) return;

    va_list valist;
    va_start(valist, format);
    fprintf(stderr, "%7s: ", LoggerLevelToString(level));
    vfprintf(stderr, format, valist);
    va_end(valist);
}


int mymain(int argc, char *argv[])
{
    cmdline_options *options = initialize_cmdline_options(mylog, argc, argv);
    if (options == NULL) return -1;

    int result = rescue_media_file(mylog, options);

    return result;
}
