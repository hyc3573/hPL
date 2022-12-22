#ifndef __EVAL_HPP
#define __EVAL_HPP

#include <map>
#include <string>
#include <deque>
#include <memory>
#include "node.hpp"

struct Context
{
    std::deque<std::map<std::string, Data>> stack;
    bool goto_flag;
    std::string goto_label;

    Context();

    Data get(std::string id);
    void set(std::string id, Data value);

    void push();
    void pop();
};

Data evaluate(std::shared_ptr<Node> AST, Context& context);

#endif
