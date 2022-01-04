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

    int infile_seek_position;
    char object_type;
    int channels;
    long samplerate;
    char aac_output_format;
}
cmdline_options;

cmdline_options *initialize_cmdline_options(Logger logger, int argc, char *argv[]);
void release_cmdline_options(cmdline_options *options);
int rescue_media_file(Logger logger, cmdline_options *options);


typedef struct output_audio_file_tag output_audio_file;
struct output_audio_file_tag
{
    void *data;
    int (*open)(output_audio_file *, char *);
    void (*close)(output_audio_file *);
    int (*writeheader)(output_audio_file *);
    int (*write)(output_audio_file *, unsigned char *samples, int sample_count, int channel_count);
};

void release_audio_wav_file(output_audio_file *wavfile);
output_audio_file *create_audio_wav_file(Logger logger, int channels);


#endif
