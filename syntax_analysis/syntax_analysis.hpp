#ifndef SYNTAX_ANALYSIS
#define SYNTAX_ANALYSIS 

#include "syntax_node.hpp"
enum Status {
  parseConst,
  parseVar,
  parseFunc
};


void syntaxParse(list<struct Lexeme> tokenList);

#endif // !SYNTAX_ANALYSIS