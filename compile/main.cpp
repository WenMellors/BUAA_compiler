#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <fstream>
#include "lexical_analysis.hpp"
#include "syntax_analysis.hpp"
using namespace std;

vector<int> globalReg = {19, 20, 21, 22, 23, 24, 25};
vector<int> localReg = {12, 13, 14, 15, 16, 17, 18};
vector<int> tempReg = {8, 9, 10, 11};
vector<int> saveReg;
map<string, int> midVar; // 存放中间变量被分配到的寄存器, 如果对应值为负，则为其在 DM 的偏移的负数
map<string, int> midUse; // 正在使用中的中间变量，不可以替换
map<string, vector<int>> funcMapReg; // 记录这个函数会分配哪些局部寄存器
int paraLen;
int nextReg;
int dmOff;
FILE* mips;
void genMips();
void initData();
void initVar(int level, string name);

int main () {
  list<struct Lexeme> tokenList;
  tokenList = lexcialParse();
  syntaxParse(tokenList);
  mips = fopen("mips.txt", "w");
  initData();
  fprintf(mips, ".text\n");
  fprintf(mips, "la $k0, d\n");
  fprintf(mips, "move $gp, $sp\n");
  symbolTable[0] = symbolMap.at("global"); // load global Table
  initVar(0, "global");
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

bool sortFunction(int i, int j) {
  return i>j;
}

// 为函数内的变量分配地址
void initVar(int level, string name) {
  // 并不是所有的 symbol 都是变量
  int length = 0;
  int offset = -4;
  list<Symbol*>::iterator iter = symbolTable[level].begin();
  vector<int> weights;
  map<int, vector<Symbol*>> weightToSymbol;
  while(iter != symbolTable[level].end()) {
    if ((*iter)->kind == VAR) {
      (*iter)->spOff = offset; // 标记每个变量到 fp 的偏移，如果是全局变量就是 gp
      offset -= 4;
      length++;
      weights.push_back((*iter)->weight);
      if (weightToSymbol.find((*iter)->weight) == weightToSymbol.end()) {
        vector<Symbol*> newVec;
        newVec.push_back(*iter);
        weightToSymbol.insert({(*iter)->weight, newVec});
      } else {
        weightToSymbol.at((*iter)->weight).push_back(*iter);
      }
    } else if ((*iter)->kind == Para) {
      // 形参的 sp 由调用方减去，但是这里得记下他的偏移
      (*iter)->spOff = offset;
      offset -= 4;
      weights.push_back((*iter)->weight);
      if (weightToSymbol.find((*iter)->weight) == weightToSymbol.end()) {
        vector<Symbol*> newVec;
        newVec.push_back(*iter);
        weightToSymbol.insert({(*iter)->weight, newVec});
      } else {
        weightToSymbol.at((*iter)->weight).push_back(*iter);
      }
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
  sort(weights.begin(), weights.end(), sortFunction);
  // assign reg to var and para
  vector<int> distributeVec;
  int next = level == 0 ? 0 : nextReg;
  for (int i = 0; i < weights.size(); i++) {
    if (weights[i] == 0) {
      break; // 0 needn't be distribute
    }
    if (level == 0 && next < globalReg.size()) {
      Symbol* temp = weightToSymbol.at(weights[i]).back();
      weightToSymbol.at(weights[i]).pop_back();
      temp->regNo = globalReg[next];
      distributeVec.push_back(temp->regNo);
      temp->distributed = true;
      next++;
      // global 不会有参数变量，且参数变量的值传递由调用函数负责
    } else if (level == 1) {
      Symbol* temp = weightToSymbol.at(weights[i]).back();
      weightToSymbol.at(weights[i]).pop_back();
      temp->regNo = localReg[next];
      distributeVec.push_back(temp->regNo);
      temp->distributed = true;
      next = (next + 1) % localReg.size();
      if (next == nextReg) {
        break;
      }
    } else {
      break;
    }
  }
  if (level == 0) {
    while (next < globalReg.size()) {
      localReg.push_back(globalReg[next]);
      next++;
    }
  } else {
    funcMapReg.insert({name, distributeVec});
    nextReg = next;
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

int findNextReg() { // find a reg for temp var
  if (!tempReg.empty()) {
    int reg = tempReg.back();
    tempReg.pop_back();
    return reg;
  }
  // 没有空闲寄存器了
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
  fprintf(stderr, "not enough reg\n");
  exit(1);
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
      fprintf(stderr, "not find symbol\n");
      exit(1);
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

int load(string name) { // load a var
  // 检查是否装载，如果没有的话，就装载
  int level = 1;
  Symbol* sym = lookTable(name, 1);
  if (sym == NULL) {
    level = 0;
    sym = lookTable(name, 0);
    if (sym == NULL) {
      fprintf(stderr, "not find symbol\n");
      exit(1);
    }
  }
  if (sym->regNo != 0 && (!sym->disable || (sym->disable && sym->regNo != sym->disableReg))) {
    // 已经被分配了寄存器的
    return sym->regNo;
  } else if (sym->disable && sym->distributed) {
    // 这个还是从栈帧里面取吧
    sym->use++; // 引用次数加一
    int reg = findNextReg();
    if (reg == -1) {
      fprintf(stderr, "no enough reg\n");
      exit(1);
    }
    fprintf(mips, "lw $%d, %d($fp)\n", reg, sym->spOff);
    sym->regNo = reg;
    return reg;
  } else {
    // 当作中间变量处理，只是需要注意一下换出写回
    sym->use++; // 引用次数加一
    int reg = findNextReg();
    if (reg == -1) {
      fprintf(stderr, "no enough reg\n");
      exit(1);
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

void pop(string name, bool rewrite) { // 如果变量没有被分配寄存器，那么该寄存器将会被归还到中间寄存器池，并写回栈
  int level = 1;
  Symbol* sym = lookTable(name, 1);
  if (sym == NULL) {
    level = 0;
    sym = lookTable(name, 0);
    if (sym == NULL) {
      fprintf(stderr, "not find symbol\n");
      exit(1);
    }
  }
  if (sym->distributed && !sym->disable) {
    return;
  }
  sym->use --;
  // 还在被引用则不换出
  if (sym->use == 0 && sym->regNo != 0) {
    // 将变量换出寄存器堆
    // disable 的情况一定不会 rewrite
    if (rewrite) { // 如果修改了就写回栈里
      if (level == 0) {
        // 全局变量
        fprintf(mips, "sw $%d, %d($gp)\n", sym->regNo, sym->spOff);
      } else {
        // 局部变量
        fprintf(mips, "sw $%d, %d($fp)\n", sym->regNo, sym->spOff);
      }
    }
    // 归还借用的寄存器
    tempReg.push_back(sym->regNo);
    sym->use = 0;
    if (sym->disable) {
      sym->regNo = sym->disableReg;
    } else {
      sym->regNo = 0;
    }
  }
}

int loadMid(string name) {
  // 如果中间变量在寄存堆里面，则直接返回
  if (midVar.find(name) == midVar.end()) {
    // 没有这个中间变量，它要么没加载，要么已经出去了
    fprintf(stderr, "not found mid var\n");
    exit(1);
  }
  if (midVar.at(name) > 0 ) {
    return midVar.at(name);
  } else {
    int reg = findNextReg();
    if (reg == -1) {
      fprintf(stderr, "no enough reg\n");
      exit(1);
    }
    fprintf(mips, "lw $%d, %d($k0)\n", reg, -midVar[name]);
    midVar.at(name) = reg; //TODO: 检查这个修改应该 ok
    return reg;
  }
}

void saveAll(string name) {
  // fp, ra 一定要保存，中间变量在寄存器里面的需要保存
  // 局部寄存器中，只需要保存下一个函数会用到的寄存器
  int n = 0;
  vector<int> distributedVec = funcMapReg.at(name);
  saveReg.clear();
  for(map<string, int>::iterator iter = midVar.begin(); iter != midVar.end(); iter++) {
    if (iter->second > 0) {
      // 在寄存器堆里面的中间变量
      saveReg.push_back(iter->second);
      n++;
    }
  }
  // 对于冲突的寄存器保存在变量被分配到的地方，只有中间变量才写到这里
  for (list<Symbol*>::iterator iter = symbolTable[1].begin(); iter != symbolTable[1].end(); iter++) {
    if ((*iter)->regNo != 0) {
      // 在寄存器堆里面的局部变量
      bool shouldSave = false;
      for (int i = 0; i < distributedVec.size(); i++) {
        if (distributedVec[i] == (*iter)->regNo) {
          shouldSave = true;
          break;
        }
      }
      if (shouldSave) {
        if ((*iter)->disable) {
          // 已经 disable 过了的
          (*iter)->disableTimes++;
        } else {
          // 写回栈桢里
          fprintf(mips, "sw $%d, %d($fp)\n", (*iter)->regNo, (*iter)->spOff);
          (*iter)->disable = true;
          (*iter)->disableReg = (*iter)->regNo;
          (*iter)->disableTimes++;
        }
      }
    }
  }
  n+=2;
  int off = 0;
  fprintf(mips, "addi $sp, $sp, -%d\n", n*4);
  for(int i = 0; i < saveReg.size(); i++) {
    fprintf(mips, "sw $%d, %d($sp)\n", saveReg[i], off);
    off +=4;
  }
  fprintf(mips, "sw $fp, %d($sp)\n", off);
  off+=4;
  fprintf(mips, "sw $ra, %d($sp)\n", off);
}

void restoreAll(string name) {
  int n = 0;
  vector<int> distributedVec = funcMapReg.at(name);
  saveReg.clear();
  for(map<string, int>::iterator iter = midVar.begin(); iter != midVar.end(); iter++) {
    if (iter->second > 0) {
      // 在寄存器堆里面的中间变量
      saveReg.push_back(iter->second);
      n++;
    }
  }
  n+=2;
  int off = 0;
  for(int i = 0; i < saveReg.size(); i++) {
    fprintf(mips, "lw $%d, %d($sp)\n", saveReg[i], off);
    off +=4;
  }
  fprintf(mips, "lw $fp, %d($sp)\n", off);
  off+=4;
  fprintf(mips, "lw $ra, %d($sp)\n", off);
  fprintf(mips, "addi $sp, $sp, %d\n", n*4); // 出栈
  // fp 恢复之后，恢复冲突寄存器的值
  for (list<Symbol*>::iterator iter = symbolTable[1].begin(); iter != symbolTable[1].end(); iter++) {
    if ((*iter)->regNo != 0) {
      // 在寄存器堆里面的局部变量
      bool shouldRestore = false;
      for (int i = 0; i < distributedVec.size(); i++) {
        if (distributedVec[i] == (*iter)->regNo) {
          shouldRestore = true;
          break;
        }
      }
      if (shouldRestore) {
        // 要将其恢复
        (*iter)->disableTimes--;
        if ((*iter)->disableTimes == 0) {
          fprintf(mips, "lw $%d, %d($fp)\n", (*iter)->regNo, (*iter)->spOff);
          (*iter)->regNo = (*iter)->disableReg;
          (*iter)->disable = false;
        }
      }
    }
  }
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
  }
  if (cnt != 0) {
    fprintf(mips, "addi $sp, $sp, %d\n", cnt * 4);
  }
}

void popMid(string name) {
  // 将一个中间变量废弃
  if (midVar.find(name) == midVar.end()) {
    return;
  }
  int reg = midVar.at(name);
  if (reg > 0) {
    // 设置对应的寄存器为可用
    tempReg.push_back(reg);
  }
  midVar.erase(name);
  midUse.erase(name);
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
        fprintf(stderr, "j error\n");
        exit(1);
      }
      fprintf(mips, "j %s\n", strs[1].data());
    } else if (strs[0] == "$bez") {
      if (strs.size() != 3) {
        fprintf(stderr, "bez error\n");
        exit(1);
      }
      int regA;
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
      }
      fprintf(mips, "beq $%d, $0, %s\n", regA, strs[2].data());
      if (isChar(strs[1]) || isConst(strs[1]) || isNum(strs[1])) {
        tempReg.push_back(regA);
      } else if (isMid(strs[1])) {
        popMid(strs[1]);
      } else {
        pop(strs[1], false);
      }
    } else if (strs[0] == "$bnz") {
      if (strs.size() != 3) {
        fprintf(stderr, "bnz error\n");
        exit(1);
      }
      int regA;
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
      }
      fprintf(mips, "bne $%d, $0, %s\n", regA, strs[2].data());
      if (isChar(strs[1]) || isConst(strs[1]) || isNum(strs[1])) {
        tempReg.push_back(regA);
      } else if (isMid(strs[1])) {
        popMid(strs[1]);
      } else {
        pop(strs[1], false);
      }
    } else if (strs[0] == "$push") { // push 的时候参数的变量已经被分配寄存器，所以直接传给它
      if (strs.size() != 5){
        fprintf(stderr, "error push\n");
        exit(1);
      }
      int n = stoi(strs[2]);
      int i = 0;
      Symbol* temp;
      list<Symbol*> callFuncTable = symbolMap.at(strs[4]);
      for(list<Symbol*>::iterator iter = callFuncTable.begin(); iter != callFuncTable.end(); iter++) {
        i++;
        if (n == i) {
          temp = (*iter);
          break;
        }
      }
      if (strs[1].length() == 3 && strs[1][0] == '\'' && strs[1][2] == '\'') {
        // 表达式为一个字符，存入 ASCII 码值
        int ascii = strs[1][1];
        if (temp->distributed) {
          fprintf(mips, "li $%d, %d\n", temp->regNo, ascii);
        } else {
          fprintf(mips, "li $at, %d\n", ascii);
          fprintf(mips, "sw $at, %d($sp)\n", 4*(stoi(strs[3]) - stoi(strs[2])));
        }
      } else if (isNum(strs[1])) {
        // 表达式为一个整数
        if (temp->distributed) {
          fprintf(mips, "li $%d, %d\n", temp->regNo, stoi(strs[1]));
        } else {
          fprintf(mips, "li $at, %d\n", stoi(strs[1])); // TODO: stoi 应该可以转负数吧
          fprintf(mips, "sw $at, %d($sp)\n", 4*(stoi(strs[3]) - stoi(strs[2])));
        }
      } else if (isMid(strs[1])) {
        // 中间变量
        if (temp->distributed) {
          fprintf(mips, "move $%d, $%d\n", temp->regNo, loadMid(strs[1]));
        } else {  
          fprintf(mips, "sw $%d, %d($sp)\n", loadMid(strs[1]), 4*(stoi(strs[3]) - stoi(strs[2])));
        }
        popMid(strs[1]);
      } else {
        // 标识符
        if (isConst(strs[1])) {
          if (temp->distributed) {
            fprintf(mips, "li $%d, %d\n", temp->regNo, getConst(strs[1]));
          } else {
            fprintf(mips, "li $at, %d\n", getConst(strs[1]));
            fprintf(mips, "sw $at, %d($sp)\n", 4*(stoi(strs[3]) - stoi(strs[2])));
          }
        } else {
          // 因为现在局部变量寄存器池已经存在被更改的可能，被更改的可能是因为这个同一个寄存器被两个函数都用了，那么该寄存器的值一定在栈里
          // 对于冲突的寄存器，我们已经用 disable 解决了这个问题，在 load 和 pop 的机制里检查了。
          int reg = load(strs[1]); // 如果 strs[1] 没有被分配寄存器，那么 reg 必不可能与 saveReg 冲突
          if (temp->distributed) {
            fprintf(mips, "move $%d, $%d\n", temp->regNo, reg);
          } else {
            fprintf(mips, "sw $%d, %d($sp)\n", reg, 4*(stoi(strs[3]) - stoi(strs[2])));
          }
          pop(strs[1], false);
        }
      }
    } else if (strs[0] == "$save") {
      // 保存现场，自减 sp
      saveAll(strs[2]);
      Symbol* sym = lookTable(strs[2], 0);
      if (sym == NULL) {
        fprintf(stderr, "use func error\n");
        exit(1);
      }
      paraLen = sym->remark.length();
      fprintf(mips, "addi $sp, $sp, -%d\n", paraLen * 4);
    } else if (strs[0] == "$call") {
      Symbol* sym = lookTable(strs[1], 0);
      if (sym == NULL) {
        fprintf(stderr, "use func error\n");
        exit(1);
      }
      paraLen = sym->remark.length();
      fprintf(mips, "addi $fp, $sp, %d\n", paraLen*4); // update fp
      fprintf(mips, "jal %s\n", strs[1].data());
      restoreAll(strs[1]);
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
            int reg = load(strs[2]);
            fprintf(mips, "move $a0, $%d\n", reg);
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
      // reset tempReg
      tempReg = {8, 9, 10, 11};
      initVar(1, strs[1]);
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
        tempReg.push_back(regA);
      } else {
        pop(strs[1], false);
      }
      if (isMid(strs[2])) {
        popMid(strs[2]);
      } else if (isChar(strs[2]) || isConst(strs[2]) || isNum(strs[2])) {
        tempReg.push_back(regB);
      } else {
        pop(strs[2], false);
      }
    } else {
      if (strs[1] != "=") {
        fprintf(stderr, "unkown mid code\n");
        exit(1);
      }
      int regA = 0;
      if (!isMid(strs[0]) && strs[0].back() != ']') {
        // 单个标识符
        regA = load(strs[0]); // 不能对 const 进行赋值，所以这个肯定不是 isConst
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
          if (isChar(strs[2])) {
            fprintf(mips, "li $k1, %d\n", strs[2][1]);
          } else if (isNum(strs[2])) {
            fprintf(mips, "li $k1, %d\n", stoi(strs[2]));
          } else {
            fprintf(mips, "li $k1, %d\n", getConst(strs[2]));
          }
          if (strs[3] == "-" || strs[3] == "+") {
            fprintf(mips, "%s $%d, $k1, $%d\n", strs[3] == "-" ? "sub" : "add", regA, regC);
          } else { // 乘除
            fprintf(mips, "%s $%d, $k1, $%d\n", strs[3] == "*" ? "mul" : "div", regA, regC);
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
          popMid(strs[3]);
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
        bool isGlobal = false;
        if (sym == NULL) {
          sym = lookTable(a, 0);
          isGlobal = true;
          if (sym == NULL) {
            fprintf(stderr, "not find symbol\n");
            exit(1);
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
          popMid(b);
          if (isGlobal) {
            fprintf(mips, "add $k1, $gp, $k1\n"); // 修改起始地址
          } else {
            fprintf(mips, "add $k1, $fp, $k1\n"); // 修改起始地址
          }
        } else { // 标识符
          useK1 = true;
          if (isConst(b)) {
            int reg = findNextReg();
            fprintf(mips, "li $%d %d\n", reg, getConst(b));
            fprintf(mips, "sll $k1, $%d, 2\n", reg);
            tempReg.push_back(reg);
          } else {
            fprintf(mips, "sll $k1, $%d, 2\n", load(b));
            pop(b, false);
          }
          if (isGlobal) {
            fprintf(mips, "add $k1, $gp, $k1\n"); // 修改起始地址
          } else {
            fprintf(mips, "add $k1, $fp, $k1\n"); // 修改起始地址
          }
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
        } else if (!isGlobal) {
          fprintf(mips, "sw $%d, %d($fp)\n", reg, off);
        } else {
          fprintf(mips, "sw $%d, %d($gp)\n", reg, off);
        }
        if (reg != 1) { // 标识符或者中间变量
          if (isMid(strs[2])) {
            popMid(strs[2]);
          } else {
            pop(strs[2], false);
          }
        }
      } else if (strs[2].back() == ']') { // 取出数组的值 a = b[c]
        string c = strs[2].substr(strs[2].find('[') + 1, strs[2].length() - strs[2].find('[') - 2);
        string b = strs[2].substr(0, strs[2].find('['));
        Symbol* sym = lookTable(b, 1);
        bool isGlobal = false;
        if (sym == NULL) {
          sym = lookTable(b, 0);
          isGlobal = true;
          if (sym == NULL) {
            fprintf(stderr, "not find symbol\n");
            exit(1);
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
          popMid(c);
          if (isGlobal) {
            fprintf(mips, "add $k1, $gp, $k1\n"); // 修改起始地址
          } else {
            fprintf(mips, "add $k1, $fp, $k1\n"); // 修改起始地址
          }
        } else { // 标识符
          useK1 = true;
          if (isConst(c)) {
            int reg = findNextReg();
            fprintf(mips, "li $%d %d\n", reg, getConst(c));
            fprintf(mips, "sll $k1, $%d, 2\n", reg);
            tempReg.push_back(reg);
          } else {
            fprintf(mips, "sll $k1, $%d, 2\n", load(c));
            pop(c, false);
          }
          if (isGlobal) {
            fprintf(mips, "add $k1, $gp, $k1\n"); // 修改起始地址
          } else {
            fprintf(mips, "add $k1, $fp, $k1\n"); // 修改起始地址
          }
        }
        // lw
        if (useK1) {
          fprintf(mips, "lw $%d, %d($k1)\n", regA, off);
        } else if (!isGlobal) {
          fprintf(mips, "lw $%d, %d($fp)\n", regA, off);
        } else {
          fprintf(mips, "lw $%d, %d($gp)\n", regA, off);
        }
      } else if (strs[2] == "RET") { // 获取函数返回值
        fprintf(mips, "move $%d, $v0\n", regA);
      } else { // a = b
        if (isMid(strs[2])) {
          fprintf(mips, "move $%d, $%d\n", regA, loadMid(strs[2]));
          popMid(strs[2]);
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