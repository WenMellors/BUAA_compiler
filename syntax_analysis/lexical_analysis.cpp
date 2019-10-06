#include "lexical_analysis.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

FILE * in;
struct Lexeme* alexeme;
list<struct Lexeme> tokenList;

void appendLexeme (string name, string value);
int isReserved ();

void parseIdenty (char c);
void parseNum (char c);

list<struct Lexeme> lexcialParse () {
  char c;
  in = fopen("testfile.txt", "r");

  while ((c = fgetc(in)) != EOF) {
    if (isalpha(c) || c == '_') { // IDENFR OR Reserved word
      parseIdenty(c);
    } else if (isdigit(c)) {
      parseNum(c);
    } else if (c == '+') {
      appendLexeme("PLUS", "+");
    } else if (c == '-') {
      appendLexeme("MINU", "-");
    } else if (c == '=') {
      c = fgetc(in);
      if (c == '=') {
        appendLexeme("EQL", "==");
      } else {
        fseek(in, -1, SEEK_CUR);
        appendLexeme("ASSIGN", "=");
      }
    } else if (c == '*') {
      appendLexeme("MULT", "*");
    } else if (c == '/') {
      appendLexeme("DIV", "/");
    } else if (c == '<') {
      c = fgetc(in);
      if (c == '=') {
        appendLexeme("LEQ", "<=");
      } else {
        fseek(in, -1, SEEK_CUR);
        appendLexeme("LSS", "<");
      }
    } else if (c == '>') {
      c = fgetc(in);
      if (c == '=') {
        appendLexeme("GEQ", ">=");
      } else {
        fseek(in, -1, SEEK_CUR);
        appendLexeme("GRE", ">");
      }
    } else if (c == '!') {
      c = fgetc(in); // c must be =
      if (c != '=') {
        exit(-1);
      }
      appendLexeme("NEQ", "!=");
    } else if (c == ';') {
      appendLexeme("SEMICN", ";");
    } else if (c == ',') {
      appendLexeme("COMMA", ",");
    } else if (c == '(') {
      appendLexeme("LPARENT", "(");
    } else if (c == ')') {
      appendLexeme("RPARENT", ")");
    } else if (c == '[') {
      appendLexeme("LBRACK", "[");
    } else if (c == ']') {
      appendLexeme("RBRACK", "]");
    } else if (c == '{') {
      appendLexeme("LBRACE", "{");
    } else if (c == '}') {
      appendLexeme("RBRACE", "}");
    } else if (c == '\'') {
      c = fgetc(in);
      appendLexeme("CHARCON", string(1, c));
      c = fgetc(in); // c = '
    } else if (c == '\"' ) {
      // parse string
      alexeme = new Lexeme;
      while((c = fgetc(in)) != '\"') {
        alexeme->value.push_back(c);
      }
      alexeme->token = "STRCON";
      tokenList.push_back(*alexeme);
    }
  }
  return tokenList;
}

void parseIdenty (char c) {
  alexeme = new Lexeme;
  do {
    alexeme->value.push_back(c);
    c = fgetc(in);
  } while(isalnum(c) || c == '_');
  fseek(in, -1, SEEK_CUR); //ungetchar
  if (isReserved() == 0) {
    alexeme->token = "IDENFR";
    tokenList.push_back(*alexeme);
  }
}

void parseNum (char c) {
  alexeme = new Lexeme;
  do {
    alexeme->value.push_back(c);
    c = fgetc(in);
  } while(isdigit(c));
  fseek(in, -1, SEEK_CUR); //ungetchar
  alexeme->token = "INTCON";
  tokenList.push_back(*alexeme);
}

void appendLexeme (string token, string value) {
  alexeme = new Lexeme;
  alexeme->token = token;
  alexeme->value = value;
  tokenList.push_back(*alexeme);
}

int isReserved () {
  auto token = alexeme->value.data();
  if (strcmp(token, "const") == 0) {
    alexeme->token = "CONSTTK";
  } else if (strcmp(token, "int") == 0) {
    alexeme->token = "INTTK";
  } else if (strcmp(token, "char") == 0) {
    alexeme->token = "CHARTK";
  } else if (strcmp(token, "void") == 0) {
    alexeme->token = "VOIDTK";
  } else if (strcmp(token, "main") == 0) {
    alexeme->token = "MAINTK";
  } else if (strcmp(token, "if") == 0) {
    alexeme->token = "IFTK";
  } else if (strcmp(token, "else") == 0) {
    alexeme->token = "ELSETK";
  } else if (strcmp(token, "do") == 0) {
    alexeme->token = "DOTK";
  } else if (strcmp(token, "while") == 0) {
    alexeme->token = "WHILETK";
  } else if (strcmp(token, "for") == 0) {
    alexeme->token = "FORTK";
  } else if (strcmp(token, "scanf") == 0) {
    alexeme->token = "SCANFTK";
  } else if (strcmp(token, "printf") == 0) {
    alexeme->token = "PRINTFTK";
  } else if (strcmp(token, "return") == 0) {
    alexeme->token = "RETURNTK";
  } else {
    return 0; // not a reserved word
  }
  tokenList.push_back(*alexeme);
  return 1;
}
