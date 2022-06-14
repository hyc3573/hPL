#include <boost/test/tools/old/interface.hpp>
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE hPLTEST
#include <boost/test/unit_test.hpp>
#include <boost/test/tools/output_test_stream.hpp>

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

    BOOST_CHECK_THROW(lex("saf;ldk", in, data), lex_error);

    vector<Tok> expectedin = {Tok::PLUS, Tok::MINUS, Tok::MUL, Tok::DIV, Tok::OPA, Tok::CPA, Tok::NUM};

    lex(" \n\t+-*/()123", in, data);

    BOOST_CHECK_EQUAL_COLLECTIONS(in.begin(), in.end(), expectedin.begin(),
                                  expectedin.end());

    lex("1+1*3/6+(1-2)", in, data);
    BOOST_CHECK_EQUAL(in.size(), 13);

    shared_ptr<Node> tree = make_shared<Node>();
    parse(tree, in, data);
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
}
