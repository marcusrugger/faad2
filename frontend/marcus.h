#ifndef MARCUS_H_INCLUDED
#define MARCUS_H_INCLUDED

#define MARCUS

int mymain(int argc, char *argv[]);

#define SUCCESS 0
#define SUCCESSFUL(a) ((a) == (SUCCESS))
#define FAILED(a) ((a) != (SUCCESS))

typedef enum
{
    LOGGER_QUIET = 0,
    LOGGER_DISPLAY,
    LOGGER_ERROR,
    LOGGER_WARNING,
    LOGGER_INFO,
    LOGGER_DEBUG
}
LOGGER_LEVEL;

typedef void (*Logger)(LOGGER_LEVEL, char *, ...);
void set_logger_level(LOGGER_LEVEL);


typedef struct
{
    char *input_filename;
    char *output_filename;

    char object_type;
    int channels;
    long samplerate;
    char output_format;
}
cmdline_options;

cmdline_options *initialize_cmdline_options(Logger logger, int argc, char *argv[]);
void release_cmdline_options(cmdline_options *options);
int rescue_media_file(Logger logger, cmdline_options *options);


typedef struct
{
    Logger logger;
    int channels;
    FILE *file;
}
audio_wav_file;

void wav_file_close(audio_wav_file *wavfile);
int wav_file_open(audio_wav_file *wavfile, char *filename);
void release_audio_wav_file(audio_wav_file *wavfile);
audio_wav_file *create_audio_wav_file(Logger logger, int channels);


#endif
