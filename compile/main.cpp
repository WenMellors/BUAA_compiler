#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include "lexical_analysis.hpp"
#include "syntax_analysis.hpp"
using namespace std;

int regUse[18]; // + 8 使用
int nextReg;
map<string, int> midVar; // 存放中间变量被分配到的寄存器, 如果对应值为负，则为其在 DM 的偏移的负数
map<string, int> midUse; // 正在使用中的中间变量，不可以替换
int dmOff;
FILE* mips;
void genMips();
void initData();
void initVar(int level);
void blockAnalysis();

int main () {
  list<struct Lexeme> tokenList;
  tokenList = lexcialParse();
  syntaxParse(tokenList);
  // blockAnalysis();
  mips = fopen("mips.txt", "w");
  initData();
  fprintf(mips, ".text\n");
  fprintf(mips, "la $k0, d\n");
  fprintf(mips, "move $gp, $sp\n");
  symbolTable[0] = symbolMap.at("global"); // load global Table
  initVar(0);
  fprintf(mips, "move $fp, $sp\n"); // 初始化栈帧
  fprintf(mips, "jal main\n"); // 跳转到 main 函数
  fprintf(mips, "li $v0, 10\n");
  fprintf(mips, "syscall\n"); // 结束进程
  genMips();
  return 0;
}

void initData() {
  fprintf(mips, ".data\n");
  for (int i = 1; i <= strCnt; i++) {
    fprintf(mips, "str%d: .asciiz \"%s\"\n", i, strMap.at(i).data());
  }
  fprintf(mips, "d: .align 2\n");
}

// 为函数内的变量分配地址
void initVar(int level) {
  // 并不是所有的 symbol 都是变量
  int length = 0;
  int offset = -4;
  list<Symbol*>::iterator iter = symbolTable[level].begin();
  while(iter != symbolTable[level].end()) {
    if ((*iter)->kind == VAR) {
      (*iter)->spOff = offset; // 标记每个变量到 fp 的偏移，如果是全局变量就是 gp
      offset -= 4;
      length++;
    } else if ((*iter)->kind == Para) {
      // 形参的 sp 由调用方减去，但是这里得记下他的偏移
      (*iter)->spOff = offset;
      offset -= 4;
    } else if ((*iter)->kind == ARRAY) {
      // 坚信你不会爆栈
      int arrSize = stoi((*iter)->remark);
      offset -= 4*(arrSize - 1);
      (*iter)->spOff = offset;
      offset -=4;
      length += arrSize;
    }
    iter++;
  }
  if (length != 0) {
    fprintf(mips, "addi $sp, $sp, -%d\n", length*4); // sp 自减
  } 
}

vector<string> split(string s , char delim) {
  vector<string> sv;
  istringstream iss(s);
  string temp;
  
  while (getline(iss, temp, delim)) {
    sv.push_back(temp);
  }

  return sv;
}

bool isNum(string s) {
  for (int i = 0; i < s.length(); i++) {
    if (!isdigit(s[i])) {
      if (i == 0 && (s[i] == '+' || s[i] == '-')) {
        continue;
      }
      return false;
    }
  }
  return true;
}

bool isChar(string s) {
  return s.length() == 3 && s[0] == '\'' && s[2] == '\'';
}

bool isMid(string s) {
  return isNum(s.substr(0, s.length() - 1)) && s.back() == 't';
}

