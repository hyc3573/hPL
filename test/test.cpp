#include <boost/test/tools/old/interface.hpp>
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE hPLTEST
#include <boost/test/unit_test.hpp>

#include "../lex.hpp"
#include "../parse.hpp"
#include "../utils.hpp"
#include <iostream>
#include <vector>

using namespace std;

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
}
