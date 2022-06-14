#include "utils.hpp"
#include <exception>
#include <stdexcept>

parse_error::parse_error(char const* const message) : std::runtime_error(message) {};
lex_error::lex_error(char const* const message) : std::runtime_error(message) {};

void panik(const char *format...)
{
    va_list vargs;
    va_start(vargs, format);
    vfprintf(stderr, format, vargs);
    va_end(vargs);

    exit(1);
}

