#include <string>
#include "lexical_analysis.hpp"
using namespace std;

int main () {
  list<struct Lexeme> tokenList;
  tokenList = lexcialParse();
  return 1;
}