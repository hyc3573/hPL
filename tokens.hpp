#ifndef __TOKENS_HPP
#define __TOKENS_HPP

#include <regex>

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
    e
};

typedef struct
{
    std::regex re;
    Tok token;
} LexToken;

extern const char *tokstr[13];
extern LexToken rule[7];
extern std::regex strignore;

union Data
{
    int n;

    Data(int n) : n(n){};
    Data() : n(0){};
};

#endif
