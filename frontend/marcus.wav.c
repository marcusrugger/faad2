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


void wav_file_close(audio_wav_file *wavfile)
{
    if (wavfile == NULL) return;
    if (wavfile->file == NULL) return;
    fclose(wavfile->file);
}


int wav_file_open(audio_wav_file *wavfile, char *filename)
{
    if (wavfile == NULL) return -1;

    if (filename == NULL)
    {
        wavfile->logger(LOGGER_ERROR, "filename may not be null\n");
        return -1;
    }

    wavfile->file = fopen(filename, "wb");
    if (wavfile->file == NULL)
    {
        wavfile->logger(LOGGER_ERROR, "Could not open output wav file: %d, %s\n", errno, strerror(errno));
        free(wavfile);
        return -1;
    }

    return 0;
}


void release_audio_wav_file(audio_wav_file *wavfile)
{
    if (wavfile == NULL) return;
    free(wavfile);
}


audio_wav_file *create_audio_wav_file(Logger logger, int channels)
{
    audio_wav_file *wavfile = (audio_wav_file *)malloc(sizeof(audio_wav_file));
    if (wavfile == NULL)
    {
        logger(LOGGER_ERROR, "Could not instantiate audio_wav_file: %d, %s\n", errno, strerror(errno));
        return NULL;
    }

    wavfile->logger = logger;
    wavfile->channels = channels;
    wavfile->file = NULL;

    return wavfile;
}