int findNextReg() {
  for (int i = 0; i < 18; i++) {
    if (regUse[nextReg] == 0) {
      // 找到一个未使用的寄存器，棒！
      regUse[nextReg] = 1;
      int res = nextReg + 8; // 要加 8 位偏移
      if (nextReg == 17) {
        nextReg = 0;
      } else {
        nextReg++;
      }
      return res;
    } else {
      if (nextReg == 17) {
        nextReg = 0;
      } else {
        nextReg++;
      }
    }
  }
  // 找了一圈发现没有未使用的寄存器，策略：先将全局变量移出寄存器堆，再将局部变量，最后是中间变量, 不可以把正在使用的寄存器换出去
  for(list<Symbol*>::iterator iter = symbolTable[0].begin(); iter != symbolTable[0].end(); iter++) {
    printf("there is global var in reg! %s", (*iter)->name.data());
    if ((*iter)->regNo != 0 && (*iter)->use == 0) {
      // 将其换出
      int res = (*iter)->regNo; // regNo 已经 + 8
      fprintf(mips, "sw $%d, %d($gp)\n", res, (*iter)->spOff); // 存到 gp
      (*iter)->regNo = 0;
      return res;
    }
  }
  for(list<Symbol*>::iterator iter = symbolTable[1].begin(); iter != symbolTable[1].end(); iter++) {
    printf("there is local var in reg! %s", (*iter)->name.data());
    if ((*iter)->regNo != 0 && (*iter)->use == 0) {
      // 将其换出
      int res = (*iter)->regNo; // regNo 已经 + 8
      fprintf(mips, "sw $%d, %d($fp)\n", res, (*iter)->spOff); // 存到 fp
      (*iter)->regNo = 0;
      return res;
    }
  }
  for(map<string, int>::iterator iter = midVar.begin(); iter != midVar.end(); iter++) {
    if (iter->second > 0 && midUse.find(iter->first) == midUse.end()) {
      // 小于 0 表示在 DM 里面，不可能为 0，且该中间变量没有正在被使用
      int res = iter->second;
      dmOff += 4;
      iter->second = -dmOff;
      fprintf(mips, "sw $%d, %d($k0)\n", res, dmOff);
      return res;
    }
  }
  // cannot be this
  printf("not enough reg\n");
  exit(0);
  return -1;
}

bool isConst(string name) {
  Symbol* sym = lookTable(name, 1);
  if (sym == NULL) {
    sym = lookTable(name, 0);
    if (sym == NULL) {
      return false;
    }
  }
  if (sym->kind == CONST) {
    return true;
  } else {
    return false;
  }
}

int getConst(string name) {
  Symbol* sym = lookTable(name, 1);
  if (sym == NULL) {
    sym = lookTable(name, 0);
    if (sym == NULL) {
      printf("not find symbol\n");
      exit(0);
    }
  }
  if (sym->kind == CONST) {
    if (isChar(sym->remark)) {
      return sym->remark[1];
    } else {
      return stoi(sym->remark);
    }
  } else {
    return -1;
  }
}

int load(string name) {
  // 检查是否装载，如果没有的话，就装载
  int level = 1;
  Symbol* sym = lookTable(name, 1);
  if (sym == NULL) {
    level = 0;
    sym = lookTable(name, 0);
    if (sym == NULL) {
      printf("not find symbol\n");
      exit(0);
    }
  }
  if (sym->regNo != 0) { //TODO: 初始应该是 0
    // 已经被加载
    return sym->regNo;
  } else {
    // 要加载
    int reg = findNextReg();
    if (reg == -1) {
      printf("no enough reg\n");
      exit(0);
    }
    if (level == 0) {
      // 全局变量
      fprintf(mips, "lw $%d, %d($gp)\n", reg, sym->spOff);
    } else {
      // 局部变量
      fprintf(mips, "lw $%d, %d($fp)\n", reg, sym->spOff);
    }
    sym->regNo = reg;
    return reg;
  }
}

void pop(string name, bool rewrite) {
  int level = 1;
  Symbol* sym = lookTable(name, 1);
  if (sym == NULL) {
    level = 0;
    sym = lookTable(name, 0);
    if (sym == NULL) {
      printf("not find symbol\n");
      exit(0);
    }
  }
  if (sym->regNo != 0) {
    // 将变量换出寄存器堆
    if (rewrite) { // 如果修改了就写回栈里
      if (level == 0) {
        // 全局变量
        fprintf(mips, "sw $%d, %d($gp)\n", sym->regNo, sym->spOff);
      } else {
        // 局部变量
        fprintf(mips, "sw $%d, %d($fp)\n", sym->regNo, sym->spOff);
      }
    }
    regUse[sym->regNo - 8] = 0;
    sym->regNo = 0;
    sym->use = 0;
  }
}

int loadMid(string name) {
  // 如果中间变量在寄存堆里面，则直接返回
  if (midVar.find(name) == midVar.end()) {
    // 没有这个中间变量，它要么没加载，要么已经出去了
    printf("not found mid var\n");
    exit(0);
  }
  if (midVar.at(name) > 0 ) {
    return midVar.at(name);
  } else {
    int reg = findNextReg();
    if (reg == -1) {
      printf("no enough reg\n");
      exit(0);
    }
    fprintf(mips, "lw $%d, %d($k0)\n", reg, -midVar[name]);
    midVar.at(name) = reg; //TODO: 检查这个修改应该 ok
    return reg;
  }
}

