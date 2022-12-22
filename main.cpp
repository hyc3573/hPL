#include "eval.hpp"
#include "lex.hpp"
#include "node.hpp"
#include "parse.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>

using namespace std;

int main(int argc, char *argv[])
{
    if (argc == 2)
    {
        ifstream ifs(argv[1]);
        string content((istreambuf_iterator<char>(ifs)),
                       istreambuf_iterator<char>());

        vector<Tok> in;
        vector<Data> data;

        lex(content, in, data);

        shared_ptr<Node> tree;
        tree = make_shared<Node>();
        parse(tree, in, data, Tok::PROG, false);
        auto ast = toAST(tree);
        printNode(ast, 0);

        Context context;
        context.set("result", -1);
        int input;
        cout << "input: ";
        cin >> input;
        context.set("input", input);

        evaluate(ast, context);

        cout << get<long long int>(context.get("result")) << endl;
    }
}
