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
#include <errno.h>

#include <neaacdec.h>

#include "unicode_support.h"
#include "audio.h"
#include "mp4read.h"

#include "marcus.h"


typedef struct
{
    FILE *file;
}
buffer_infile;


typedef struct
{
    FILE *file;
}
buffer_outfile;


static int rmf_next_step(
    Logger logger,
    cmdline_options *options,
    NeAACDecHandle hDecoder,
    buffer_infile *pInFile,
    buffer_outfile *pOutFile)
{
    int result = -1;
    return result;
}


static int rmf_open_outfile(
    Logger logger,
    cmdline_options *options,
    NeAACDecHandle hDecoder,
    buffer_infile *pInFile)
{
    int result = -1;
    buffer_outfile outfileBuffer;

    outfileBuffer.file = faad_fopen(options->output_filename, "wb");
    if (outfileBuffer.file != NULL)
    {
        result = rmf_next_step(logger, options, hDecoder, pInFile, &outfileBuffer);
        fclose(outfileBuffer.file);
    }
    else
    {
        logger(LOGGER_ERROR, "Could not open outfile: %d, %s\n", errno, strerror(errno));
    }

    return result;
}


static int rmf_open_infile(
    Logger logger,
    cmdline_options *options,
    NeAACDecHandle hDecoder)
{
    int result = -1;
    buffer_infile infileBuffer;

    infileBuffer.file = faad_fopen(options->input_filename, "rb");
    if (infileBuffer.file != NULL)
    {
        result = rmf_open_outfile(logger, options, hDecoder, &infileBuffer);
        fclose(infileBuffer.file);
    }
    else
    {
        logger(LOGGER_ERROR, "Could not open infile: %d, %s\n", errno, strerror(errno));
    }

    return result;
}


static void display_config(Logger logger, cmdline_options *options, NeAACDecConfigurationPtr config)
{
    logger(LOGGER_INFO, "Current configuration:\n");
    logger(LOGGER_INFO, "------------------------------\n");
    logger(LOGGER_INFO, "         input filename: %s\n", options->input_filename);
    logger(LOGGER_INFO, "        output filename: %s\n", options->output_filename);
    logger(LOGGER_INFO, "------------------------------\n");
    logger(LOGGER_INFO, "          defObjectType: %d\n", config->defObjectType);
    logger(LOGGER_INFO, "          defSampleRate: %d\n", config->defSampleRate);
    logger(LOGGER_INFO, "           outputFormat: %d\n", config->outputFormat);
    logger(LOGGER_INFO, "            downMatrix): %d\n", config->downMatrix);
    logger(LOGGER_INFO, "       useOldADTSFormat: %d\n", config->useOldADTSFormat);
    logger(LOGGER_INFO, "dontUpSampleImplicitSBR: %d\n", config->dontUpSampleImplicitSBR);
    logger(LOGGER_INFO, "------------------------------\n");
}


static void cmdline_to_decconfig(cmdline_options *options, NeAACDecConfigurationPtr config)
{
    config->defObjectType = options->object_type;
    config->defSampleRate = options->samplerate;
    config->outputFormat = options->output_format;
}


int rescue_media_file(Logger logger, cmdline_options *options)
{
    int result = 0;

    NeAACDecHandle hDecoder = NeAACDecOpen();
    NeAACDecConfigurationPtr config = NeAACDecGetCurrentConfiguration(hDecoder);
    cmdline_to_decconfig(options, config);
    display_config(logger, options, config);
    NeAACDecSetConfiguration(hDecoder, config);

    result = rmf_open_infile(logger, options, hDecoder);

    NeAACDecClose(hDecoder);
    return result;
}
