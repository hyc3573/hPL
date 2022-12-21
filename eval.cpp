#include "eval.hpp"
#include <variant>

using namespace std;

Data evaluate(shared_ptr<Node> AST, Context& context)
{
    switch (AST->type)
    {
    case Tok::EXPR:
        return evaluate(AST->children.front(), context);
        break;

    case Tok::NUM:
        return AST->data;
        break;

    case Tok::MUL:
    {
        int acc = get<int>(evaluate(AST->children.front(), context));
        for (auto i = next(AST->children.begin());
             i != AST->children.end();
             i++)
        {
            if ((**i).type == Tok::MUL)
                acc *= get<int>(
                    evaluate((**i).children.front(),
                             context));
            if ((**i).type == Tok::DIV)
                acc *= get<int>(
                    evaluate((**i).children.front(),
                             context));
        }

        return acc;
        
        break;
    }

    case Tok::PLUS:
    {
        int acc = get<int>(evaluate(AST->children.front(), context));
        for (auto i = next(AST->children.begin());
             i != AST->children.end();
             i++)
        {
            if ((**i).type == Tok::PLUS)
                acc += get<int>(
                    evaluate((**i).children.front(),
                             context));
            if ((**i).type == Tok::MINUS)
                acc -= get<int>(
                    evaluate((**i).children.front(),
                             context));
        }

        return acc;
        
        break;
    }

    default:
        return {};
    }
}
