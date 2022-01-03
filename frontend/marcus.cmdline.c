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


static int process_cmdline_option_input_filename(Logger logger, cmdline_options *options)
{
    if (optarg == NULL)
    {
        logger(LOGGER_WARNING, "Missing input filename parameter.\n");
        return -1;
    }

    options->input_filename = (char *) malloc(sizeof(char) * (strlen(optarg) + 1));
    if (options->input_filename == NULL)
    {
        logger(LOGGER_ERROR, "Error allocating memory for input_filename.\n");
        return -1;
    }

    strcpy(options->input_filename, optarg);
    return 0;
}


static int process_cmdline_option_logger_level(Logger logger)
{
    if (optarg == NULL)
    {
        logger(LOGGER_WARNING, "Missing logger level parameter.\n");
        return -1;
    }

    if (strcasecmp(optarg, "quiet") == 0) set_logger_level(LOGGER_QUIET);
    else if (strcasecmp(optarg, "display") == 0) set_logger_level(LOGGER_DISPLAY);
    else if (strcasecmp(optarg, "error") == 0) set_logger_level(LOGGER_ERROR);
    else if (strcasecmp(optarg, "warning") == 0) set_logger_level(LOGGER_WARNING);
    else if (strcasecmp(optarg, "info") == 0) set_logger_level(LOGGER_INFO);
    else if (strcasecmp(optarg, "debug") == 0) set_logger_level(LOGGER_DEBUG);
    else set_logger_level(LOGGER_DISPLAY);

    return 0;
}


static int process_cmdline_option_output_filename(Logger logger, cmdline_options *options)
{
    if (optarg == NULL)
    {
        logger(LOGGER_WARNING, "Missing output filename parameter.\n");
        return -1;
    }

    options->output_filename = (char *) malloc(sizeof(char) * (strlen(optarg) + 1));
    if (options->output_filename == NULL)
    {
        logger(LOGGER_ERROR, "Error allocating memory for output_filename.\n");
        return -1;
    }

    strcpy(options->output_filename, optarg);
    return 0;
}


static int process_cmdline_option_samplerate(Logger logger, cmdline_options *options)
{
    if (optarg == NULL)
    {
        logger(LOGGER_WARNING, "Missing sample rate parameter.\n");
        return -1;
    }

    options->samplerate = atol(optarg);
    return 0;
}


static int display_help_options(Logger logger)
{
    logger(LOGGER_DISPLAY, "Current available options:\n\n");
    logger(LOGGER_DISPLAY, "-h, --help:       display this help message\n");
    logger(LOGGER_DISPLAY, "-i, --infile:     input filename\n");
    logger(LOGGER_DISPLAY, "-l, --logger:     logging level (quiet, display, error, warning, info, debug)\n");
    logger(LOGGER_DISPLAY, "-s, --samplerate: sample rate\n");
    logger(LOGGER_DISPLAY, "-o, --outfile:    output filename\n");
    return -1;
}


static int process_cmdline_option(Logger logger, cmdline_options *options, int c)
{
    switch (c)
    {
        case 'h': return display_help_options(logger);
        case 'i': return process_cmdline_option_input_filename(logger, options);
        case 'l': return process_cmdline_option_logger_level(logger);
        case 'o': return process_cmdline_option_output_filename(logger, options);
        case 's': return process_cmdline_option_samplerate(logger, options);
        default:
            logger(LOGGER_WARNING, "Unrecognized option: %c\n", c);
            return 0;
    }
}


static int process_cmdline_options(Logger logger, cmdline_options *options, int argc, char *argv[])
{
    int ch = -1;
    int option_index = 0;
    static struct option long_options[] =
    {
        { "infile",     required_argument, 0, 'i' },
        { "outfile",    required_argument, 0, 'o' },
        { "logger",     required_argument, 0, 'l' },
        { "samplerate", required_argument, 0, 's' },
        { "help",       no_argument,       0, 'h' },
        { 0, 0, 0, 0 }
    };

    while ((ch = getopt_long(argc, argv, "o:h", long_options, NULL)) != -1)
    {
        int result = process_cmdline_option(logger, options, ch);
        if (FAILED(result)) return result;
    }

    return 0;
}


static void set_to_defaults(cmdline_options *options)
{
    options->input_filename = NULL;
    options->output_filename = NULL;
    options->object_type = LC;
    options->samplerate = 48000;
    options->output_format = FAAD_FMT_16BIT;
}

void release_options(cmdline_options *options)
{
    if (options->input_filename != NULL)
        free(options->input_filename);

    if (options->output_filename != NULL)
        free(options->output_filename);

    free(options);
}


cmdline_options *initialize_cmdline_options(Logger logger, int argc, char *argv[])
{
    cmdline_options *options = (cmdline_options *) malloc(sizeof(cmdline_options));
    if (options == NULL) return NULL;
    set_to_defaults(options);

    int result = process_cmdline_options(logger, options, argc, argv);
    if (SUCCESSFUL(result)) return options;

    release_options(options);
    return NULL;
}
