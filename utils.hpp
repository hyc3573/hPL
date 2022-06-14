#ifndef __UTILS_HPP
#define __UTILS_HPP

#include <stdarg.h>
#include <stdexcept>
#include <stdio.h>
#include <cstdlib>
#include <exception>
#include <string>

void panik(const char *format...);

class parse_error: public std::runtime_error
{
public:
    parse_error(char const* const message);
};

class lex_error : public std::runtime_error
{
public:
    lex_error(char const* const message);
};

#endif
