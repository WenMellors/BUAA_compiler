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
    this->weight = 0;
    this->distributed = false; // indicate the var be distributed a reg or not
    this->disable = false;
    this->disableReg = 0;
    this->disableTimes = 0; // 用于嵌套标记一个变量被 disable 了几次
  }
  string name;
  int type; // 0 or 1, 0: int, 1: char 
  int kind;
  string remark; // 001 -> int int char, indicate func parameter, and Array use remark to store length, and const is the value
  int regNo; // 被分配到的寄存器
  bool distributed;
  bool disable; // 函数调用时，寄存器会被用到的局部变量理应被换出，那么就不能通过该寄存器来访问局部变量了
  int disableReg;
  int disableTimes;
  int spOff; // 被分配到的 fp 偏移，如果是全局变量的话，就是 gp
  int use;
  int weight;
};

#define ARRAY 0
#define CONST 1
#define VAR 2
#define FUNC 3
#define VOID 4
#define Para 5

#define CIRCLE_WEIGHT 100
#define WEIGHT 1 

#define INT 0
#define CHAR 1

#endif // !SYMBOL_TABLE