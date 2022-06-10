#ifndef __PARSE_HPP
#define __PARSE_HPP

#include "node.hpp"
#include <deque>
#include <ios>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <string>
#include <vector>

bool clean(const std::shared_ptr<Node> node);

void parse(const std::shared_ptr<Node> tree, const std::vector<Tok>& in,
           const std::vector<Data>& data);

#endif
