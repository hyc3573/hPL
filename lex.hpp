#ifndef __LEX_HPP
#define __LEX_HPP

#include "tokens.hpp"
#include <string>
#include <vector>
#include <iostream>
#include "utils.hpp"

void lex(std::string input, std::vector<Tok> &in, std::vector<Data> &data);

#endif
