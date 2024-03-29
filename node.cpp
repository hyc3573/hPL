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

void Node::add(std::shared_ptr<Node> tree)
{
    using namespace std;
    children.push_back(tree);
    children.back()->parent = self;
    children.back()->i = prev(children.end());
}

void Node::add(std::shared_ptr<Node> tree,
               std::list<shared_ptr<Node>>::iterator i)
{
    using namespace std;
    children.insert(i, tree);
    children.back()->parent = self;
    children.back()->i = prev(children.end());
}

bool Node::validate()
{
    bool result = !self.expired();

    for (auto i=children.begin();i!=children.end();i++)
    {
        result =
            result &&
            !(**i).parent.expired() &&
            (**i).parent.lock() == self.lock() &&
            (**i).validate() &&
            (**i).i == i;
    }

    return result;
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

    os << node->type;

    if (!holds_alternative<monostate>(node->data))
        os << ": ";
    
    if (holds_alternative<long long int>(node->data))
        os << get<long long int>(node->data);
    else if (holds_alternative<string>(node->data))
        os << get<string>(node->data);
    
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
