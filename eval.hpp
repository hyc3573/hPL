#ifndef __EVAL_HPP
#define __EVAL_HPP

#include <map>
#include <string>
#include <stack>
#include <memory>
#include "node.hpp"

struct Context
{
    std::stack<std::map<std::string, int>> stack;
};

Data evaluate(std::shared_ptr<Node> AST, Context& context);

#endif
