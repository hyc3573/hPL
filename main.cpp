#include "regex"
#include <deque>
#include <ios>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <regex>
#include <stdarg.h>
#include <string>
#include <vector>

using namespace std;

void panik(const char *format...)
{
    va_list vargs;
    va_start(vargs, format);
    vfprintf(stderr, format, vargs);
    va_end(vargs);

    exit(1);
}

union Data
{
    int n;

    Data(int n) : n(n){};
    Data() : n(0){};
};

enum class Tok
{
    NUM,
    PLUS,
    MINUS,
    MUL,
    DIV,

    OPA,
    CPA,

    E,
    Ep,
    F,
    T,
    Tp,
    e
};

const char *tokstr[] = {"NUM", "+",  "-", "*", "/",  "(", ")",
                        "E",   "E'", "F", "T", "T'", "e"};

typedef struct Node
{
    Tok type;
    list<shared_ptr<Node>> children;
    weak_ptr<Node> parent;
    weak_ptr<Node> self;
    list<shared_ptr<Node>>::iterator
        i; // insert/erase on children doesn't change this
    Data data;

    void add(Tok newtype, Data data = 0)
    {
        children.push_back(make_shared<Node>());
        children.back()->self = children.back();
        children.back()->type = newtype;
        children.back()->children = {};
        children.back()->parent = self;
        children.back()->i = --children.end();
        children.back()->data = data;
    }

    ~Node()
    {
        children.clear();
    }

    Node()
    {
    }

} Node;

bool clean(const shared_ptr<Node> node)
{
    for (auto i = node->children.begin(); i != node->children.end();)
    {
        bool shouldErase = clean(*i);
        if (shouldErase)
            i = node->children.erase(i);
        else
            ++i;
    }

    auto parent = node->parent.lock();

    switch (node->type)
    {
    case Tok::T:
    case Tok::E:
        if (node->children.size() == 3)
        {
            // elevate operator
            auto op = next(node->children.begin());
            node->type = op->get()->type;
            node->children.erase(op);

            break;
        }
        else if (node->children.size() == 1)
        {
            if (node->parent.expired())
            {
                panik("parent freed!\n");
            }

            auto parent = node->parent.lock();
            parent->children.insert(node->i, node->children.front());

            return true;
        }

    case Tok::F:
    case Tok::e:
    case Tok::Tp:
    case Tok::Ep:
        // case Tok::T:
        {
            // elevate children
            for (auto i = node->children.begin(); i != node->children.end();
                 i++)
            {
                parent->children.insert(node->i, *i);
            }

            // indicate that we should delete this
            return true;
        }
        break;

    case Tok::OPA:
    case Tok::CPA:
        return true;

    default:
        break;
    }

    return false;
}

void printNode(const shared_ptr<const Node> node, const int indent,
               const Node *const cur = NULL)
{
    for (int i = 0; i < indent; i++)
        printf("|   ");

    printf("%s: %d", tokstr[static_cast<int>(node->type)], node->data);
    if (node.get() == cur)
    {
        printf("*");
    }
    printf("\n");

    for (auto &n : node->children)
    {
        printNode(n, indent + 1, cur);
    }
}

typedef struct
{
    regex re;
    Tok token;
} LexToken;

