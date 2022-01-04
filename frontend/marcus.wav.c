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
#include <math.h>

#include <neaacdec.h>

#include "unicode_support.h"
#include "audio.h"
#include "mp4read.h"

#include "marcus.h"


#define CHANNEL_COUNT 16
#define SAMPLES_PER_FRAME 1024
#define BITS_PER_SAMPLE 16
#define SAMPLE_FORMAT short


typedef struct audio_wav_file_tag audio_wav_file;
struct audio_wav_file_tag
{
    Logger logger;
    FILE *file;
    int channels;
    int sample_rate;
    int bits_per_sample;
    int current_pair_count;
    int total_samples;
    int header_written;

    SAMPLE_FORMAT buffer[SAMPLES_PER_FRAME][CHANNEL_COUNT];

    int (*writer)(audio_wav_file *, unsigned char *, int, int);
};


static int wav_file_write_multichannel(audio_wav_file *wavfile, unsigned char *inbuffer, int sample_count, int channel_count)
{
    SAMPLE_FORMAT *sample_buffer = (SAMPLE_FORMAT *)inbuffer;
    SAMPLE_FORMAT *output_buffer = (SAMPLE_FORMAT *)wavfile->buffer + 2 * wavfile->current_pair_count;

    for (int sindex = sample_count / channel_count; sindex > 0; --sindex)
    {
        *output_buffer++ = *sample_buffer++;
        *output_buffer++ = *sample_buffer++;
        output_buffer += (wavfile->channels - 2);
    }

    wavfile->current_pair_count++;
    if (wavfile->current_pair_count < wavfile->channels / 2) return 0;

    wavfile->current_pair_count = 0;
    int count = wavfile->channels * SAMPLES_PER_FRAME;
    int written = fwrite(wavfile->buffer, sizeof(SAMPLE_FORMAT), count, wavfile->file);
    if (count != written)
    {
        wavfile->logger(LOGGER_ERROR, "wav_file_write_multichannel_16: error writing wav file: %d, %s\n", errno, strerror(errno));
        return -1;
    }

    return 0;
}


static int wav_file_write(output_audio_file *audiofile, unsigned char *samples, int sample_count, int channel_count)
{
    audio_wav_file *wavfile = (audiofile != NULL) ? (audio_wav_file *)audiofile->data : NULL;
    if (wavfile == NULL || wavfile->file == NULL)
    {
        wavfile->logger(LOGGER_ERROR, "wav_file_write: can not write samples, wav file is not open.\n");
        return -1;
    }

    int result = wavfile->writer(wavfile, samples, sample_count, channel_count);
    wavfile->total_samples += sample_count;
    return result;
}


typedef struct
{
    /* RIFF chunk */
    int8_t riff[4];
    int32_t filesize;
    int8_t wave[4];

    /* fmt chunk */
    int8_t fmt[4];
    int32_t format_length;
    int16_t format;
    int16_t channels;
    int32_t sample_rate;
    int32_t byte_rate;
    int16_t block_align;
    int16_t bits_per_sample;

    /* data chunk */
    int8_t data[4];
    int32_t data_size;
}
audio_wav_file_header;


static void wav_file_initialize_header(audio_wav_file *wavfile, audio_wav_file_header *header)
{
    header->riff[0] = 'R';
    header->riff[1] = 'I';
    header->riff[2] = 'F';
    header->riff[3] = 'F';

    header->filesize = 0x7FFFFFFF;

    header->wave[0] = 'W';
    header->wave[1] = 'A';
    header->wave[2] = 'V';
    header->wave[3] = 'E';

    header->fmt[0] = 'f';
    header->fmt[1] = 'm';
    header->fmt[2] = 't';
    header->fmt[3] = ' ';

    header->format_length = 16;
    header->format = 1;
    header->channels = wavfile->channels;
    header->sample_rate = wavfile->sample_rate;
    header->byte_rate = (wavfile->sample_rate * wavfile->channels * wavfile->bits_per_sample) / 8;
    header->block_align = (wavfile->bits_per_sample * wavfile->channels) / 8;
    header->bits_per_sample = wavfile->bits_per_sample;

    header->data[0] = 'd';
    header->data[1] = 'a';
    header->data[2] = 't';
    header->data[3] = 'a';

    header->data_size = 0x7FFFFFFF;
}


