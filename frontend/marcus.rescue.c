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


static void cmdline_to_decconfig(cmdline_options *options, NeAACDecConfigurationPtr config)
{
    if (options->samplerate > 0) config->defSampleRate = options->samplerate;
}


int rescue_media_file(Logger logger, cmdline_options *options)
{
    NeAACDecHandle hDecoder = NeAACDecOpen();
    NeAACDecConfigurationPtr config = NeAACDecGetCurrentConfiguration(hDecoder);
    NeAACDecFrameInfo frameInfo;

    cmdline_to_decconfig(options, config);

    logger(LOGGER_INFO, "Default sample rate: %d\n", config->defSampleRate);

    return 0;
}
