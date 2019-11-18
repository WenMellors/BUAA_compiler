#ifndef LEXICAL_ANALYSIS
#define LEXICAL_ANALYSIS

#include <string>
#include <list>
using namespace std;

struct Lexeme{
  Lexeme() {}
  string token;
  string value;
  int lineNumber; // just for print out error.txt
  bool error;
};

list<struct Lexeme> lexcialParse ();

#endif // !LEXICAL_ANALYSIS
