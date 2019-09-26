#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

FILE * in;
FILE * out;
char* token;
int tokenTail = 0;
int tokenLen = 100;

void appendChar (char c);
void resetToken ();
int isReserved ();

void parseIdenty (char c);
void parseNum (char c);

int main () {
  char c;
  in = fopen("testfile.txt", "r");
  out = fopen("output.txt", "w");

  token = calloc(101, sizeof(char)); // token = char[101]

  while ((c = fgetc(in)) != EOF) {
    if (isalpha(c) || c == '_') { // IDENFR OR Reserved word
      parseIdenty(c);
    } else if (isdigit(c)) {
      parseNum(c);
    } else if (c == '+') {
      fprintf(out, "PLUS +\n");
    } else if (c == '-') {
      fprintf(out, "MINU -\n");
    } else if (c == '=') {
      c = fgetc(in);
      if (c == '=') {
        fprintf(out, "EQL ==\n");
      } else {
        fseek(in, -1, SEEK_CUR);
        fprintf(out, "ASSIGN =\n");
      }
    } else if (c == '*') {
      fprintf(out, "MULT *\n");
    } else if (c == '/') {
      fprintf(out, "DIV /\n");
    } else if (c == '<') {
      c = fgetc(in);
      if (c == '=') {
        fprintf(out, "LEQ <=\n");
      } else {
        fseek(in, -1, SEEK_CUR);
        fprintf(out, "LSS <\n");
      }
    } else if (c == '>') {
      c = fgetc(in);
      if (c == '=') {
        fprintf(out, "GEQ >=\n");
      } else {
        fseek(in, -1, SEEK_CUR);
        fprintf(out, "GRE >\n");
      }
    } else if (c == '!') {
      c = fgetc(in); // c must be =
      fprintf(out, "NEQ !=\n");
    } else if (c == ';') {
      fprintf(out, "SEMICN ;\n");
    } else if (c == ',') {
      fprintf(out, "COMMA ,\n");
    } else if (c == '(') {
      fprintf(out, "LPARENT (\n");
    } else if (c == ')') {
      fprintf(out, "RPARENT )\n");
    } else if (c == '[') {
      fprintf(out, "LBRACK [\n");
    } else if (c == ']') {
      fprintf(out, "RBRACK ]\n");
    } else if (c == '{') {
      fprintf(out, "LBRACE {\n");
    } else if (c == '}') {
      fprintf(out, "RBRACE }\n");
    } else if (c == '\'') {
      c = fgetc(in);
      fprintf(out, "CHARCON %c\n", c);
      c = fgetc(in); // c = '
    } else if (c == '\"' ) {
      // parse string
      while((c = fgetc(in)) != '\"') {
        appendChar(c);
      }
      fprintf(out, "STRCON %s\n", token);
      resetToken();
    }
  }
  free(token);
}

void parseIdenty (char c) {
  do {
    appendChar(c);
    c = fgetc(in);
  } while(isalnum(c) || c == '_');
  fseek(in, -1, SEEK_CUR); //ungetchar
  if (isReserved() == 0) {
    fprintf(out, "IDENFR %s\n", token);
  }
  resetToken();
}

void parseNum (char c) {
  do {
    appendChar(c);
    c = fgetc(in);
  } while(isdigit(c));
  fseek(in, -1, SEEK_CUR); //ungetchar
  fprintf(out, "INTCON %s\n", token);
  resetToken();
}

void appendChar (char c) {
  if (strlen(token) == tokenLen) {
    token = (char *)realloc(token, tokenLen*10 + 1); // 1 is reserved for '\0'
    tokenLen *= 10;
  }
  token[tokenTail] = c;
  token[tokenTail + 1] = '\0'; // realloc is not initialized
  tokenTail++;
}

void resetToken () {
  memset(token, '\0', tokenTail);
  tokenTail = 0;
}

int isReserved () {
  if (strcmp(token, "const") == 0) {
    fprintf(out, "CONSTTK const\n");
  } else if (strcmp(token, "int") == 0) {
    fprintf(out, "INTTK int\n");
  } else if (strcmp(token, "char") == 0) {
    fprintf(out, "CHARTK char\n");
  } else if (strcmp(token, "void") == 0) {
    fprintf(out, "VOIDTK void\n");
  } else if (strcmp(token, "main") == 0) {
    fprintf(out, "MAINTK main\n");
  } else if (strcmp(token, "if") == 0) {
    fprintf(out, "IFTK if\n");
  } else if (strcmp(token, "else") == 0) {
    fprintf(out, "ELSETK else\n");
  } else if (strcmp(token, "do") == 0) {
    fprintf(out, "DOTK do\n");
  } else if (strcmp(token, "while") == 0) {
    fprintf(out, "WHILETK while\n");
  } else if (strcmp(token, "for") == 0) {
    fprintf(out, "FORTK for\n");
  } else if (strcmp(token, "scanf") == 0) {
    fprintf(out, "SCANFTK scanf\n");
  } else if (strcmp(token, "printf") == 0) {
    fprintf(out, "PRINTFTK printf\n");
  } else if (strcmp(token, "return") == 0) {
    fprintf(out, "RETURNTK return\n");
  } else {
    return 0; // not a reserved word
  }
  return 1;
}
