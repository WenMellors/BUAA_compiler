#include <string>
#include "lexical_analysis.hpp"
#include "syntax_analysis.hpp"
using namespace std;

int main () {
  list<struct Lexeme> tokenList;
  tokenList = lexcialParse();
  syntaxParse(tokenList);
  return 1;
}