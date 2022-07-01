#include "tokens.hpp"

const char *tokstr[] = {
    "NUM", "+", "-", "*", "/", "(", ")", "E", "E'", "F", "T", "T'", "e", "ID", "=", "statement", "program", ";", "label", "goto", "if", "==", "expression", "X'", "{", "}", ",", "LIST", "L'"};

LexToken rule[] = {
        {std::regex("[0-9]+", std::regex::extended), Tok::NUM},
        {std::regex("\\+", std::regex::extended), Tok::PLUS},
        {std::regex("-", std::regex::extended), Tok::MINUS},
        {std::regex("\\*", std::regex::extended), Tok::MUL},
        {std::regex("/", std::regex::extended), Tok::DIV},
        {std::regex("\\(", std::regex::extended), Tok::OPA},
        {std::regex("\\)", std::regex::extended), Tok::CPA},
        {std::regex("==", std::regex::extended), Tok::EQ},
        {std::regex("label", std::regex::extended), Tok::LBL},
        {std::regex("goto", std::regex::extended), Tok::GOTO},
        {std::regex("if", std::regex::extended), Tok::IF},
        {std::regex("=", std::regex::extended), Tok::SUBS},
        {std::regex(";", std::regex::extended), Tok::DELIM},
        {std::regex("\\{", std::regex::extended), Tok::OBR},
        {std::regex("}", std::regex::extended), Tok::CBR},
        {std::regex(",", std::regex::extended), Tok::COMMA},

        {std::regex("[a-zA-Z_-][a-zA-Z0-9_-]*", std::regex::extended), Tok::ID}
};

std::regex strignore = std::regex("[ \n\t]+", std::regex::extended);

std::ostream& operator<<(std::ostream& os, const Tok& in)
{
    os << tokstr[(int)in];
    return os;
}
