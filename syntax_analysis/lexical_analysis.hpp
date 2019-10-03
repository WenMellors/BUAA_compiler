#ifndef LEXICAL_ANALYSIS
#define LEXICAL_ANALYSIS

#include <string>
#include <list>
using namespace std;

struct Lexeme{
  string token;
  string value;
};

list<struct Lexeme> lexcialParse ();

#endif // !LEXICAL_ANALYSIS
