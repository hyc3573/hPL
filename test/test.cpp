#include <memory>
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE hPLTEST
#include <boost/test/tools/output_test_stream.hpp>
#include <boost/test/unit_test.hpp>

#include "../lex.hpp"
#include "../parse.hpp"
#include "../utils.hpp"
#include "../eval.hpp"
#include <iostream>
#include <vector>
using namespace std;
using boost::test_tools::output_test_stream;

BOOST_AUTO_TEST_CASE(hPLTEST)
{
    vector<Tok> in;
    vector<Data> data;

    vector<Tok> expectedin = {Tok::PLUS, Tok::MINUS, Tok::MUL, Tok::DIV,
                              Tok::OPA,  Tok::CPA,   Tok::NUM};

    lex(" \n\t+-*/()123", in, data);

    BOOST_CHECK_EQUAL_COLLECTIONS(in.begin(), in.end(), expectedin.begin(),
                                  expectedin.end());

    lex("1+1*3/6+(1-2)", in, data);
    BOOST_CHECK_EQUAL(in.size(), 13);

    shared_ptr<Node> tree = make_shared<Node>();
    parse(tree, in, data, Tok::E);
    clean(tree);

    printNode(tree, 0);

    output_test_stream os;
    printNodeOs(os, tree, 0);

    string expected = R"(+
|   NUM: 1
|   *
|   |   NUM: 1
|   |   NUM: 3
|   |   /
|   |   |   NUM: 6
|   +
|   |   NUM: 1
|   |   -
|   |   |   NUM: 2
)";

    BOOST_CHECK(os.is_equal(expected, false));

    expectedin = {Tok::ID, Tok::SUBS, Tok::NUM, Tok::PLUS, Tok::ID};

    lex("id=3+d", in, data);

    BOOST_CHECK_EQUAL_COLLECTIONS(in.begin(), in.end(), expectedin.begin(),
                                  expectedin.end());

    tree = make_shared<Node>();
    parse(tree, in, data, Tok::PROG);
    clean(tree);

    printNode(tree, 0);

    expected = R"(program
|   =
|   |   ID: id
|   |   +
|   |   |   NUM: 3
|   |   |   ID: d
)";
    os.flush();
    printNodeOs(os, tree, 0);
    BOOST_CHECK(os.is_equal(expected, false));

    expectedin = {Tok::ID,   Tok::SUBS,  Tok::ID,    Tok::DELIM, Tok::ID,
                  Tok::SUBS, Tok::NUM,   Tok::DELIM, Tok::ID,    Tok::SUBS,
                  Tok::NUM,  Tok::DELIM, Tok::DELIM};

    lex("id=id;id=10;id=100;;", in, data);

    BOOST_CHECK_EQUAL_COLLECTIONS(in.begin(), in.end(), expectedin.begin(),
                                  expectedin.end());

    tree = make_shared<Node>();
    parse(tree, in, data, Tok::PROG);
    clean(tree);

    os.flush();
    printNodeOs(os, tree, 0);

    expected = R"(program
|   =
|   |   ID: id
|   |   ID: id
|   =
|   |   ID: id
|   |   NUM: 10
|   =
|   |   ID: id
|   |   NUM: 100
)";
    BOOST_CHECK(os.is_equal(expected, false));

    expectedin = {Tok::LBL, Tok::ID, Tok::DELIM, Tok::GOTO, Tok::ID, Tok::DELIM, Tok::IF, Tok::NUM, Tok::EQ, Tok::NUM, Tok::GOTO, Tok::ID, Tok::DELIM};
    lex("label a;goto a;if 1 == 1 goto a;", in, data);
    for (auto &i : in)
        cout << i << " ";
    cout << endl;
    BOOST_CHECK_EQUAL_COLLECTIONS(expectedin.begin(), expectedin.end(), in.begin(), in.end());

    tree = make_shared<Node>();
    parse(tree, in, data, Tok::PROG);
    clean(tree);

    printNode(tree, 0);

    os.flush();
    printNodeOs(os, tree, 0);

    expected = R"(program
|   label
|   |   ID: a
|   goto
|   |   ID: a
|   if
|   |   ==
|   |   |   NUM: 1
|   |   |   NUM: 1
|   |   goto
|   |   |   ID: a
)";
    BOOST_CHECK(os.is_equal(expected, false));

    expectedin = {
        Tok::OBR,
        Tok::ID,
        Tok::SUBS,
        Tok::ID,
        Tok::CBR
    };
    lex("{a=a}", in, data);  
    BOOST_CHECK_EQUAL_COLLECTIONS(expectedin.begin(), expectedin.end(), in.begin(), in.end());

    tree = make_shared<Node>();
    parse(tree, in, data, Tok::PROG);
    clean(tree);
    printNode(tree, 0);

    lex(R"(if asdf == 5
{
    asdf = 10;
})", in, data);
    tree = make_shared<Node>();
    parse(tree, in, data, Tok::PROG);
    clean(tree);
    printNode(tree, 0);

    lex("fun asdf (a,b,c,d) {a(12,34);b=1+a(34,56+78-10*10/10,10*10==10)}", in, data);
    for (auto &i : in)
    {
        cout << i << endl;
    }

    tree = make_shared<Node>();
    parse(tree, in, data, Tok::PROG);
    printNode(tree, 0);
    auto ast = toAST(tree, false);
    printNode(ast, 0);
    cout << ast->validate() << endl;

    lex("1*(5+2)*3", in, data);
    tree = make_shared<Node>();
    parse(tree, in, data, Tok::EXPR);
    ast = toAST(tree, false);
    printNode(ast, 0);

    Context context;
    cout << get<int>(evaluate(ast, context)) << endl;
}
