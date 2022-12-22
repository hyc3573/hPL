#include "lex.hpp"

void lex(std::string input, std::vector<Tok> &in, std::vector<Data> &data,
         bool quiet)
{
    in.clear();
    data.clear();

    using namespace std;

    while (input.size())
    {
        bool hadMatch = false;
        smatch match;

        for (auto &tok : rule)
        {
            auto result = regex_search(input, match, tok.re);
            if (result && match.position() == 0)
            {
                // we have a match!!
                string str = match.str();
                if (!quiet)
                {
                    cout << str << endl;
                    cout << tokstr[static_cast<int>(tok.token)] << endl << endl;
                }
                input = match.suffix();
                in.push_back(tok.token);
                data.push_back({});

                if (tok.token == Tok::NUM)
                {
                    data.back() = stoll(str);
                }
                else if (tok.token == Tok::ID)
                {
                    data.back() = str;
                }

                hadMatch = true;

                break;
            }
        }

        if (regex_search(input, match, strignore) && match.position() == 0)
        {
            input = match.suffix();
            hadMatch = true;
        }

        if (!hadMatch)
        {
            throw lex_error("lex error! invalid token");
        }
    }
}
