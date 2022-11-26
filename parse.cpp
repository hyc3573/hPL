#include "parse.hpp"
#include "utils.hpp"

#include <exception>
#include <iterator>
#include <string>
#include <sys/types.h>

#define SKIP(x)                                                                \
    case (x):                                                                  \
        if (in[pos] == (x))                                                    \
            pos++;                                                             \
        else                                                                   \
            throw parse_error(                                                 \
                (tokstr[(int)(x)] + std::string(" expected")).c_str());        \
        goto skip

using namespace std;

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
    case Tok::Tp:
    case Tok::Ep:
    case Tok::Xp:
    case Tok::PROG:
    case Tok::Lp:
    case Tok::LIST:
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
    // F -> (EXPR) | (LIST) | NUM | ID | ID ( LIST )

    // statement -> { PROG } | ID SUB EXPR | LBL ID | GOTO ID | IF EXPR statement | FUN ID (LIST) statement | EXPR
    // statement | e prog -> statement DELIM prog | statement LIST -> EXPR COMMA
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
                    if (in[i] == Tok::DELIM)
                        hasDelim = true;

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
                    if (in[pos+1] == Tok::SUBS)
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
                    cur->add(Tok::ID, data[pos+1]);
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
                    pos += 1;

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
                    if (in[pos+1] == Tok::OPA)
                    {
                        if (!quiet)
                            cout << "F -> ID ( LIST )" << endl;

                        st1.pop_back();
                        st1.push_back(Tok::ID);
                        st2.push_front(Tok::CPA);
                        st2.push_front(Tok::LIST);
                        st2.push_front(Tok::OPA);

                        cur->add(Tok::ID, data[pos]);
                        cur->add(Tok::OPA);
                        cur->add(Tok::LIST);
                        cur->add(Tok::CPA);
                        cur = cur->children.front();

                        pos+=2;
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
