#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "getopt.h"


#ifdef __cplusplus
extern "C" {
#endif


#define GETOPT_PRINT_ERR(...)                                                                      \
    do                                                                                             \
    {                                                                                              \
        if (opterr)                                                                                \
            fprintf(stderr, __VA_ARGS__);                                                          \
    } while (0)


int optind         = 1;
char const *optarg = NULL;
int optopt         = -1;
int opterr         = 1;

char *ProgName;


char *base_name(char *argv_0)
{
    char *p_slash     = strrchr(argv_0, '/');
    char *p_backslash = strrchr(argv_0, '\\');

    // TODO Maybe not the best solution, but is it my fault that Windows allows
    // both styles of directory separators?
    if (p_slash - argv_0 > p_backslash - argv_0)
    {
        return p_slash ? (p_slash + 1) : argv_0;
    }
    else
    {
        return p_backslash ? (p_backslash + 1) : argv_0;
    }
}


inline bool not_terminal_element(size_t i, struct option const *long_options)
{
    return long_options[i].name || long_options[i].has_arg || long_options[i].flag || long_options[i].val;
}


static struct option const *find_long_option(struct option const *long_options, char *token, char **p_arg)
{
    // Find the '=' if present
    char *p_equals = strchr(token, '=');  // NULL if not found

    if (p_equals)
    {
        *p_equals = '\0';  // we shorten the string for strcmp
        *p_arg = p_equals + 1;  // worst-case scenario, p_arg points to an empty string
    }
    else
    {
        *p_arg = NULL;
    }

    // Search for match
    for (size_t i = 0; not_terminal_element(i, long_options); ++i)
    {
        if (strcmp(token, long_options[i].name) == 0)
        {
            return &long_options[i];
        }
    }

    return NULL;
}


static void rotate_args_left(char **args, int length, int from)
{
    char *buf = args[from];

    for (int i = from + 1; i < length; ++i)
    {
        args[i - 1] = args[i];
    }

    args[length - 1] = buf;
}


static int parse_short_option(int argc,
                              char **argv,
                              char const *options,
                              size_t n_options,
                              int *last_token_pos,
                              char const *prog_name)
{
    char const *p_shortopt = (char const *)memchr(options, argv[optind][1], n_options);

    optopt = argv[optind][1];

    // If the short option is unknown
    if (!p_shortopt)
    {
        GETOPT_PRINT_ERR("%s: invalid option -- '%c'\n", prog_name, argv[optind][1]);
        optarg = NULL;
        optind++;
        return '?';
    }

    // If an argument is expected...
    if (*(p_shortopt + 1) == ':')
    {
        // ... and found in the same token, i.e., omitting whitespace
        if (argv[optind][2] != '\0')
        {
            optarg = argv[optind] + 2;
            optind++;
            return *p_shortopt;
        }
        else if (*last_token_pos > optind && argv[optind + 1][0] != '-') // ... or found after whitespace (as the next token)
        {
            optarg = argv[optind + 1];
            optind += 2;
            return *p_shortopt;
        }
        else
        {
            GETOPT_PRINT_ERR("%s: option requires an argument -- '%c'\n", prog_name, *p_shortopt);
            optind++;
            return '?';
        }
    }
    else // ... or no argument expected
    {
        optarg = NULL;

        // If the short-option token has trailing characters, two cases arise. See below.
        // (In both cases, the token will be parsed again: optind is left unchanged).
        if (argv[optind][2] != '\0')
        {
            // If first trailing character is itself a short option, we interpret the token as
            // short-option chaining, and we prepare it to parse the next short option in the next
            // call.
            if (memchr(options, argv[optind][2], n_options))
            {
                // Mutate the token into the next short-option (and the other trailing characters if any)
                argv[optind][1] = '-';
                argv[optind] = argv[optind] + 1;
            }
            else
            {
                // Otherwise, consider the trailing part of the token to be a positional argument.
                argv[optind] = argv[optind] + 2;  // Skip the option characters
                rotate_args_left(argv, argc, optind);
                (*last_token_pos)--;
            }
        }
        else // If the token only contains the two short-option characters, there is no ambiguity:
             // advance optind normally.
        {
            optind++;
        }

        return *p_shortopt;
    }
}


int parse_long_option(int argc,
                      char **argv,
                      struct option const *long_options,
                      int *last_token_pos,
                      char const *prog_name,
                      int *indexptr)
{
    // Parse long-option (+2 to remove the double dash)
    char *p_arg = NULL;
    struct option const *lo = find_long_option(long_options, argv[optind] + 2, &p_arg);

    if (!lo)
    {
        GETOPT_PRINT_ERR("%s: unrecognized option '%s'\n", prog_name, argv[optind]);
        optind++;
        return '?';
    }

    // If the long-option is found, we return its index into long_options using *indexptr
    *indexptr = (int)(lo - long_options);

    // Register argument if present
    if (lo->has_arg)
    {
        // GNU getopt regards anything after '=' or space to be the argument
        // (even if it starts with a dash or double-dash!)
        if (p_arg)
        {
            if (*last_token_pos > optind)
            {
                optarg = p_arg;
            }
            else
            {
                GETOPT_PRINT_ERR("%s: option '%s' requires an argument\n", prog_name, argv[optind]);
                optarg = NULL;
            }
            // (optind stays in place)
        }
        else
        {
            if (*last_token_pos > optind)
            {
                optarg = argv[optind + 1];
            }
            else
            {
                GETOPT_PRINT_ERR("%s: option '%s' requires an argument\n", prog_name, argv[optind]);
                optarg = NULL;
            }
            optind++;
        }
    }

    if (!lo->flag)
    {
        optind++;
        return lo->val;
    }
    else
    {
        *lo->flag = lo->val;
        optind++;
        return 0;
    }
}


int getopt(int argc, char **argv, char const *options)
{
    static int last_token_pos;

    last_token_pos = argc - 1;

    char *prog_name = base_name(argv[0]);

    size_t n_options = strlen(options);

    // All tokens have been consumed
    if (optind > last_token_pos)
    {
        return -1;
    }

    // Deal with positional arguments (TODO test for non-options and single dashes)
    while (argv[optind][0] != '-' || argv[optind][1] == '\0')
    {
        rotate_args_left(argv, argc, optind);
        // (optind stays in place)
        if (optind > --last_token_pos)
        {
            return -1;
        }
    }

    // The token read here is guaranteed to start with a dash

    // Deal with the terminating double dash
    if (argv[optind][1] == '-')
    {
        if (argv[optind][2] == '\0')
        {
            optind++;
        }
        else
        {
            // The terminating double dash can be followed by a non-space-separated argument
            argv[optind] = argv[optind] + 2;
        }

        // This effectively terminates parsing and fixes optind
        last_token_pos = optind - 1;
        return -1;
    }

    return parse_short_option(argc, argv, options, n_options, &last_token_pos, prog_name);
}


int getopt_long(int argc, char **argv, char const *options, struct option const *long_options, int *idx)
{
    static int last_token_pos;

    last_token_pos = argc - 1;

    char *prog_name = base_name(argv[0]);

    // All tokens have been consumed
    if (optind > last_token_pos)
    {
        return -1;
    }

    // The loop is necessary to rotate multiple positional arguments silently in one go
    // Deal with positional arguments
    while (argv[optind][0] != '-' || argv[optind][1] == '\0')  // Non-options and single dashes are positional
    {
        rotate_args_left(argv, argc, optind);
        if (optind > --last_token_pos)
        {
            return -1;
        }
        // (optind stays in place)
    }

    // The token read here is guaranteed to start with a dash

    if (argv[optind][1] == '-') // if double dash
    {
        // Parse terminating double dash
        if (argv[optind][2] == '\0')
        {
            optind++;
            // Ignore the rest of the options: consider them to be positional parameters
            last_token_pos = optind - 1;
            return -1;
        }

        return parse_long_option(argc, argv, long_options, &last_token_pos, prog_name, idx);
    }
    else
    {
        // Parse short-option
        size_t n_options = strlen(options);
        return parse_short_option(argc, argv, options, n_options, &last_token_pos, prog_name);
    }
}

#ifdef __cplusplus
}
#endif