static int wav_file_write_header(output_audio_file *audiofile)
{
    audio_wav_file *wavfile = (audiofile != NULL) ? (audio_wav_file *)audiofile->data : NULL;
    if (wavfile == NULL || wavfile->file == NULL)
    {
        wavfile->logger(LOGGER_ERROR, "wav_file_write_header: can not write header, wav file is not open.\n");
        return -1;
    }

    audio_wav_file_header header;
    wav_file_initialize_header(wavfile, &header);

    size_t written = fwrite(&header, 1, sizeof(header), wavfile->file);
    if (written != sizeof(header))
    {
        wavfile->logger(LOGGER_ERROR, "wav_file_write_header: write failed, unable to write header: %d, %s\n", errno, strerror(errno));
        return -1;
    }

    wavfile->header_written = 1;

    return 0;
}


static void wav_file_rewrite_header(audio_wav_file *wavfile)
{
    if (!wavfile->header_written) return;

    audio_wav_file_header header;
    wav_file_initialize_header(wavfile, &header);

    header.data_size = wavfile->total_samples * sizeof(SAMPLE_FORMAT);
    header.filesize = 4 + (8 + header.format_length) + (8 + header.data_size);

    int result = fseek(wavfile->file, 0, SEEK_SET);
    if (result != 0)
    {
        wavfile->logger(LOGGER_ERROR, "wav_file_rewrite_header: fseek failed, unable to rewrite header: %d, %s\n", errno, strerror(errno));
        return;
    }

    size_t written = fwrite(&header, 1, sizeof(header), wavfile->file);
    if (written != sizeof(header))
    {
        wavfile->logger(LOGGER_ERROR, "wav_file_rewrite_header: write failed, unable to rewrite header: %d, %s\n", errno, strerror(errno));
        return;
    }
}


static void wav_file_close(output_audio_file *audiofile)
{
    audio_wav_file *wavfile = (audiofile != NULL) ? (audio_wav_file *)audiofile->data : NULL;
    if (wavfile == NULL || wavfile->file == NULL) return;

    wav_file_rewrite_header(wavfile);
    fclose(wavfile->file);
    wavfile->file = NULL;
}


static int wav_file_open(output_audio_file *audiofile, char *filename)
{
    audio_wav_file *wavfile = (audiofile != NULL) ? (audio_wav_file *)audiofile->data : NULL;
    if (wavfile == NULL) return -1;

    if (filename == NULL)
    {
        wavfile->logger(LOGGER_ERROR, "wav_file_open: filename may not be null\n");
        return -1;
    }

    wavfile->file = fopen(filename, "wb");
    if (wavfile->file == NULL)
    {
        wavfile->logger(LOGGER_ERROR, "wav_file_open: could not open output wav file: %d, %s\n", errno, strerror(errno));
        return -1;
    }

    return 0;
}


void release_audio_wav_file(output_audio_file *audiofile)
{
    if (audiofile == NULL) return;

    audio_wav_file *wavfile = (audio_wav_file *)audiofile->data;
    if (wavfile != NULL)
    {
        if (wavfile->file != NULL) wav_file_close(audiofile);
        free(wavfile);
    }

    free(audiofile);
}


output_audio_file *create_audio_wav_file(Logger logger, int channels)
{
    audio_wav_file *wavfile = (audio_wav_file *)malloc(sizeof(audio_wav_file));
    if (wavfile == NULL)
    {
        logger(LOGGER_ERROR, "create_audio_wav_file: could not instantiate audio_wav_file: %d, %s\n", errno, strerror(errno));
        return NULL;
    }

    wavfile->logger = logger;
    wavfile->channels = CHANNEL_COUNT;
    wavfile->file = NULL;
    wavfile->sample_rate = 48000;
    wavfile->bits_per_sample = BITS_PER_SAMPLE;
    wavfile->current_pair_count = 0;
    wavfile->total_samples = 0;
    wavfile->header_written = 0;
    wavfile->writer = wav_file_write_multichannel;

    output_audio_file *audiofile = (output_audio_file *)malloc(sizeof(output_audio_file));
    if (audiofile == NULL)
    {
        logger(LOGGER_ERROR, "create_audio_wav_file: could not instantiate audio_file: %d, %s\n", errno, strerror(errno));
        free(wavfile);
        return NULL;
    }

    audiofile->data = wavfile;
    audiofile->open = wav_file_open;
    audiofile->close = wav_file_close;
    audiofile->writeheader = wav_file_write_header;
    audiofile->write = wav_file_write;

    return audiofile;
}
