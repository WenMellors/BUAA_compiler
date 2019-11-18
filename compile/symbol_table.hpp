#ifndef SYMBOL_TABLE
#define SYMBOL_TABLE

#include <string>
#include <list>
using namespace std;

struct Symbol {
  Symbol() {
    this->regNo = 0;
    this->spOff = 0;
    this->use = 0;
  }
  string name;
  int type; // 0 or 1, 0: int, 1: char 
  int kind;
  string remark; // 001 -> int int char, indicate func parameter, and Array use remark to store length, and const is the value
  int regNo; // 被分配到的寄存器
  int spOff; // 被分配到的 fp 偏移，如果是全局变量的话，就是 gp
  int use;
};

#define ARRAY 0
#define CONST 1
#define VAR 2
#define FUNC 3
#define VOID 4
#define Para 5

#define INT 0
#define CHAR 1

#endif // !SYMBOL_TABLE