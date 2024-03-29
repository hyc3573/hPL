#include "parse.hpp"
#include "node.hpp"
#include "utils.hpp"

#include <boost/container_hash/extensions.hpp>
#include <exception>
#include <iterator>
#include <queue>
#include <stack>
#include <string>
#include <sys/types.h>
#include <type_traits>
#include <utility>

#define SKIP(x)                                                                \
    case (x):                                                                  \
        if (in[pos] == (x))                                                    \
            pos++;                                                             \
        else                                                                   \
            throw parse_error(                                                 \
                (tokstr[(int)(x)] + std::string(" expected")).c_str());        \
        goto skip

using namespace std;

const shared_ptr<Node> toAST(const shared_ptr<Node> tree, bool quiet)
{
    // copy tree to result
    queue<pair<shared_ptr<Node>, shared_ptr<Node>>> q1;

    shared_ptr<Node> result = make_shared<Node>();
    result->type = tree->type;
    result->parent = weak_ptr<Node>();
    result->self = result;
    result->i = result->children.begin();
    result->data = {};

    q1.push({tree, result});

    while (!q1.empty())
    {
        auto element = q1.front();
        q1.pop();

        for (auto &i : element.first->children)
        {
            element.second->add(i->type);
            element.second->children.back()->data = i->data;
            q1.push({i, element.second->children.back()});
        }
    }
    assert(result->validate());

    // first pass: remove e, parenthesis, semicolon node
    queue<shared_ptr<Node>> q2;
    q2.push(result);
    while (!q2.empty())
    {
        auto element = q2.front();
        q2.pop();

        if (element->type == Tok::e || element->type == Tok::OPA ||
            element->type == Tok::CPA || element->type == Tok::OBR ||
            element->type == Tok::CBR || element->type == Tok::DELIM)
        {
            auto parent = element->parent.lock();
            parent->children.erase(element->i);
        }
        else
        {
            for (auto &i : element->children)
            {
                q2.push(i);
            }
        }
    }
    assert(result->validate());

    // second pass: remove childrenless node
    q2.push(result);
    while (!q2.empty())
    {
        auto element = q2.front();
        q2.pop();

        switch (element->type)
        {
        case Tok::E:
        case Tok::Ep:
        case Tok::F:
        case Tok::T:
        case Tok::Tp:
        case Tok::Xp:
        case Tok::Lp:
        case Tok::PROG:
        case Tok::STMT:
            if (element->children.empty() && element.use_count() >= 2)
            {
                auto parent = element->parent.lock();
                assert(result->validate());

                if (element->i != parent->children.end())
                {
                    parent->children.erase(element->i);
                    element->i = parent->children.end();
                    q2.push(parent);
                }

                break;
            }

        default:
            for (auto &i : element->children)
            {
                q2.push(i);
            }
        }
    }
    assert(result->validate());

    // third pass: elevate only child
    q2.push(result);
    while (!q2.empty())
    {
        auto element = q2.front();
        q2.pop();

        switch (element->type)
        {
        case Tok::E:
        case Tok::Ep:
        case Tok::F:
        case Tok::T:
        case Tok::Tp:
        case Tok::Xp:
        case Tok::Lp:
            if (element->children.size() == 1)
            {
                auto child = *element->children.front();

                element->type = child.type;
                element->children = child.children;
                element->data = child.data;

                for (auto i = element->children.begin();
                     i != element->children.end(); i++)
                {
                    (**i).parent = element;
                    (**i).i = i;
                }

                if (!element->parent.expired())
                {
                    q2.push(element->parent.lock());
                }
            }

        default:
            break;
        }

        for (auto &i : element->children)
        {
            q2.push(i);
        }
    }
    assert(result->validate());

    // fourth pass: flatten addition and multiplication and eqality
    q2.push(result);
    while (!q2.empty())
    {
        auto element = q2.front();
        q2.pop();

        if (element->type == Tok::E || element->type == Tok::T)
        {
            if (element->children.size() == 2)
            {
                element->type = element->type == Tok::E ? Tok::PLUS : Tok::MUL;

                auto Ep = element->children.back();

                while (Ep->children.size() == 3)
                {
                    Ep->type = Ep->children.front()->type;
                    Ep->children.pop_front();
                    element->add(Ep->children.back());
                    Ep->children.pop_back();

                    Ep = element->children.back();
                }

                if (Ep->children.size() == 2)
                {
                    Ep->type = Ep->children.front()->type;
                    Ep->children.pop_front();
                }
            }
        }

        if (element->type == Tok::EXPR &&
            element->children.back()->type == Tok::Xp)
        {
            auto children = move(element->children);
            element->add(Tok::EQ);
            auto eq = element->children.front();

            for (auto &i : children)
            {
                eq->add(i);
            }

            auto arg2 = children.back()->children.back();
            eq->children.pop_back();
            eq->add(arg2);
        }

        for (auto &i : element->children)
        {
            q2.push(i);
        }
    }
    assert(result->validate());

    // fifth path: flatten list, add substitution nodes and PROG
    q2.push(result);
    while (!q2.empty())
    {
        auto element = q2.front();
        q2.pop();

        if (element->type == Tok::LIST && element->children.size() == 2)
        {
            auto elems = move(
                element
                    ->children); // https://stackoverflow.com/questions/12613428/stl-vector-moving-all-elements-of-a-vector

            element->children.push_back(elems.front());
            auto list = elems.back()->children.back();
            while (list->children.size() == 2)
            {
                element->add(list->children.front());
                list = list->children.back()->children.back();
            }
            element->add(list->children.front());

            for (auto i = element->children.begin();
                 i != element->children.end(); i++)
            {
                (**i).parent = element;
                (**i).i = i;
            }
            assert(result->validate());
        }

        if (element->type == Tok::STMT && element->children.size() == 3 &&
            (**next(element->children.begin())).type == Tok::SUBS)
        {
            element->add(Tok::SUBS);
            element->children.erase(next(element->children.begin()));

            for (int i = 0; i < 2; i++)
            {
                auto subs = element->children.back();
                subs->add(element->children.front());
                element->children.pop_front();
            }
            assert(result->validate());
        }

        while (element->type == Tok::PROG &&
               element->children.back()->type == Tok::PROG)
        {
            auto stmts = move(element->children.back()->children);
            element->children.pop_back();

            for (auto &i : stmts)
            {
                element->add(i);
            }
        }

        for (auto &i : element->children)
        {
            q2.push(i);
        }
    }
    assert(result->validate());

    return result;
}

