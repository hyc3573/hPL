#include "tokens.hpp"

const char *tokstr[] = {"NUM", "+",  "-", "*", "/",  "(", ")",
                        "E",   "E'", "F", "T", "T'", "e"};

LexToken rule[] = {
        {std::regex("[0-9]+", std::regex::extended), Tok::NUM},
        {std::regex("\\+", std::regex::extended), Tok::PLUS},
        {std::regex("\\-", std::regex::extended), Tok::MINUS},
        {std::regex("\\*", std::regex::extended), Tok::MUL},
        {std::regex("\\/", std::regex::extended), Tok::DIV},
        {std::regex("\\(", std::regex::extended), Tok::OPA},
        {std::regex("\\)", std::regex::extended), Tok::CPA},
};

std::regex strignore = std::regex("[ \n\t]+", std::regex::extended);
