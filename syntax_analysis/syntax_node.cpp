#include "syntax_node.hpp"

SyntaxNode::SyntaxNode(string value, string value2, string token) {
  syntaxValue = value;
  lexicalValue = value2;
  lexicalToken = token;
}

void SyntaxNode::appendChild(SyntaxNode child) {
  children.push_back(child);
}

bool SyntaxNode::isLeaf() {
  return children.empty();
}

string SyntaxNode::getSyntaxValue() {
  return syntaxValue;
}

string SyntaxNode::getLexicalValue() {
  return lexicalValue;
}

string SyntaxNode::getLexicalToken() {
  return lexicalToken;
}

list<SyntaxNode> SyntaxNode::getChildren() {
  return children;
}