bool clean(const std::shared_ptr<Node> node, bool quiet)
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
    case Tok::EXPR:
        if (node->children.size() == 1)
        {
            if (parent == NULL)
            {
                throw std::runtime_error("Parent of node is already free'd!");
            }

            parent->children.insert(node->i, node->children.front());

            return true;
        }
        else if (node->children.size() % 2 == 1)
        {
            // elevate operator
            auto op = next(node->children.begin());
            node->type = op->get()->type;

            if (node->type == Tok::DIV)
                node->type = Tok::MUL;
            else if (node->type == Tok::MINUS)
                node->type = Tok::PLUS;

            for (auto i = next(node->children.begin());
                 i != node->children.end();)
            {
                auto j = i;
                advance(j, 2);

                if ((*i)->type == Tok::DIV || (*i)->type == Tok::MINUS)
                {
                    (*i)->children.push_back(*next(i));
                    node->children.erase(next(i));
                }
                else
                {
                    node->children.erase(i);
                }

                i = j;
            }

            break;
        }

    case Tok::STMT:
        if (node->children.size() == 1)
        {
            if (node->children.front()->type == Tok::e)
            {
                return true;
            }
        }
        else if (node->children.size() == 2)
        {
            if (node->children.front()->type == Tok::LBL ||
                node->children.front()->type == Tok::GOTO)
            {
                node->type = node->children.front()->type;
                node->children.pop_front();
            }
        }
        else if (node->children.size() == 3)
        {
            if (next(node->children.begin())->get()->type == Tok::SUBS)
            {
                node->type = Tok::SUBS;
                node->children.erase(next(node->children.begin()));
            }
            else if (node->children.front()->type == Tok::IF)
            {
                node->type = Tok::IF;
                node->children.pop_front();
            }
        }
        else if (node->children.empty())
        {
            return true;
        }

        break;

    case Tok::F:
    case Tok::e:
    case Tok::DELIM:
    case Tok::COMMA:
    case Tok::Tp:
    case Tok::Ep:
    case Tok::Xp:
    case Tok::PROG:
    case Tok::Lp:
        // case Tok::LIST:
        // case Tok::T:
        {
            if (parent == NULL || parent->type == Tok::STMT)
                return false;

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
    case Tok::OBR:
    case Tok::CBR:
        return true;

    default:
        break;
    }

    return false;
}

