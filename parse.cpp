#include "parse.hpp"
#include "utils.hpp"

using namespace std;

bool clean(const std::shared_ptr<Node> node)
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
        if (node->children.size() == 1)
        {
            if (parent == NULL)
            {
                panik("parent freed!\n");
            }

            parent->children.insert(node->i, node->children.front());

            return true;
        }
        else if (node->children.size() % 2 == 1)
        {
            // elevate operator
            auto op = next(node->children.begin());
            node->type = op->get()->type;

            for (auto i = next(node->children.begin());
                 i != node->children.end();)
            {
                auto j = i;
                advance(j, 2);
                node->children.erase(i);
                i = j;
            }

            break;
        }

    case Tok::F:
    case Tok::e:
    case Tok::Tp:
    case Tok::Ep:
        // case Tok::T:
        {
            if (parent == NULL)
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
        return true;

    default:
        break;
    }

    return false;
}

void parse(const std::shared_ptr<Node> tree, const std::vector<Tok> &in,
           const std::vector<Data> &data)
{
    // rules:
    // E -> T Ep
    // Ep -> + T Ep | - T Ep | e
    // T  -> F Tp
    // Tp -> * F Tp | / F Tp | e
    // F -> (E) | NUM

    deque<Tok> st1;
    deque<Tok> st2;
    st2.push_back(Tok::E);

    auto cur = tree;
    tree->type = Tok::E;
    tree->parent = weak_ptr<Node>();
    tree->self = tree;
    tree->i = tree->children.begin();
    tree->data = 0;

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
}
