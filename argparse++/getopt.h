/** getopt.h -- My own rough implementation of GNU getopt and getopt_long */

#ifndef H_GETOPT_H
#define H_GETOPT_H


#ifdef __cplusplus
extern "C" {
#endif


extern int         optind;
extern int         optind;
extern char const *optarg;
extern int         optopt;
extern int         opterr; // "verbose" flag for the error messages


struct option
{
    char const *name;
    int has_arg;
    int *flag;
    int val;
};


enum ArgumentType {
    no_argument = 0,
    required_argument
};


int getopt(int argc, char **argv, char const *options);

int getopt_long(int argc, char **argv, char const *options, struct option const *long_options, int *idx);


#ifdef __cplusplus
}
#endif


#endif /* H_GETOPT_H */
