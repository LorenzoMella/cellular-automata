#include <cstdlib>
#include <optional>
#include <print>
#include <string.h>
#include <string>
#include <vector>
#include <map>

#include "argparse.hpp"


ArgumentParser::~ArgumentParser()
{
    for (auto &o : m_options)
    {
        std::free((void *)o.name);
    }
}


void ArgumentParser::add_arg(std::string const long_option, char short_option, bool expects_arguments)
{
    m_options.push_back({
        .name = strdup(long_option.c_str()), // we don't assume anything about the string lifetime
        .has_arg = expects_arguments ? required_argument : no_argument,
        .flag = nullptr,
        .val = short_option
    });

    m_short = m_short + short_option + (expects_arguments ? ":" : "");
}


void ArgumentParser::add_arg(std::string const long_option, bool expects_arguments)
{
    m_options.push_back({
        .name = strdup(long_option.c_str()), // we don't assume anything about the string lifetime
        .has_arg = expects_arguments ? required_argument : no_argument,
        .flag = &m_getopt_long_flag,
        .val = -((int)m_option_args.size()) - 1
    });
}


void ArgumentParser::add_arg(char short_option, bool expects_arguments)
{
    // We define an identifier starting with # to avoid ambiguities with long option names. Only
    // hyphens and alphanumeric characters are allowed for long options. The ambiguity arises from
    // the fact that nobody prevents the use of single-char long options, such as --F.
    char id[3] {'#', short_option, '\0'};
    m_options.push_back({
        .name = strdup(id),
        .has_arg = expects_arguments ? required_argument : no_argument,
        .flag = nullptr,
        .val = short_option
    });
    m_short = m_short + short_option + (expects_arguments ? ":" : "");
}


void ArgumentParser::parse_args(int argc, char *argv[])
{
    // Temporarily add terminal element to struct option array
    m_options.push_back({0, 0, 0, 0});

    for (;;)
    {
        // longind is the index into m_options.data() of the detected *long option*.
        // getopt_long will not overwrite it if a *short option* is found.
        int longind = -1;
        int c = getopt_long(argc, argv, m_short.c_str(), m_options.data(), &longind);

        // -1 signals termination
        // (argv[optind] is now the first positional argument)
        if (-1 == c)
        {
            break;
        }

        // '?' signals an unrecognized option *or* a known option with missing required argument
        // (getopt_long has already printed an error message to stderr)
        if ('?' == c)
        {
            continue;
        }

        // If the long option was set with no corresponding short option, c == 0, and
        // m_getopt_long_flag is set to an integer identifier
        if (0 == c)
        {
            c = m_getopt_long_flag;
        }

        // FIXME Avoid the quadratic search somehow
        auto it = m_options.begin();
        for (; it != m_options.end(); ++it)
        {
            if (it->val == c)
            {
                if (optarg)
                {
                    m_option_args.insert_or_assign(it->name, std::optional<std::string>{optarg});
                }
                else
                {
                    m_option_args.insert_or_assign(it->name, std::optional<std::string>{});
                }

                break;
            }
        }

        if (it == m_options.end())
        {
            // Getting here likely means that m_options has been corrupted
            std::println(stderr, "{}: undetected overflow of the option array", __func__);
            std::exit(EXIT_FAILURE);
        }
    }

    // Remove temporary terminal element
    m_options.pop_back();

    if (optind < argc)
    {
        m_positional_args.reserve(argc - optind);
        for (int i = optind; i < argc; ++i)
        {
            m_positional_args.emplace_back(argv[i]);
        }
    }
}
