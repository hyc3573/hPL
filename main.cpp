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
#include "lex.hpp"
#include "node.hpp"
#include "parse.hpp"

using namespace std;

int main()
{
    // lexer
    cout << "lexing:" << endl << endl;
    
    string input = "1+3*3+(4+5+6)";

    vector<Tok> in;
    vector<Data> data;

    lex(input, in ,data);
    
    cout << "----------------------------------" << endl;
    cout << "parsing:" << endl << endl;

    // parser

    
    if (in.size() == 0)
        panik("empty input!\n");

    shared_ptr<Node> tree = make_shared<Node>();

    parse(tree, in, data, Tok::E);

    clean(tree);

    printNode(tree, 0);
}
