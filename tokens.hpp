#ifndef __TOKENS_HPP
#define __TOKENS_HPP

#include <iterator>
#include <regex>
#include <iostream>

enum class Tok
{
    NUM,
    PLUS,
    MINUS,
    MUL,
    DIV,

    OPA,
    CPA,

    E,
    Ep,
    F,
    T,
    Tp,
    e,

    ID,
    SUBS,

    STMT,
    PROG,

    DELIM
};

std::ostream& operator<<(std::ostream& os, const Tok& tok);

typedef struct
{
    std::regex re;
    Tok token;
} LexToken;

extern const char *tokstr[18];
extern LexToken rule[10];
extern std::regex strignore;

union Data
{
    int n;

    Data(int n) : n(n){};
    Data() : n(0){};
};

#endif