int main()
{
    // lexer
    cout << "lexing:" << endl << endl;
    LexToken rule[] = {
        {regex("[0-9]+", regex::extended), Tok::NUM},
        {regex("\\+", regex::extended), Tok::PLUS},
        {regex("\\-", regex::extended), Tok::MINUS},
        {regex("\\*", regex::extended), Tok::MUL},
        {regex("\\/", regex::extended), Tok::DIV},
        {regex("\\(", regex::extended), Tok::OPA},
        {regex("\\)", regex::extended), Tok::CPA},
    };
    regex ignore = regex("[ \n\t]+", regex::extended);

    string input = "1+2*3+(4+5)+6";

    vector<Tok> in;
    vector<Data> data;

    while (input.size())
    {
        bool hadMatch = false;
        smatch match;

        for (auto &tok : rule)
        {
            auto result = regex_search(input, match, tok.re);
            if (result && match.position() == 0)
            {
                // we have a match!!
                string str = match.str();
                cout << str << endl;
                cout << tokstr[static_cast<int>(tok.token)] << endl << endl;
                input = match.suffix();
                in.push_back(tok.token);
                data.push_back(0);

                if (tok.token == Tok::NUM)
                {
                    data.back().n = stoi(str);
                }

                hadMatch = true;

                break;
            }
        }

        if (regex_search(input, match, ignore))
        {
            input = match.suffix();
            hadMatch = true;
        }

        if (!hadMatch)
        {
            panik("Lexing error!\n");
        }
    }
    cout << "----------------------------------" << endl;
    cout << "parsing:" << endl << endl;

    // parser

    // rules:
    // E -> T Ep
    // Ep -> + T Ep | - T Ep | e
    // T  -> F Tp
    // Tp -> * F Tp | / F Tp | e
    // F -> (E) | NUM

    if (in.size() == 0)
        panik("empty input!\n");

    deque<Tok> st1;
    deque<Tok> st2;
    st2.push_back(Tok::E);

    shared_ptr<Node> cur = make_shared<Node>();
    shared_ptr<Node> tree = cur;
    tree->type = Tok::E;
    tree->parent = weak_ptr<Node>();
    tree->self = tree;
    tree->i = tree->children.begin();
    tree->data = 0;

    // recursive descent parser
    int pos = 0;
    bool shift = false;
    while (in.size() > pos)
    {
        shift = false;

        if (st1.empty())
        {
            shift = true;
        }
        else
        {
            switch (st1.back())
            {
            case Tok::E:
                cout << "E -> T E'" << endl;
                st1.pop_back();
                st1.push_back(Tok::T);
                st2.push_front(Tok::Ep);

                cur->add(Tok::T);
                cur->add(Tok::Ep);

                cur = cur->children.front();
                break;

            case Tok::T:
                cout << "T -> F T'" << endl;
                st1.pop_back();
                st1.push_back(Tok::F);
                st2.push_front(Tok::Tp);

                cur->add(Tok::F);
                cur->add(Tok::Tp);
                cur = cur->children.front();
                break;

            case Tok::F:
                switch (in[pos])
                {
                case Tok::NUM:
                    cout << "F -> NUM" << endl;
                    st1.pop_back();
                    st1.push_back(Tok::NUM);

                    cur->add(Tok::NUM, data[pos]);
                    cur = cur->children.front();

                    shift = true;
                    pos++;

                    break;

                case Tok::OPA:
                    cout << "F -> ( E )" << endl;
                    st1.pop_back();
                    st1.push_back(Tok::OPA);
                    st1.push_back(Tok::E);
                    st2.push_front(Tok::CPA);

                    cur->add(Tok::OPA);
                    cur->add(Tok::E);
                    cur->add(Tok::CPA);
                    cur = *next(cur->children.begin());

                    pos++;
                    break;

                default:
                    panik("F -> e does not exist!\n");
                    break;
                }
                break;

            case Tok::Tp:
                switch (in[pos])
                {
                case Tok::MUL:
                    cout << "T' -> * F T'" << endl;
                    st1.pop_back();
                    st1.push_back(Tok::MUL);
                    st1.push_back(Tok::F);

                    st2.push_front(Tok::Tp);

                    cur->add(Tok::MUL);
                    cur->add(Tok::F);
                    cur->add(Tok::Tp);
                    cur = *next(cur->children.begin());

                    pos++;
                    break;

                case Tok::DIV:
                    cout << "T' -> / F T'" << endl;
                    st1.pop_back();
                    st1.push_back(Tok::DIV);
                    st1.push_back(Tok::F);

                    st2.push_front(Tok::Tp);

                    cur->add(Tok::DIV);
                    cur->add(Tok::F);
                    cur->add(Tok::Tp);
                    cur = *next(cur->children.begin());

                    pos++;
                    break;

                default:
                    cout << "T' -> e" << endl;
                    st1.pop_back();

                    cur->type = Tok::e;
                    shift = true;

                    break;
                }
                break;

            case Tok::Ep:
                switch (in[pos])
                {
                case Tok::PLUS:
                    cout << "E' -> + T E'" << endl;
                    st1.pop_back();
                    st1.push_back(Tok::PLUS);
                    st1.push_back(Tok::T);

                    st2.push_front(Tok::Ep);

                    cur->add(Tok::PLUS);
                    cur->add(Tok::T);
                    cur->add(Tok::Ep);
                    cur = *next(cur->children.begin());

                    pos++;
                    break;
                case Tok::MINUS:
                    cout << "E' -> - T E'" << endl;
                    st1.pop_back();
                    st1.push_back(Tok::MINUS);
                    st1.push_back(Tok::T);

                    st2.push_front(Tok::Ep);

                    cur->add(Tok::MINUS);
                    cur->add(Tok::T);
                    cur->add(Tok::Ep);
                    cur = *next(cur->children.begin());
                    cur = *next(cur->children.begin()); //

                    pos++;
                    break;

                default:
                    cout << "E' -> e" << endl;
                    st1.pop_back();

                    cur->type = Tok::e;
                    shift = true;

                    break;
                }
                break;

            case Tok::CPA:
                if (in[pos] == Tok::CPA)
                {
                    pos++;
                }

            default:
                shift = true;
                break;
            }
        }

        if (shift)
        {
            if (st2.empty())
                break;

            cout << "Shift: ";
            st1.push_back(st2.front());
            st2.pop_front();

            // make cur point to next leaf node on tree (depth first)
            // traverse up until we can traverse next element in list.
            list<shared_ptr<Node>>::iterator
                i; // i is iterator used like pointer of linked list node.
            do
            {
                if (cur->parent.expired())
                    goto pass; // goto statement considered harmful considered
                               // harmful?

                i = cur->i;
                i++;
                cur = cur->parent.lock();
            } while (cur->children.end() == i); // if it is not valid
            cur = *i;
        pass:

            cout << tokstr[static_cast<int>(st1.back())] << endl;
        }

        cout << endl << "stack" << endl;
        for (auto i : st1)
        {
            cout << tokstr[static_cast<int>(i)] << " ";
        }
        cout << "| ";
        for (auto i : st2)
        {
            cout << tokstr[static_cast<int>(i)] << " ";
        }
        cout << endl << endl << "in" << endl;

        for (int i = 0; i < in.size(); i++)
        {
            if (i == pos)
                cout << "| ";

            cout << tokstr[static_cast<int>(in[i])] << " ";
        }
        cout << endl;
        cout << endl;
        cout << "tree" << endl;
        printNode(tree, 0, cur.get());
        cout << "----------------------------------" << endl;
    }

    for (auto i : st1)
    {
        cout << tokstr[static_cast<int>(i)] << " ";
    }

    for (auto i : st2)
    {
        cout << tokstr[static_cast<int>(i)] << " ";
    }
    cout << endl;

    printNode(tree, 0);
    cout << "----------------------------------" << endl;

    clean(tree);

    printNode(tree, 0);
}
