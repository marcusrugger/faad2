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


#ifndef min
#define min(a,b) ( (a) < (b) ? (a) : (b) )
#endif


static const int MAX_CHANNELS = 16;
static const int FALSE = 0;
static const int TRUE = !FALSE;


typedef struct
{
    FILE *file;
    unsigned char *buffer;
    int buffer_size;

    int at_eof;
    int bytes_consumed;
    int bytes_left_in_buffer;
    int file_offset;
}
buffer_infile;

static void initialize_buffer_infile(buffer_infile *pinfile)
{
    pinfile->file = NULL;
    pinfile->buffer = NULL;
    pinfile->buffer_size = 0;
    pinfile->at_eof = FALSE;
    pinfile->bytes_consumed = 0;
    pinfile->file_offset = 0;
}

static int initial_fill_inbuffer(buffer_infile *pinfile)
{
    int bread = fread(pinfile->buffer, 1, pinfile->buffer_size, pinfile->file);
    pinfile->bytes_left_in_buffer = bread;
    return bread;
}

static void fill_inbuffer(buffer_infile *pinfile)
{
    if (pinfile->bytes_consumed > 0)
    {
        if (pinfile->bytes_left_in_buffer)
        {
            memmove(
                pinfile->buffer,
                pinfile->buffer + pinfile->bytes_consumed,
                pinfile->bytes_left_in_buffer * sizeof(unsigned char));
        }

        if (!pinfile->at_eof)
        {
            int bread = fread(
                pinfile->buffer + pinfile->bytes_left_in_buffer,
                1,
                pinfile->bytes_consumed,
                pinfile->file);

            pinfile->at_eof = (bread != pinfile->bytes_consumed);

            pinfile->bytes_left_in_buffer += bread;
        }

        pinfile->bytes_consumed = 0;
    }
}

static void advance_inbuffer(buffer_infile *pinfile, int bytes)
{
    while ((pinfile->bytes_left_in_buffer > 0) && (bytes > 0))
    {
        int chunk = min(bytes, pinfile->bytes_left_in_buffer);

        bytes -= chunk;
        pinfile->file_offset += chunk;
        pinfile->bytes_consumed += chunk;
        pinfile->bytes_left_in_buffer -= chunk;

        if (pinfile->bytes_left_in_buffer == 0)
            fill_inbuffer(pinfile);
    }
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
    output_audio_file *poutfile)
{
    NeAACDecFrameInfo frameinfo;
    int a = 0;

    int bread = initial_fill_inbuffer(pinfile);

    do
    {
        void *sample_buffer = NeAACDecDecode(hDecoder, &frameinfo, pinfile->buffer, pinfile->buffer_size);
        logger(LOGGER_INFO, "Object count: %d; Frame info: channels = %d, bytes consumed = %d\n", ++a, frameinfo.channels, frameinfo.bytesconsumed);
        if (frameinfo.channels == 0) return -1;

        poutfile->write(poutfile, sample_buffer, frameinfo.samples, frameinfo.channels);

        advance_inbuffer(pinfile, frameinfo.bytesconsumed);
        fill_inbuffer(pinfile);
    }
    while (TRUE);

    return 0;
}


static int rmf_initialize_aac_decoder(
    Logger logger,
    cmdline_options *options,
    NeAACDecHandle hDecoder,
    buffer_infile *pinfile,
    output_audio_file *poutfile)
{
    int result = -1;
    unsigned long samplerate;
    unsigned char channels;

    int bread = NeAACDecInit(hDecoder, pinfile->buffer, pinfile->buffer_size, &samplerate, &channels);
    if (bread >= 0)
    {
        logger(LOGGER_INFO, "NeAACDecInit: samplerate = %ld, channels = %d\n", samplerate, channels);
        result = rmf_decode_aac(logger, options, hDecoder, pinfile, poutfile);
    }
    else
    {
        logger(LOGGER_ERROR, "Could not initialize aac decoder.\n");
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

    output_audio_file *poutfile = create_audio_wav_file(logger, options->channels);
    if (poutfile == NULL) return -1;

    result = poutfile->open(poutfile, options->output_filename);
    if (SUCCESSFUL(result))
    {
        poutfile->writeheader(poutfile);
        result = rmf_initialize_aac_decoder(logger, options, hDecoder, pinfile, poutfile);
        poutfile->close(poutfile);
    }

    release_audio_wav_file(poutfile);
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
        memset(pinfile->buffer, 0, pinfile->buffer_size);
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
