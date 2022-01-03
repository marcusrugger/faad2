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



int rescue_media_file(Logger logger, cmdline_options *options)
{
    return 0;
}