void saveAll() {
  // t0-t9, s0-s7, fp, ra 一共 20 个
  int off = 0;
  fprintf(mips, "addi $sp, $sp, -80\n");
  for(int i = 8; i <= 25; i++) {
    fprintf(mips, "sw $%d, %d($sp)\n", i, off);
    off +=4;
  }
  fprintf(mips, "sw $fp, %d($sp)\n", off);
  off+=4;
  fprintf(mips, "sw $ra, %d($sp)\n", off);
}

void restoreAll() {
  int off = 0;
  for(int i = 8; i <= 25; i++) {
    fprintf(mips, "lw $%d, %d($sp)\n", i, off);
    off +=4;
  }
  fprintf(mips, "lw $fp, %d($sp)\n", off);
  off+=4;
  fprintf(mips, "lw $ra, %d($sp)\n", off);
  fprintf(mips, "addi $sp, $sp, 80\n"); // 出栈
}

void popStack() {
  // 计数当前局部符号表有多少在栈里的变量
  int cnt = 0;
  for(list<Symbol*>::iterator iter = symbolTable[1].begin(); iter != symbolTable[1].end(); iter++) {
    if ((*iter)->kind == Para || (*iter)->kind == VAR) {
      cnt++;
    } else if ((*iter)->kind == ARRAY) {
      cnt += stoi((*iter)->remark);
    }
    if ((*iter)->regNo != 0) {
      // 取消对寄存器的占用
      regUse[(*iter)->regNo - 8] = 0;
      (*iter)->regNo = 0; // 虽然不用也可以
    }
  }
  if (cnt != 0) {
    fprintf(mips, "addi $sp, $sp, %d\n", cnt * 4);
  }
}

void popMid(string name) {
  // 策略：一个中间变量一旦被用了，那么它就可以走了
  if (midVar.find(name) == midVar.end()) {
    return;
  }
  int reg = midVar.at(name);
  if (reg > 0) {
    // 设置对应的寄存器为可用
    regUse[reg - 8] = 0;
  }
  midVar.erase(name);
  midUse.erase(name);
}

void setUse(string name) {
  Symbol* sym = lookTable(name, 1);
  if (sym == NULL) {
    sym = lookTable(name, 0);
    if (sym == NULL) {
      printf("not find symbol\n");
      exit(0);
    }
  }
  sym->use = 1;
}

void freeUse(string name) {
  Symbol* sym = lookTable(name, 1);
  if (sym == NULL) {
    sym = lookTable(name, 0);
    if (sym == NULL) {
      printf("not find symbol\n");
      exit(0);
    }
  }
  sym->use = 0;
}

void popAllVar() { // 将所有变量换出去
  for(list<Symbol*>::iterator iter = symbolTable[0].begin(); iter != symbolTable[0].end(); iter++) {
    if ((*iter)->regNo != 0 && (*iter)->use == 0) {
      // 将其换出
      fprintf(mips, "sw $%d, %d($gp)\n", (*iter)->regNo, (*iter)->spOff); // 存到 gp
      regUse[(*iter)->regNo - 8] = 0; // set regUse free
      (*iter)->regNo = 0;
    }
  }
  for(list<Symbol*>::iterator iter = symbolTable[1].begin(); iter != symbolTable[1].end(); iter++) {
    if ((*iter)->regNo != 0 && (*iter)->use == 0) {
      // 将其换出
      fprintf(mips, "sw $%d, %d($fp)\n", (*iter)->regNo, (*iter)->spOff); // 存到 gp
      regUse[(*iter)->regNo - 8] = 0; // set regUse free
      (*iter)->regNo = 0;
    }
  }
}

