#include "lexical_analysis.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

FILE * in;
int line;
struct Lexeme* alexeme;
list<struct Lexeme> tokenList;

void appendLexeme (string name, string value, bool error);
int isReserved ();

void parseIdenty (char c);
void parseNum (char c);

list<struct Lexeme> lexcialParse () {
  char c;
  in = fopen("testfile.txt", "r");
  line = 1;
  while ((c = fgetc(in)) != EOF) {
    if (isalpha(c) || c == '_') { // IDENFR OR Reserved word
      parseIdenty(c);
    } else if (isdigit(c)) {
      parseNum(c);
    } else if (c == '+') {
      appendLexeme("PLUS", "+", false);
    } else if (c == '-') {
      appendLexeme("MINU", "-", false);
    } else if (c == '=') {
      c = fgetc(in);
      if (c == '=') {
        appendLexeme("EQL", "==", false);
      } else {
        fseek(in, -1, SEEK_CUR);
        appendLexeme("ASSIGN", "=", false);
      }
    } else if (c == '*') {
      appendLexeme("MULT", "*", false);
    } else if (c == '/') {
      appendLexeme("DIV", "/", false);
    } else if (c == '<') {
      c = fgetc(in);
      if (c == '=') {
        appendLexeme("LEQ", "<=", false);
      } else {
        fseek(in, -1, SEEK_CUR);
        appendLexeme("LSS", "<", false);
      }
    } else if (c == '>') {
      c = fgetc(in);
      if (c == '=') {
        appendLexeme("GEQ", ">=", false);
      } else {
        fseek(in, -1, SEEK_CUR);
        appendLexeme("GRE", ">", false);
      }
    } else if (c == '!') {
      c = fgetc(in); // c must be =
      if (c != '=') { // I think will not happen
        appendLexeme("NEQ", "!", true);
      }
      appendLexeme("NEQ", "!=", false);
    } else if (c == ';') {
      appendLexeme("SEMICN", ";", false);
    } else if (c == ',') {
      appendLexeme("COMMA", ",", false);
    } else if (c == '(') {
      appendLexeme("LPARENT", "(", false);
    } else if (c == ')') {
      appendLexeme("RPARENT", ")", false);
    } else if (c == '[') {
      appendLexeme("LBRACK", "[", false);
    } else if (c == ']') {
      appendLexeme("RBRACK", "]", false);
    } else if (c == '{') {
      appendLexeme("LBRACE", "{", false);
    } else if (c == '}') {
      appendLexeme("RBRACE", "}", false);
    } else if (c == '\'') {
      c = fgetc(in);
      if (c != '+' && c != '-' && c != '*' && c != '/' && !isalnum(c)) {
        appendLexeme("CHARCON", string(1, c), true);
      }
      else {
        appendLexeme("CHARCON", string(1, c), false);
      }
      c = fgetc(in); // c = ' it's a problem
    } else if (c == '\"' ) {
      // parse string
      alexeme = new Lexeme;
      alexeme->error = false;
      while((c = fgetc(in)) != '\"') {
        if (c != 32 && c != 33 && c < 35 && c > 126) {
          alexeme->error = true;
        }
        alexeme->value.push_back(c);
      }
      alexeme->token = "STRCON";
      alexeme->lineNumber = line;
      tokenList.push_back(*alexeme);
    } else if (c == '\n') {
      line += 1;
    } // suppose will not happen
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
    alexeme->error = false;
    alexeme->lineNumber = line;
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
  alexeme->error = false;
  alexeme->lineNumber = line;
  tokenList.push_back(*alexeme);
}

void appendLexeme (string token, string value, bool error) {
  alexeme = new Lexeme;
  alexeme->error = error;
  alexeme->token = token;
  alexeme->value = value;
  alexeme->lineNumber = line;
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
  alexeme->error = false;
  alexeme->lineNumber = line;
  tokenList.push_back(*alexeme);
  return 1;
}
