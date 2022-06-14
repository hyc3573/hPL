#ifndef __NODE_HPP
#define __NODE_HPP
#include "tokens.hpp"
#include <list>
#include <locale>
#include <memory>
#include <iostream>

typedef struct Node
{
    Tok type;
    std::list<std::shared_ptr<Node>> children;
    std::weak_ptr<Node> parent;
    std::weak_ptr<Node> self;
    std::list<std::shared_ptr<Node>>::iterator
        i; // insert/erase on children doesn't change this
    Data data;

    Node(){};
    ~Node();
    void add(Tok, Data data=0);

} Node;

void printNode(const std::shared_ptr<const Node> node, const int indent,
               const Node *const cur = NULL);

void printNodeOs(std::ostream& os, const std::shared_ptr<const Node> node, const int indent,
               const Node *const cur = NULL);

#endif
