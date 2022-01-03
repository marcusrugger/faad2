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


static const int MAX_CHANNELS = 16;


typedef struct
{
    FILE *file;
    unsigned char *buffer;
    int buffer_size;
}
buffer_infile;

static void initialize_buffer_infile(buffer_infile *pinfile)
{
    pinfile->file = NULL;
    pinfile->buffer = NULL;
    pinfile->buffer_size = 0;
}


typedef struct
{
    FILE *file;
}
buffer_outfile;

static void initialize_buffer_outfile(buffer_outfile *poutfile)
{
    poutfile->file = NULL;
}


static int rmf_decode_aac(
    Logger logger,
    cmdline_options *options,
    NeAACDecHandle hDecoder,
    buffer_infile *pinfile,
    buffer_outfile *poutfile)
{
    int result = -1;

    for (int i = 0; i < 1; i++)
    {

    }

    return result;
}


static int rmf_open_outfile(
    Logger logger,
    cmdline_options *options,
    NeAACDecHandle hDecoder,
    buffer_infile *pinfile)
{
    int result = -1;

    buffer_outfile outfileBuffer;
    initialize_buffer_outfile(&outfileBuffer);

    outfileBuffer.file = faad_fopen(options->output_filename, "wb");
    if (outfileBuffer.file != NULL)
    {
        result = rmf_decode_aac(logger, options, hDecoder, pinfile, &outfileBuffer);
        fclose(outfileBuffer.file);
    }
    else
    {
        logger(LOGGER_ERROR, "Could not open outfile: %d, %s\n", errno, strerror(errno));
    }

    return result;
}


static int rmf_malloc_infile_buffer(
    Logger logger,
    cmdline_options *options,
    NeAACDecHandle hDecoder,
    buffer_infile *pinfile)
{
    int result = -1;

    pinfile->buffer_size = FAAD_MIN_STREAMSIZE * MAX_CHANNELS;
    pinfile->buffer = (unsigned char*)malloc(pinfile->buffer_size);
    if (pinfile->buffer != NULL)
    {
        result = rmf_open_outfile(logger, options, hDecoder, pinfile);
        free(pinfile->buffer);
    }
    else
    {
        logger(LOGGER_ERROR, "Could not instantiate infile buffer: %d, %s\n", errno, strerror(errno));
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
    initialize_buffer_infile(&infileBuffer);

    infileBuffer.file = faad_fopen(options->input_filename, "rb");
    if (infileBuffer.file != NULL)
    {
        result = rmf_malloc_infile_buffer(logger, options, hDecoder, &infileBuffer);
        fclose(infileBuffer.file);
    }
    else
    {
        logger(LOGGER_ERROR, "Could not open infile: %d, %s\n", errno, strerror(errno));
    }

    return result;
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
    NeAACDecSetConfiguration(hDecoder, config);

    result = rmf_open_infile(logger, options, hDecoder);

    NeAACDecClose(hDecoder);
    return result;
}
