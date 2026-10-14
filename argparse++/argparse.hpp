// argparse -- Argument Parser ++
// Version: 0.1
// An argument parser API inspired by Python argparse.

// The GNU getopt_long function does the heavy-lifting (implying that glibc is a
// dependecy).
// I also wrote a drop-in replacement of getopt.h for Windows.
// MacOS includes the BSD getopt_long, which is very similar. The docs tell all
// about the differences.

// Current implementation: both long and short options can be specified, either
// in pairs, or just the long or the short one. At most one (space-separated) argument
// is supported for any option, and no type conversion is applied: the
// result is stored as a C++ string.
// Use comma-separated values or other client-side conventions to allow for multiple arguments.

// To do:
// - Dealing with multiple whitespace-separated arguments per option
// - Type conversions
// - Check long and short-option syntax


#ifndef H_ARGPARSE_HPP
#define H_ARGPARSE_HPP


#if defined(__GNUC__) || defined(__clang__)
#    include <getopt.h>
#else
#    include "getopt.h"
#endif

#include <map>
#include <optional>
#include <string>
#include <vector>


struct ArgumentParser
{
    ArgumentParser() {};

    ~ArgumentParser();

    auto begin() { return std::begin(m_option_args); }

    auto end() { return std::end(m_option_args); }

    std::string get_positional(size_t n) const { return m_positional_args[n]; }

    size_t n_positional() const { return m_positional_args.size(); }

    std::optional<std::string> get_optional(std::string long_option) const { return m_option_args.at(long_option); }

    bool has_optional(std::string long_option) const { return m_option_args.contains(long_option); }

    void parse_args(int argc, char *argv[]);

    void add_arg(std::string const long_option, char short_option, bool expects_arguments);

    void add_arg(std::string const long_option, bool expects_arguments);

    void add_arg(char short_option, bool expects_arguments);

private:
    std::vector<struct option> m_options {};
    std::string m_short {};

    std::map<std::string, std::optional<std::string>> m_option_args {};
    std::vector<std::string> m_positional_args {};

    int m_getopt_long_flag {0};
};


#endif /* H_ARGPARSE_HPP */
