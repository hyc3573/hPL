#include "node.hpp"

using namespace std;

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

void printNodeOs(std::ostream &os, const std::shared_ptr<const Node> node,
                 const int indent, const Node *const cur)
{
    for (int i = 0; i < indent; i++)
        os << "|   ";

    os << node->type << ": " << node->data.n;
    if (node.get() == cur)
    {
        os << "*";
    }
    os << "\n";

    for (auto i = node->children.begin(); i != node->children.end(); ++i)
    {
        printNodeOs(os, *i, indent + 1, cur);
    }
}

void printNode(const std::shared_ptr<const Node> node, const int indent,
               const Node *const cur)
{
    printNodeOs(std::cout, node, indent, cur);
}