void parse(const std::shared_ptr<Node> tree, const std::vector<Tok> &in,
           const std::vector<Data> &data, Tok starting, bool quiet)
{
    // rules:
    // EXPR -> E Xp

    // Xp -> == E Xp | e

    // E -> T Ep

    // Ep -> + T Ep | - T Ep | e

    // T  -> F Tp

    // Tp -> * F Tp | / F Tp | e

    // F -> (EXPR) | (LIST) | NUM | ID | FUNCALL

    // FUNCALL -> ID ( LIST )

    // statement -> { PROG } | ID SUB EXPR | LBL ID | GOTO ID |
    // IF EXPR statement | FUN ID (LIST) statement |
    // EXPR | e

    // prog -> statement DELIM prog | statement

    // LIST -> EXPR L'

    // L' -> , LIST | e

    deque<Tok> st1;
    deque<Tok> st2;
    st2.push_back(starting);

    auto cur = tree;
    tree->type = starting;
    tree->parent = weak_ptr<Node>();
    tree->self = tree;
    tree->i = tree->children.begin();
    tree->data = {};

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
            case Tok::PROG:
            {
                bool hasDelim = false;

                for (int i = pos; i < in.size(); i++)
                {
                    if (in[i] == Tok::DELIM)
                        hasDelim = true;
                    if (in[i] == Tok::CBR)
                        break;
                }
                if (hasDelim)
                {
                    if (!quiet)
                        cout << "PROG -> STMT PROG" << endl;

                    st1.pop_back();
                    st1.push_back(Tok::STMT);
                    st2.push_front(Tok::PROG);
                    st2.push_front(Tok::DELIM);

                    cur->add(Tok::STMT);
                    cur->add(Tok::DELIM);
                    cur->add(Tok::PROG);

                    cur = cur->children.front();
                }
                else
                {
                    if (!quiet)
                        cout << "PROG -> STMT" << endl;

                    st1.pop_back();
                    st1.push_back(Tok::STMT);
                    cur->add(Tok::STMT);

                    cur = cur->children.front();
                }
            }
            break;

            case Tok::STMT:
                if (pos >= in.size())
                {
                    if (!quiet)
                        cout << "STMT -> e" << endl;
                    st1.pop_back();
                    cur->type = Tok::e;

                    shift = true;

                    break;
                }

                switch (in[pos])
                {
                case Tok::ID:
                    if (in[pos + 1] == Tok::SUBS)
                    {
                        if (!quiet)
                            cout << "STMT -> ID SUBS EXPR" << endl;

                        st1.pop_back();

                        st1.push_back(Tok::ID);
                        st1.push_back(Tok::SUBS);
                        st1.push_back(Tok::EXPR);

                        cur->add(Tok::ID, data[pos]);
                        cur->add(Tok::SUBS);
                        cur->add(Tok::EXPR);

                        cur = cur->children.back();

                        pos += 2;
                    }
                    else
                    {
                        if (!quiet)
                            cout << "STMT -> EXPR" << endl;

                        st1.pop_back();
                        st1.push_back(Tok::EXPR);
                        cur->add(Tok::EXPR);
                        cur = cur->children.back();
                    }

                    break;

                case Tok::LBL:
                    if (!quiet)
                        cout << "STMT -> label ID" << endl;
                    st1.pop_back();
                    st1.push_back(Tok::LBL);
                    st1.push_back(Tok::ID);

                    cur->add(Tok::LBL);
                    cur->add(Tok::ID, data[pos + 1]);
                    cur = cur->children.back();

                    pos += 2;
                    shift = true;

                    break;

                case Tok::GOTO:
                    if (!quiet)
                        cout << "STMT -> GOTO ID" << endl;
                    st1.pop_back();
                    st1.push_back(Tok::GOTO);
                    st1.push_back(Tok::ID);

                    cur->add(Tok::GOTO);
                    cur->add(Tok::ID, data[pos + 1]);
                    cur = cur->children.back();

                    pos += 2;
                    shift = true;

                    break;

                case Tok::IF:
                    if (!quiet)
                        cout << "STMT -> IF expression statement" << endl;
                    st1.pop_back();
                    st1.push_back(Tok::IF);
                    st1.push_back(Tok::EXPR);

                    st2.push_front(Tok::STMT);

                    cur->add(Tok::IF);
                    cur->add(Tok::EXPR);
                    cur->add(Tok::STMT);
                    cur = *next(cur->children.begin());

                    pos += 1;

                    break;

                case Tok::FUN:
                    if (!quiet)
                        cout << "STMT -> FUN ID ( LIST ) statement" << endl;
                    st1.pop_back();
                    st1.push_back(Tok::FUN);
                    st1.push_back(Tok::ID);
                    st1.push_back(Tok::OPA);
                    st1.push_back(Tok::LIST);

                    st2.push_front(Tok::STMT);
                    st2.push_front(Tok::CPA);

                    cur->add(Tok::FUN);
                    cur->add(Tok::ID, data[pos + 1]);
                    cur->add(Tok::OPA);
                    cur->add(Tok::LIST);
                    cur->add(Tok::CPA);
                    cur->add(Tok::STMT);

                    cur = *next(cur->children.begin(), 3);

                    pos += 3;

                    break;

                case Tok::OBR:
                    if (!quiet)
                        cout << "STMT -> { PROG }" << endl;

                    st1.pop_back();
                    st1.push_back(Tok::OBR);
                    st1.push_back(Tok::PROG);
                    st2.push_front(Tok::CBR);

                    cur->add(Tok::OBR);
                    cur->add(Tok::PROG);
                    cur->add(Tok::CBR);

                    cur = *next(cur->children.begin());

                    pos += 1;
                    break;

                case Tok::DELIM:
                case Tok::CBR:
                    if (!quiet)
                        cout << "STMT -> e" << endl;
                    st1.pop_back();
                    cur->add(Tok::e);

                    shift = true;
                    // pos += 1;

                    break;

                default:
                    if (!quiet)
                        cout << "STMT -> EXPR" << endl;
                    st1.pop_back();
                    st1.push_back(Tok::EXPR);
                    cur->add(Tok::EXPR);

                    cur = cur->children.back();
                    break;
                }

                break;

            case Tok::LIST:
                if (!quiet)
                    cout << "LIST -> EXPR L'" << endl;

                st1.pop_back();
                st1.push_back(Tok::EXPR);
                st2.push_front(Tok::Lp);

                cur->add(Tok::EXPR);
                cur->add(Tok::Lp);

                cur = cur->children.front();

                break;

            case Tok::Lp:
                switch (in[pos])
                {
                case Tok::COMMA:
                    if (!quiet)
                        cout << "L' -> , LIST" << endl;

                    st1.pop_back();
                    st1.push_back(Tok::COMMA);
                    st1.push_back(Tok::LIST);

                    cur->add(Tok::COMMA);
                    cur->add(Tok::LIST);

                    cur = *next(cur->children.begin());

                    pos++;
                    break;

                default:
                    if (!quiet)
                        cout << "L' -> e" << endl;

                    st1.pop_back();
                    cur->add(Tok::e);

                    shift = true;
                    break;
                }
                break;

            case Tok::EXPR:
                if (!quiet)
                    cout << "EXPR -> E X'" << endl;
                st1.pop_back();
                st1.push_back(Tok::E);
                st2.push_front(Tok::Xp);

                cur->add(Tok::E);
                cur->add(Tok::Xp);

                cur = cur->children.front();
                break;

            case Tok::Xp:
                switch (in[pos])
                {
                case Tok::EQ:
                    if (!quiet)
                        cout << "X' -> == EXPR X'" << endl;
                    st1.pop_back();
                    st1.push_back(Tok::EQ);
                    st1.push_back(Tok::EXPR);

                    st2.push_front(Tok::Xp);

                    cur->add(Tok::EQ);
                    cur->add(Tok::T);
                    cur->add(Tok::Xp);
                    cur = *next(cur->children.begin());

                    pos++;

                    break;

                default:
                    if (!quiet)
                        cout << "X' -> e" << endl;
                    st1.pop_back();

                    cur->add(Tok::e);

                    shift = true;
                }

                break;

            case Tok::E:
                if (!quiet)
                    cout << "E -> T E'" << endl;
                st1.pop_back();
                st1.push_back(Tok::T);
                st2.push_front(Tok::Ep);

                cur->add(Tok::T);
                cur->add(Tok::Ep);

                cur = cur->children.front();
                break;

            case Tok::T:
                if (!quiet)
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
                    if (!quiet)
                        cout << "F -> NUM" << endl;
                    st1.pop_back();
                    st1.push_back(Tok::NUM);

                    cur->add(Tok::NUM, data[pos]);
                    cur = cur->children.front();

                    shift = true;
                    pos++;

                    break;

                case Tok::OPA:
                {
                    bool expr = true;

                    for (auto i = in.begin() + pos; *i != Tok::CPA; i++)
                    {
                        if (*i == Tok::COMMA)
                        {
                            expr = false;
                            break;
                        }

                        if (i == in.end())
                            throw parse_error("closing parenthesis expected!");
                    }

                    if (expr)
                    {
                        if (!quiet)
                            cout << "F -> ( EXPR )" << endl;
                        st1.pop_back();
                        st1.push_back(Tok::OPA);
                        st1.push_back(Tok::EXPR);
                        st2.push_front(Tok::CPA);

                        cur->add(Tok::OPA);
                        cur->add(Tok::EXPR);
                        cur->add(Tok::CPA);
                        cur = *next(cur->children.begin());

                        pos++;
                    }
                    else
                    {
                        if (!quiet)
                            cout << "F -> ( LIST )" << endl;

                        st1.pop_back();
                        st1.push_back(Tok::OPA);
                        st1.push_back(Tok::LIST);
                        st2.push_front(Tok::CPA);

                        cur->add(Tok::OPA);
                        cur->add(Tok::LIST);
                        cur->add(Tok::CPA);
                        cur = *next(cur->children.begin());

                        pos++;
                    }
                }
                break;

                case Tok::ID:
                    if (in[pos + 1] == Tok::OPA)
                    {
                        if (!quiet)
                        {
                            cout << "F -> FUNCALL" << endl;
                            cout << "FUNCALL -> ID ( LIST )" << endl;
                        }
                        st1.pop_back();
                        st1.push_back(Tok::ID);
                        st2.push_front(Tok::CPA);
                        st2.push_front(Tok::LIST);
                        st2.push_front(Tok::OPA);

                        cur->add(Tok::FUNCALL);
                        cur = cur->children.back();

                        cur->add(Tok::ID, data[pos]);
                        cur->add(Tok::OPA);
                        cur->add(Tok::LIST);
                        cur->add(Tok::CPA);
                        cur = cur->children.front();

                        pos += 2;
                        shift = true;
                    }
                    else
                    {
                        if (!quiet)
                            cout << "F -> ID" << endl;
                        st1.pop_back();
                        st1.push_back(Tok::ID);

                        cur->add(Tok::ID, data[pos]);
                        cur = cur->children.front();

                        pos++;
                        shift = true;
                    }

                    break;

                default:
                    throw parse_error("Number or parenthesis expected!");
                    break;
                }
                break;

            case Tok::Tp:
                switch (in[pos])
                {
                case Tok::MUL:
                    if (!quiet)
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
                    if (!quiet)
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
                    if (!quiet)
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
                    if (!quiet)
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
                    if (!quiet)
                        cout << "E' -> - T E'" << endl;
                    st1.pop_back();
                    st1.push_back(Tok::MINUS);
                    st1.push_back(Tok::T);

                    st2.push_front(Tok::Ep);

                    cur->add(Tok::MINUS);
                    cur->add(Tok::T);
                    cur->add(Tok::Ep);

                    cur = *next(cur->children.begin());

                    pos++;
                    break;

                default:
                    if (!quiet)
                        cout << "E' -> e" << endl;
                    st1.pop_back();

                    cur->type = Tok::e;
                    shift = true;

                    break;
                }
                break;

                SKIP(Tok::CPA);
                SKIP(Tok::CBR);
                SKIP(Tok::COMMA);
                SKIP(Tok::DELIM);

            skip:
            default:
                shift = true;
                break;
            }
        }

        if (shift)
        {
            if (st2.empty())
                break;

            if (!quiet)
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

            if (!quiet)
                cout << tokstr[static_cast<int>(st1.back())] << endl;
        }

        if (!quiet)
        {
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
    }

    if (!quiet)
    {
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
    }
}
