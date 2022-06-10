#include "utils.hpp"

void panik(const char *format...)
{
    va_list vargs;
    va_start(vargs, format);
    vfprintf(stderr, format, vargs);
    va_end(vargs);

    exit(1);
}

