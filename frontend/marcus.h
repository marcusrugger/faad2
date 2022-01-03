#ifndef MARCUS_H_INCLUDED
#define MARCUS_H_INCLUDED

#define MARCUS

int mymain(int argc, char *argv[]);

#define SUCCESS 0
#define SUCCESSFUL(a) ((a) == (SUCCESS))
#define FAILED(a) ((a) != (SUCCESS))

typedef enum
{
    LOGGER_DISPLAY = 0,
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
}
cmdline_options;

cmdline_options *initialize_cmdline_options(Logger logger, int argc, char *argv[]);


#endif
