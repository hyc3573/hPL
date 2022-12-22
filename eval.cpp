#include "eval.hpp"
#include <exception>
#include <variant>

using namespace std;

Context::Context() : goto_flag(false)
{
    stack.push_back({});
}

Data Context::get(std::string id)
{
    for (auto i = stack.rbegin(); i != stack.rend(); i++)
    {
        if (i->count(id))
        {
            return (*i)[id];
        }
    }

    throw std::runtime_error("니얼굴");
}

void Context::set(std::string id, Data value)
{
    for (auto i = stack.rbegin(); i != stack.rend(); i++)
    {
        if (i->count(id))
        {
            (*i)[id] = value;
            return;
        }
    }

    stack.back().insert({id, value});
}

void Context::push()
{
    stack.push_back({});
}

void Context::pop()
{
    stack.pop_back();
}

Data evaluate(shared_ptr<Node> AST, Context &context)
{
    switch (AST->type)
    {
    case Tok::STMT:
    {
        switch (AST->children.front()->type)
        {
        case Tok::SUBS:
        {
            auto node = AST->children.front();
            auto id = node->children.front();
            auto expr = node->children.back();

            context.set(get<string>(id->data), evaluate(expr, context));
        }
        break;

        case Tok::IF:
        {
            int result =
                get<long long int>(evaluate(*next(AST->children.begin()), context));

            if (result)
            {
                evaluate(AST->children.back(), context);
            }
        }
        break;

        case Tok::LBL:
            return {};

        case Tok::GOTO:
            context.goto_flag = true;
            context.goto_label = get<string>(AST->children.back()->data);
            return {};

        case Tok::PROG:
        case Tok::STMT:
            evaluate(AST->children.front(), context);
            return {};
            break;            

        default:
            break;
        }
    }

        return {};
        break;

    case Tok::EXPR:
        return evaluate(AST->children.front(), context);
        break;

    case Tok::NUM:
        return AST->data;
        break;

    case Tok::ID:
        return context.get(get<string>(AST->data));

    case Tok::MUL:
    {
        int acc = get<long long int>(evaluate(AST->children.front(), context));
        for (auto i = next(AST->children.begin()); i != AST->children.end();
             i++)
        {
            if ((**i).type == Tok::MUL)
                acc *= get<long long int>(evaluate((**i).children.front(), context));
            if ((**i).type == Tok::DIV)
                acc /= get<long long int>(evaluate((**i).children.front(), context));
        }

        return acc;

        break;
    }

    case Tok::PLUS:
    {
        int acc = get<long long int>(evaluate(AST->children.front(), context));
        for (auto i = next(AST->children.begin()); i != AST->children.end();
             i++)
        {
            if ((**i).type == Tok::PLUS)
                acc += get<long long int>(evaluate((**i).children.front(), context));
            if ((**i).type == Tok::MINUS)
                acc -= get<long long int>(evaluate((**i).children.front(), context));
        }

        return acc;

        break;
    }

    case Tok::EQ:
        return evaluate(AST->children.front(), context) ==
               evaluate(AST->children.back(), context);

    case Tok::PROG:
    {
        context.push();

        for (auto i = AST->children.begin(); i != AST->children.end();)
        {
            evaluate(*i, context);

            if (context.goto_flag)
            {
                for (auto j = AST->children.begin(); j != AST->children.end();
                     j++)
                {
                    if ((**j).children.front()->type == Tok::LBL &&
                        get<string>((**j).children.back()->data) ==
                            context.goto_label)
                    {
                        i = j;
                    }
                }
                context.goto_flag = false;
            }
            else
            {
                i++;
            }
        }

        context.pop();
    }

    default:
        return {};
    }
}
