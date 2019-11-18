#ifndef SYNTAX_ANALYSIS
#define SYNTAX_ANALYSIS 

#include "syntax_node.hpp"
#include "symbol_table.hpp"
#include <map>
enum Status {
  parseConst,
  parseVar,
  parseFunc
};


void syntaxParse(list<struct Lexeme> tokenList);
Symbol* lookTable(string name, int level);

extern map<string, list<Symbol*>> symbolMap;
extern map<int, string> strMap;
extern int strCnt;
extern list<Symbol*> symbolTable[2];
#endif // !SYNTAX_ANALYSIS