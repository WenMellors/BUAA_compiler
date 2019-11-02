#ifndef SYMBOL_TABLE
#define SYMBOL_TABLE

#include <string>
#include <list>
using namespace std;

struct Symbol {
  Symbol() {}
  string name;
  int type; // 0 or 1, 0: int, 1: char 
  int kind;
  string remark; // 001 -> int int char, indicate func parameter
};

#define ARRAY 0
#define CONST 1
#define VAR 2
#define FUNC 3
#define VOID 4

#define INT 0
#define CHAR 1

#endif // !SYMBOL_TABLE