void genMips() {
  ifstream mid;
  mid.open("semi-code.txt");
  while(!mid.eof()) {
    string line;
    getline(mid, line); // 按行读取
    if (line == "") {
      return;
    }
    vector<string> strs = split(line, ' ');
    if (strs[0] == "$j") {
      // j label
      if (strs.size() != 2) {
        // 应该只有两个
        printf("j error\n");
        exit(1);
      }
      fprintf(mips, "j %s\n", strs[1].data());
    } else if (strs[0] == "$bez") {
      if (strs.size() != 3) {
        printf("bez error\n");
        exit(0);
      }
      fprintf(mips, "beq $%d, $0, %s\n", loadMid(strs[1]), strs[2].data());
      popMid(strs[1]);
    } else if (strs[0] == "$bnz") {
      if (strs.size() != 3) {
        printf("bnz error\n");
        exit(0);
      }
      fprintf(mips, "bnq $%d, $0, %s\n", loadMid(strs[1]), strs[2].data());
      popMid(strs[1]);
    } else if (strs[0] == "$push") {
      if (strs.size() != 3){
        printf("error push\n");
        exit(0);
      }
      if (strs[1].length() == 3 && strs[1][0] == '\'' && strs[1][2] == '\'') {
        // 表达式为一个字符，存入 ASCII 码值
        int ascii = strs[1][1];
        fprintf(mips, "li $at, %d\n", ascii);
        fprintf(mips, "sw $at, -%d($fp)\n", 4*stoi(strs[2]));
      } else if (isNum(strs[1])) {
        // 表达式为一个整数
        fprintf(mips, "li $at, %d\n", stoi(strs[1])); // TODO: stoi 应该可以转负数吧
        fprintf(mips, "sw $at, -%d($fp)\n", 4*stoi(strs[2]));
      } else if (isMid(strs[1])) {
        // 中间变量
        fprintf(mips, "sw $%d, -%d($fp)\n", loadMid(strs[1]), 4*stoi(strs[2]));
        popMid(strs[1]);
      } else {
        // 标识符
        if (isConst(strs[1])) {
          fprintf(mips, "li $at, %d\n", getConst(strs[1]));
          fprintf(mips, "sw $at, -%d($fp)\n", 4*stoi(strs[2]));
        } else {
          int reg = load(strs[1]);
          fprintf(mips, "sw $%d, -%d($fp)\n", reg, 4*stoi(strs[2]));
          pop(strs[2], false);
        }
      }
    } else if (strs[0] == "$save") {
      // 保存现场，fp 指向 sp，自减 sp
      saveAll();
      fprintf(mips, "move $fp, $sp\n"); // 现在的 sp 为下一个函数的栈帧开始
      Symbol* sym = lookTable(strs[2], 0);
      if (sym == NULL) {
        printf("use func error\n");
        exit(0);
      }
      int paraLen = sym->remark.length();
      fprintf(mips, "addi $sp, $sp, -%d\n", paraLen * 4);
    } else if (strs[0] == "$call") {
      fprintf(mips, "jal %s\n", strs[1].data());
      restoreAll();
    } else if (strs[0] == "scanf") {
      if (strs[1] == "int") {
        fprintf(mips, "li $v0, 5\n");
        fprintf(mips, "syscall\n");
      } else {
        fprintf(mips, "li $v0, 12\n");
        fprintf(mips, "syscall\n");
      }
      int reg = load(strs[2]); // TODO: 不能对 const 变量进行 scanf
      fprintf(mips, "move $%d, $v0\n", reg);
      pop(strs[2], true); // 用完变量就放回
    } else if (strs[0] == "$print") {
      if (strs.size() == 2) {
        if (strs[1] == "newline") {
          fprintf(mips, "li $v0, 11\n");
          fprintf(mips, "li $a0, 10\n"); // 换行
          fprintf(mips, "syscall\n");
        } else {
          fprintf(mips, "la $a0, %s\n", strs[1].data());
          fprintf(mips, "li $v0, 4\n");
          fprintf(mips, "syscall\n");
        }
      } else {
        if (strs[1] == "int") {
          fprintf(mips, "li $v0, 1\n");
        } else {
          fprintf(mips, "li $v0, 11\n");
        }
        if (isChar(strs[2])) {
          // 表达式为一个字符
          int ascci = strs[2][1];
          fprintf(mips, "li $a0, %d\n", ascci);
        } else if (isNum(strs[2])) {
          // 表达式为一个整数
          fprintf(mips, "li $a0, %d\n", stoi(strs[2]));
        } else if (isMid(strs[2])) {
          // 中间变量
          fprintf(mips, "move $a0, $%d\n", loadMid(strs[2]));
          popMid(strs[2]);
        } else {
          // 标识符
          if (isConst(strs[2])) {
            fprintf(mips, "li $a0, %d\n", getConst(strs[2]));
          } else {
            fprintf(mips, "move $a0, $%d\n", load(strs[2]));
            pop(strs[2], false);
          }
        }
        fprintf(mips, "syscall\n");
      }
    } else if (strs[0] == "return") {
      // 加载返回值，撤去 sp
      if (strs.size() == 1) {
        // 无返回值
        popStack();
        fprintf(mips, "jr $ra\n");
      } else {
        if (isChar(strs[1])) {
          int ascii = strs[1][1];
          fprintf(mips, "li $v0, %d\n", ascii);
        } else if (isNum(strs[1])) {
          fprintf(mips, "li $v0, %d\n", stoi(strs[1]));
        } else if (isMid(strs[1])) {
          fprintf(mips, "move $v0, $%d\n", loadMid(strs[1]));
          popMid(strs[1]);
        } else {
          // 标识符
          if (isConst(strs[1])) {
            fprintf(mips, "li $v0, %d\n", getConst(strs[1]));
          } else {
            fprintf(mips, "move $v0, $%d\n", load(strs[1]));
            pop(strs[1], false);
          }
        }
        popStack();
        fprintf(mips, "jr $ra\n");
      }
    } else if (strs[0].back() == ':') {
      // 标签
      fprintf(mips, "%s\n", strs[0].data());
    } else if (strs[0] == "int" || strs[0] == "void" || strs[0] == "char") {
      // 函数定义
      fprintf(mips, "%s:\n", strs[1].data()); // 函数标签
      // 加载局部符号表，并分配 sp 空间
      symbolTable[1] = symbolMap[strs[1]];
      initVar(1);
      popAllVar();
    } else if (strs[0] == "$bne" || strs[0] == "$beq" || strs[0] == "$bge" || strs[0] == "$bgt" || strs[0] == "$blt" || strs[0] == "$ble") {
      int regA;
      int regB;
      if (isChar(strs[1])) {
        regA = findNextReg();
        int ascii = strs[1][1];
        fprintf(mips, "li $%d, %d\n", regA, ascii);
      } else if (isConst(strs[1])) {
        regA = findNextReg();
        fprintf(mips, "li $%d, %d\n", regA, getConst(strs[1]));
      } else if (isNum(strs[1])) {
        regA = findNextReg();
        fprintf(mips, "li $%d, %d\n", regA, stoi(strs[1]));
      } else if (isMid(strs[1])) {
        regA = loadMid(strs[1]);
        midUse.insert({ strs[1], 1 });
      } else {
        regA = load(strs[1]);
        setUse(strs[1]);
      }
      if (isChar(strs[2])) {
        regB = findNextReg();
        int ascii = strs[2][1];
        fprintf(mips, "li $%d, %d\n", regB, ascii);
      } else if (isConst(strs[2])) {
        regB = findNextReg();
        fprintf(mips, "li $%d, %d\n", regB, getConst(strs[2]));
      } else if (isNum(strs[2])) {
        regB = findNextReg();
        fprintf(mips, "li $%d, %d\n", regB, stoi(strs[2]));
      } else if (isMid(strs[2])) {
        regB = loadMid(strs[2]);
      } else {
        regB = load(strs[2]);
      }
      fprintf(mips, "%s $%d, $%d, %s\n", strs[0].substr(1, 3).data(), regA, regB, strs[3].data());
      if (isMid(strs[1])) { // 中间变量使用结束，报废
        popMid(strs[1]);
      } else if (isChar(strs[1]) || isConst(strs[1]) || isNum(strs[1])) {
        regUse[regA - 8] = 0;
      } else {
        pop(strs[1], false);
      }
      if (isMid(strs[2])) {
        popMid(strs[2]);
      } else if (isChar(strs[2]) || isConst(strs[2]) || isNum(strs[2])) {
        regUse[regB - 8] = 0;
      } else {
        pop(strs[2], false);
      }
    } else {
      if (strs[1] != "=") {
        printf("unkown mid code\n");
        exit(0);
      }
      int regA = 0;
      if (!isMid(strs[0]) && strs[0].back() != ']') {
        // 单个标识符
        regA = load(strs[0]); // 不能对 const 进行赋值，所以这个肯定不是 isConst
        setUse(strs[0]);
      } else if (isMid(strs[0])){
        // 中间变量
        regA = findNextReg();
        midVar.insert({ strs[0], regA }); // 加载中间变量
        midUse.insert({ strs[0], 1 });
      } // 如果为数组的话，直接翻译成 sw lw
      if (strs.size() == 5) { // a = b op c
        if ((isChar(strs[2]) || isNum(strs[2]) || isConst(strs[2])) && (isChar(strs[4]) || isNum(strs[4]) || isConst(strs[4]))) { // b c 都没有分配寄存器
          if (strs[3] == "+" || strs[3] == "-") {
            if (isChar(strs[2])) {
              fprintf(mips, "li $at, %d\n", strs[2][1]);
            } else if (isNum(strs[2])) {
              fprintf(mips, "li $at, %d\n", stoi(strs[2]));
            } else {
              fprintf(mips, "li $at, %d\n", getConst(strs[2]));
            }
            if (isChar(strs[4])) {
              int ascii = strs[4][1];
              fprintf(mips, "addi $%d, $at, %d\n", regA, strs[3] == "-" ? -ascii : ascii);
            } else if (isNum(strs[4])) {
              fprintf(mips, "addi $%d, $at, %d\n", regA, strs[3] == "-" ? -stoi(strs[4]) : stoi(strs[4]));
            } else {
              fprintf(mips, "addi $%d, $at, %d\n", regA, strs[3] == "-" ? -getConst(strs[4]) : getConst(strs[4]));
            }
          } else { // 乘除
            if (isChar(strs[2])) {
              fprintf(mips, "li $k1, %d\n", strs[2][1]);
            } else if (isNum(strs[2])) {
              fprintf(mips, "li $k1, %d\n", stoi(strs[2]));
            } else {
              fprintf(mips, "li $k1, %d\n", getConst(strs[2]));
            }
            if (isChar(strs[4])) {
              int ascii = strs[4][1];
              fprintf(mips, "%s $%d, $k1, %d\n", strs[3] == "*" ? "mul" : "div", regA, ascii);
            } else if (isNum(strs[4])) {
              fprintf(mips, "%s $%d, $k1, %d\n", strs[3] == "*" ? "mul" : "div", regA, stoi(strs[4]));
            } else {
              fprintf(mips, "%s $%d, $k1, %d\n", strs[3] == "*" ? "mul" : "div", regA, getConst(strs[4]));
            }
          }
        } else if (isChar(strs[2]) || isNum(strs[2]) || isConst(strs[2])) { // c 为标识符或者中间变量
          int regC = 0;
          if (isMid(strs[4])) {
            // 一定加载过了的，所以只需要找到它就可以了
            regC = loadMid(strs[4]);
          } else {
            regC = load(strs[4]);
          }
          if (strs[3] == "-" || strs[3] == "+") {
            if (isChar(strs[2])) {
              int ascii = strs[2][1];
              fprintf(mips, "addi $%d, $%d, %d\n", regA, regC, strs[3] == "-" ? -ascii : ascii);
            } else if (isNum(strs[2])) {
              fprintf(mips, "addi $%d, $%d, %d\n", regA, regC, strs[3] == "-" ? -stoi(strs[2]) : stoi(strs[2]));
            } else {
              fprintf(mips, "addi $%d, $%d, %d\n", regA, regC, strs[3] == "-" ? -getConst(strs[2]) : getConst(strs[2]));
            }
          } else { // 乘除
            if (isChar(strs[2])) {
              int ascii = strs[2][1];
              fprintf(mips, "%s $%d, $%d, %d\n", strs[3] == "*" ? "mul" : "div", regA, regC, ascii);
            } else if (isNum(strs[2])) {
              fprintf(mips, "%s $%d, $%d, %d\n", strs[3] == "*" ? "mul" : "div", regA, regC, stoi(strs[2]));
            } else {
              fprintf(mips, "%s $%d, $%d, %d\n", strs[3] == "*" ? "mul" : "div", regA, regC, getConst(strs[2]));
            }
          }
          if (isMid(strs[4])) {
            // 一定加载过了的，所以只需要找到它就可以了
            popMid(strs[4]);
          } else {
            pop(strs[4], false);
          }
        } else if (isChar(strs[4]) || isNum(strs[4]) || isConst(strs[4])) { // b 为标识符或者中间变量
          int regB = 0;
          if (isMid(strs[2])) {
            // 一定加载过了的，所以只需要找到它就可以了
            regB = loadMid(strs[2]);
          } else {
            regB = load(strs[2]);
          }
          if (strs[3] == "+" || strs[3] == "-") {
            if (isChar(strs[4])) {
              int ascii = strs[4][1];
              fprintf(mips, "addi $%d, $%d, %d\n", regA, regB, strs[3] == "-" ? -ascii : ascii);
            } else if (isNum(strs[4])) {
              fprintf(mips, "addi $%d, $%d, %d\n", regA, regB, strs[3] == "-" ? -stoi(strs[4]) : stoi(strs[4]));
            } else {
              fprintf(mips, "addi $%d, $%d, %d\n", regA, regB, strs[3] == "-" ? -getConst(strs[4]) : getConst(strs[4]));
            }
          } else {
            if (isChar(strs[4])) {
              int ascii = strs[4][1];
              fprintf(mips, "%s $%d, $%d, %d\n", strs[3] == "*" ? "mul" : "div", regA, regB, ascii);
            } else if (isNum(strs[4])) {
              fprintf(mips, "%s $%d, $%d, %d\n", strs[3] == "*" ? "mul" : "div", regA, regB, stoi(strs[4]));
            } else {
              fprintf(mips, "%s $%d, $%d, %d\n", strs[3] == "*" ? "mul" : "div", regA, regB, getConst(strs[4]));
            }
          }
          if (isMid(strs[2])) {
            // 一定加载过了的，所以只需要找到它就可以了
            popMid(strs[2]);
          } else {
            pop(strs[2], false);
          }
        } else { // b c 都是标识符或者中间变量
          int regB = 0;
          int regC = 0;
          if (isMid(strs[2])) {
            regB = loadMid(strs[2]);
            midUse.insert({ strs[2], 1 });
          } else {
            regB = load(strs[2]);
            setUse(strs[2]);
          }
          if (isMid(strs[4])) {
            regC = loadMid(strs[4]);
          } else {
            regC = load(strs[4]);
          }
          if (strs[3] == "+" || strs[3] == "-") {
            fprintf(mips, "%s $%d, $%d, $%d\n", strs[3] == "+" ? "add" : "sub", regA, regB, regC);
          } else {
            fprintf(mips, "%s $%d, $%d, $%d\n", strs[3] == "*" ? "mul" : "div", regA, regB, regC);
          }
          if (isMid(strs[2])) {
            popMid(strs[2]);
          } else {
            pop(strs[2], false);
          }
          if (isMid(strs[4])) {
            popMid(strs[4]);
          } else {
            pop(strs[4], false);
          }
        }
      } else if (strs.size() == 4) { // 有前导 + - 号的表达式 a = op b
        // b 可以为中间变量或者 expValue
        if (isMid(strs[3])) {
          fprintf(mips, "%s $%d, $0, $%d\n", strs[2] == "+" ? "add" : "sub", regA, loadMid(strs[3]));
        } else if (isChar(strs[3])) {
          int ascii = strs[3][1];
          fprintf(mips, "addi $%d, $0, %d\n", regA, strs[2] == "+" ? ascii : -ascii);
        } else if (isNum(strs[3])) {
          fprintf(mips, "addi $%d, $0, %d\n", regA, strs[2] == "+" ? stoi(strs[3]) : -stoi(strs[3]));
        } else {
          if (isConst(strs[3])) {
            fprintf(mips, "addi $%d, $0, %d\n", regA, strs[2] == "+" ? getConst(strs[3]) : -getConst(strs[3]));
          } else {
            fprintf(mips, "%s $%d, $0, $%d\n", strs[2] == "+" ? "add" : "sub", regA, load(strs[3]));
            pop(strs[3], false);
          }
        }
      } else if (strs[0].back() == ']') { // 赋值给数组 a[b] = c
        string b = strs[0].substr(strs[0].find('[') + 1, strs[0].length() - strs[0].find('[') - 2);
        string a = strs[0].substr(0, strs[0].find('['));
        Symbol* sym = lookTable(a, 1);
        if (sym == NULL) {
          sym = lookTable(a, 0);
          if (sym == NULL) {
            printf("not find symbol\n");
            exit(0);
          }
        }
        int off = sym->spOff;
        bool useK1 = false;
        // 计算数组内偏移
        if (isChar(b)) {
          int ascii = b[1];
          off += ascii * 4;
        } else if (isNum(b)) {
          off += stoi(b) * 4;
        } else if (isMid(b)) {
          useK1 = true;
          fprintf(mips, "sll $k1, $%d, 2\n", loadMid(b));
          fprintf(mips, "add $k1, $fp, $k1\n"); // 修改起始地址
        } else { // 标识符
          useK1 = true;
          if (isConst(b)) {
            fprintf(mips, "sll $k1, %d, 2\n", getConst(b));
          } else {
            fprintf(mips, "sllv $k1, $%d, 2\n", load(b));
            pop(b, false);
          }
          fprintf(mips, "add $k1, $fp, $k1\n"); // 修改起始地址
        }
        // 计算要写入的值
        int reg = 0;
        if (isChar(strs[2])) {
          int res = strs[2][1];
          fprintf(mips, "li $at, %d\n", res);
          reg = 1;
        } else if (isNum(strs[2])) {
          int res = stoi(strs[2]);
          fprintf(mips, "li $at, %d\n", res);
          reg = 1;
        } else if (isMid(strs[2])) {
          reg = loadMid(strs[2]);
        } else {
          if (isConst(strs[2])) {
            fprintf(mips, "li $at, %d\n", getConst(strs[2]));
            reg = 1;
          } else {
            reg = load(strs[2]);
          }
        }
        if (useK1) {
          fprintf(mips, "sw $%d, %d($k1)\n", reg, off);
        } else {
          fprintf(mips, "sw $%d, %d($fp)\n", reg, off);
        }
        if (reg != 1) { // 标识符或者中间变量
          if (isMid(strs[2])) {
            popMid(strs[2]);
          } else {
            pop(strs[2], false);
          }
        }
      } else if (strs[2].back() == ']') { // 取出数组的值 a = b[c]
        string c = strs[2].substr(strs[0].find('[') + 1, strs[0].length() - strs[0].find('[') - 2);
        string b = strs[2].substr(0, strs[0].find('['));
        Symbol* sym = lookTable(b, 1);
        if (sym == NULL) {
          sym = lookTable(b, 0);
          if (sym == NULL) {
            printf("not find symbol\n");
            exit(0);
          }
        }
        // 计算偏移
        int off = sym->spOff;
        int useK1 = false;
        if (isChar(c)) {
          int ascii = c[1];
          off += ascii * 4;
        } else if (isNum(c)) {
          off += stoi(c) * 4;
        } else if (isMid(c)) {
          useK1 = true;
          fprintf(mips, "sll $k1, $%d, 2\n", loadMid(c));
          fprintf(mips, "add $k1, $fp, $k1\n"); // 修改起始地址
        } else { // 标识符
          useK1 = true;
          if (isConst(c)) {
            fprintf(mips, "sll $k1, %d, 2\n", getConst(c));
          } else {
            fprintf(mips, "sllv $k1, $%d, 2\n", load(c));
            pop(c, false);
          }
          fprintf(mips, "add $k1, $fp, $k1\n"); // 修改起始地址
        }
        // lw
        if (useK1) {
          fprintf(mips, "lw $%d, %d($k1)\n", regA, off);
        } else {
          fprintf(mips, "lw $%d, %d($fp)\n", regA, off);
        }
      } else if (strs[2] == "RET") { // 获取函数返回值
        fprintf(mips, "move $%d, $v0\n", regA);
      } else { // a = b
        if (isMid(strs[2])) {
          fprintf(mips, "move $%d, $%d\n", regA, loadMid(strs[2]));
        } else if (isChar(strs[2])) {
          int ascii = strs[2][1];
          fprintf(mips, "li $%d, %d\n", regA, ascii);
        } else if (isNum(strs[2])) {
          fprintf(mips, "li $%d, %d\n", regA, stoi(strs[2]));
        } else {
          if (isConst(strs[2])) {
            fprintf(mips, "li $%d, %d\n", regA, getConst(strs[2]));
          } else {
            fprintf(mips, "move $%d, $%d\n", regA, load(strs[2]));
            pop(strs[2], false);
          }
        }
      }
      // 取消 A 的占用情况
      if (!isMid(strs[0]) && strs[0].back() != ']') {
        // 单个标识符
        pop(strs[0], true);
      } else if (isMid(strs[0])){
        // 中间变量
        midUse.erase(strs[0]);
      }
    }
  }
}

void blockAnalysis() {
  int lineNo = 0;
  vector<struct Block*> blocks;
  vector<int> entrys; // 存储入口语句以及结束语句, 应该时递增的
  ifstream mid;
  mid.open("semi-code.txt");
  while(!mid.eof()) {
    string line;
    getline(mid, line); // 按行读取
    lineNo++;
    if (line == "") {
      return;
    }
    vector<string> strs = split(line, ' ');
    if (strs[0] == "int" || strs[0] == "void" || strs[0] == "char") {
      // 函数定义，函数入口
      entrys.push_back(lineNo);
    } else if (strs[0] == "$bne" || strs[0] == "$beq" || strs[0] == "$bge" || strs[0] == "$bgt" || strs[0] == "$blt" || strs[0] == "$ble") {
      entrys.push_back(lineNo);
      entrys.push_back(lineNo + 1);
    } else if (strs[0] == "return") {
      entrys.push_back(lineNo);
    }
  }
}