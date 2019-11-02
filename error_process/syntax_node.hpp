#ifndef SYNTAX_NODE
#define SYNTAX_NODE

#include <string>
#include <list>
#include "lexical_analysis.hpp"
using namespace std;

class SyntaxNode
{
private:
  string syntaxValue;
  string lexicalValue; // if not leaf node, should be null
  string lexicalToken;
  list<SyntaxNode*> children;
public:
  SyntaxNode(string value, string value2, string token);
  void appendChild(SyntaxNode* child);
  bool isLeaf();
  string getSyntaxValue();
  string getLexicalValue();
  string getLexicalToken();
  list<SyntaxNode*> getChildren();
};


#endif // !SYNTAX_NODE

