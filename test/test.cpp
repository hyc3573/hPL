#include <boost/test/tools/old/interface.hpp>
#include <memory>
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE hPLTEST
#include <boost/test/tools/output_test_stream.hpp>
#include <boost/test/unit_test.hpp>

#include "../lex.hpp"
#include "../parse.hpp"
#include "../utils.hpp"
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

    string expected = R"(+: 0
|   NUM: 1
|   *: 0
|   |   NUM: 1
|   |   NUM: 3
|   |   /: 0
|   |   |   NUM: 6
|   +: 0
|   |   NUM: 1
|   |   -: 0
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

    expected = R"(program: 0
|   =: 0
|   |   ID: 0
|   |   ID: 1
|   =: 0
|   |   ID: 2
|   |   NUM: 10
|   =: 0
|   |   ID: 3
|   |   NUM: 100
)";
    BOOST_CHECK(os.is_equal(expected, false));
}
