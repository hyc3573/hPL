#include "node.hpp"

void Node::add(Tok newtype, Data data)
{
    using namespace std;

    children.push_back(make_shared<Node>());
    children.back()->self = children.back();
    children.back()->type = newtype;
    children.back()->children = {};
    children.back()->parent = self;
    children.back()->i = --children.end();
    children.back()->data = data;
}

Node::~Node()
{
    children.clear();
}

void printNode(const std::shared_ptr<const Node> node, const int indent,
               const Node *const cur)
{
    for (int i = 0; i < indent; i++)
        printf("|   ");

    printf("%s: %d", tokstr[static_cast<int>(node->type)], node->data);
    if (node.get() == cur)
    {
        printf("*");
    }
    printf("\n");

    for (auto i=node->children.begin();i!=node->children.end();++i)
    {
        printNode(*i, indent + 1, cur);
    }
}


