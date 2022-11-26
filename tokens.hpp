#ifndef __TOKENS_HPP
#define __TOKENS_HPP

#include <iterator>
#include <regex>
#include <iostream>
#include <string>
#include <memory>
#include <variant>

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

    DELIM,

    LBL,
    GOTO,
    IF,

    EQ,

    EXPR,
    Xp,

    OBR,
    CBR,

    COMMA,
    LIST,
    Lp,

    FUN
};

std::ostream& operator<<(std::ostream& os, const Tok& tok);

typedef struct
{
    std::regex re;
    Tok token;
} LexToken;

extern const char *tokstr[30];
extern LexToken rule[18];
extern std::regex strignore;

// union Data
// {
//     int n;
//     std::shared_ptr<std::string> str;

//     Data(int n) : n(n){};
//     Data(std::string str) : str(std::make_shared<std::string>(str)) {};
//     Data() : n(0){};
//     ~Data() {};
// };

typedef std::variant<std::monostate, int, std::string> Data;

#endif
