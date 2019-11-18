#include "syntax_analysis.hpp"
#include <cstdio>


FILE* err;
FILE* out;
list<Symbol*> symbolTable[2];
map<string, list<Symbol*>> symbolMap;
map<int, string> strMap;
int regCnt; // use for print 1t,2t,3t
int labelCnt; 
int strCnt;

void buildSyntaxTree(list<struct Lexeme> tokenList, SyntaxNode * root);
void appendLeaf(list<struct Lexeme>::iterator* iter, SyntaxNode* root);

void constParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root, int level);
void constDefineParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root, int level);
int numParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);

void varParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root, int level);
void varDefineParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root, int level);

void funcParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);
void declareParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root, Symbol* funcSymbol);
void parameterParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root, Symbol* funcSymbol);
void complexSentenceParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root, bool needReturn, int type);

void mainParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);
void voidFuncParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);

bool sentenceListParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root, int type);
bool sentenceParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root, int type);
bool ifParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root, int type);
void conditionParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root, bool isZeroBranch);
int expParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root); // 0 int 1 char
int polyParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);
int factorParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);
bool circleParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root, int type);
int stepParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);
void useFuncParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root); // include void and other
void valuePrameterParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root, Symbol* funcSymbol);
void assignParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);
void scanfParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);
void printfParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);
int returnParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);

Symbol* lookTable(string name, int level);

void printError(int line, char errorCode);

void syntaxParse(list<struct Lexeme> tokenList) {
  SyntaxNode * root = new SyntaxNode("<程序>", "", "");
  err = fopen("error.txt", "w");
  out  = fopen("semi-code.txt","w");
  buildSyntaxTree(tokenList ,root);
  fclose(out);
  fclose(err);
  // out = fopen("output.txt", "w");
  // printTree(root);
  symbolMap.insert({ "global", symbolTable[0] });
}

void buildSyntaxTree(list<struct Lexeme> tokenList, SyntaxNode * root) {
  list<struct Lexeme>::iterator iter = tokenList.begin();
  Status status = parseConst;
  while (iter != tokenList.end()) {
    // check whether is "常量说明"
    if (iter->token == "CONSTTK") {
      SyntaxNode* constRoot = new SyntaxNode("<常量说明>", "", "");
      root->appendChild(constRoot);
      constParse(&iter, constRoot, 0);
      // finish const parse
    } else if (iter->token == "INTTK" || iter->token == "CHARTK") {
      // may be var def or func def
      if (status == parseConst) {
        iter++; // IDENFR
        iter++;
        if (iter->token == "LPARENT") {
          status = parseFunc;
        } else {
          status = parseVar;
        }
        iter--; // now iter is IDENFR
        iter--; // int or char
      }
      if (status == parseVar) {
        SyntaxNode* varRoot = new SyntaxNode("<变量说明>", "", "");
        root->appendChild(varRoot);
        varParse(&iter, varRoot, 0);
        status = parseFunc;
      } else {
        // must be func
        SyntaxNode* funcRoot = new SyntaxNode("<有返回值函数定义>", "", "");
        root->appendChild(funcRoot);
        funcParse(&iter, funcRoot);
      }
    } else {
      // must be VOID func or main
      if ((iter)->token != "VOIDTK") {
        exit(-1);
      }
      iter++;
      if (iter->token == "MAINTK") {
        iter--; // void
        SyntaxNode* mainRoot = new SyntaxNode("<主函数>", "", "");
        root->appendChild(mainRoot);
        mainParse(&iter, mainRoot);
      } else {
        iter--;
        SyntaxNode* voidFuncRoot = new SyntaxNode("<无返回值函数定义>", "", "");
        root->appendChild(voidFuncRoot);
        voidFuncParse(&iter, voidFuncRoot);
      }
    }
  }
}

void appendLeaf(list<struct Lexeme>::iterator* iter, SyntaxNode* root){
  SyntaxNode* temp = new SyntaxNode("", (*iter)->value, (*iter)->token); // const
  if ((*iter)->error) {
    printError((*iter)->lineNumber, 'a');
  }
  (*iter)++; // next lexeme node
  root->appendChild(temp);
}

void constParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root, int level) { // 常量说明
  SyntaxNode* temp;
  while ((*iter)->token == "CONSTTK") {
    appendLeaf(iter, root); // const
    temp = new SyntaxNode("<常量定义>", "", "");
    root->appendChild(temp);
    constDefineParse(iter, temp, level);
    if ((*iter)->token != "SEMICN") {
      (*iter)--;
      printError((*iter)->lineNumber, 'k');
      (*iter)++;
    } else {
      appendLeaf(iter, root); // iter must be ;
    }
  }
}

void constDefineParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root, int level) { // 常量定义
  // iter must be int or char
  int type = (*iter)->token == "INTTK" ? INT : CHAR;
  appendLeaf(iter, root); // int or char
  // iter now should be identifier
  while ((*iter)->token == "IDENFR"){
    Symbol* aSymbol = lookTable((*iter)->value, level);
    if (aSymbol != NULL) {
      printError((*iter)->lineNumber, 'b');
    } else {
      aSymbol = new Symbol;
      aSymbol->kind = CONST;
      aSymbol->name = (*iter)->value;
      aSymbol->type = type;
      symbolTable[level].push_back(aSymbol);
    }
    appendLeaf(iter, root); // IDENFR
    if ((*iter)->token != "ASSIGN") {
      exit(-1);
    }
    appendLeaf(iter, root); // =
    if ((*iter)->token == "CHARCON") {
      aSymbol->remark = "\'" + (*iter)->value + "\'";
      appendLeaf(iter, root); // CHARCON
    } else if ((*iter)->token == "INTCON" || (*iter)->token == "MINU" || (*iter)->token == "PLUS"){
      SyntaxNode* tempRoot = new SyntaxNode("<整数>", "", "");
      root->appendChild(tempRoot);
      int res = numParse(iter, tempRoot);
      aSymbol->remark = to_string(res);
    } else {
      printError((*iter)->lineNumber, 'o');
      (*iter)++; // TODO: should next token
    }
    if ((*iter)->token == "COMMA") {
      appendLeaf(iter, root); // ,
    }
  }
}

int numParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 整数
  int leadOp = -1;
  if ((*iter)->token == "PLUS" || (*iter)->token == "MINU") {
    leadOp = (*iter)->token == "PLUS" ? 1 : 2;
    appendLeaf(iter, root); // + or -
  }
  // if ((*iter)->token != "INTCON") {
  //   exit(-1);
  // }
  SyntaxNode* tempRoot = new SyntaxNode("<无符号整数>", "", "");
  root->appendChild(tempRoot);
  int result = stoi((*iter)->value);
  if (leadOp == 2) {
    result = - result;
  }
  appendLeaf(iter, tempRoot); // INTCON
  return result;
}

bool isVarDefine(list<struct Lexeme>::iterator* iter) {
  if ((*iter)->token != "INTTK" && (*iter)->token != "CHARTK" ) {
    return false;
  }
  (*iter)++; // IDENFR
  if ((*iter)->token != "IDENFR") {
    exit(-1);
  }
  (*iter)++; // (
  if ((*iter)->token == "LPARENT") {
    (*iter)--;
    (*iter)--;
    return false;
  }
  else {
    (*iter)--;
    (*iter)--;
    return true;
  }
}

void varParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root, int level) { // 变量说明
  while (isVarDefine(iter)) {
    SyntaxNode* varDefineRoot = new SyntaxNode("<变量定义>", "", "");
    root->appendChild(varDefineRoot);
    varDefineParse(iter, varDefineRoot, level);
    if ((*iter)->token != "SEMICN") {
      (*iter)--;
      printError((*iter)->lineNumber, 'k');
      (*iter)++;
    } else {
      appendLeaf(iter, root); // ;
    }
  }
}

void varDefineParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root, int level) { // 变量定义
  int type = (*iter)->token == "INTTK" ? INT : CHAR;
  appendLeaf(iter, root); // append int or char
  while ((*iter)->token == "IDENFR") {
    Symbol* aSymbol = lookTable((*iter)->value, level);
    if (aSymbol != NULL) {
      printError((*iter)->lineNumber, 'b');
    }
    aSymbol = new Symbol;
    aSymbol->name = (*iter)->value;
    aSymbol->type = type;
    appendLeaf(iter, root); // IDENFR
    if ((*iter)->token == "LBRACK") {
      appendLeaf(iter, root); // [
      SyntaxNode* unsignNumRoot = new SyntaxNode("<无符号整数>", "", "");
      root->appendChild(unsignNumRoot);
      if ((*iter)->token != "INTCON") {
        exit(-1);
      }
      aSymbol->remark = (*iter)->value;
      appendLeaf(iter, unsignNumRoot); // INTCON
      if ((*iter)->token != "RBRACK") {
        printError((*iter)->lineNumber, 'm');
      } else {
        appendLeaf(iter, root); // ]
      }
      aSymbol->kind = ARRAY;
    } else {
      aSymbol->kind = VAR;
    }
    symbolTable[level].push_back(aSymbol);
    if ((*iter)->token == "COMMA") {
      appendLeaf(iter, root);
    }
  }
}

void funcParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 有返回值函数定义
  SyntaxNode* tempRoot = new SyntaxNode("<声明头部>", "", "");
  Symbol* aSymbol = new Symbol;
  aSymbol->kind = FUNC;
  symbolTable[1].clear();
  root->appendChild(tempRoot);
  declareParse(iter, tempRoot, aSymbol);
  if ((*iter)->token != "LPARENT") {
    exit(-1);
  }
  appendLeaf(iter, root); // (
  tempRoot = new SyntaxNode("<参数表>", "", "");
  root->appendChild(tempRoot);
  parameterParse(iter, tempRoot, aSymbol);
  // 输出中间代码
  fprintf(out, "%s %s ()\n", aSymbol->type == INT ? "int" : "char", aSymbol->name.data());
  symbolTable[0].push_back(aSymbol);
  if ((*iter)->token != "RPARENT") {
    printError((*iter)->lineNumber, 'l');
  } else {
    appendLeaf(iter, root); // )
  }
  if ((*iter)->token != "LBRACE") {
    exit(-1);
  }
  appendLeaf(iter, root); // {
  tempRoot = new SyntaxNode("<复合语句>", "", "");
  root->appendChild(tempRoot);
  complexSentenceParse(iter, tempRoot, true, aSymbol->type);
  if ((*iter)->token != "RBRACE") {
    exit(-1);
  }
  appendLeaf(iter, root); // }
  // 将该函数的局部符号表记下来，要在中间代码转 mips 中使用
  symbolMap.insert({ aSymbol->name, symbolTable[1] });
}

void declareParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root, Symbol* funcSymbol) { // 声明头部
  if ((*iter)->token != "INTTK" && (*iter)->token != "CHARTK") {
    exit(-1);
  }
  funcSymbol->type = (*iter)->token == "INTTK" ? INT : CHAR;
  appendLeaf(iter, root); // int or char
  
  if ((*iter)->token != "IDENFR") {
    exit(-1);
  }
  if (lookTable((*iter)->value, 0) != NULL) {
    printError((*iter)->lineNumber, 'b');
  }
  funcSymbol->name = (*iter)->value;
  appendLeaf(iter, root); // IDENFR
}

void parameterParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root, Symbol* funcSymbol) { // 参数表
  // may be empty
  if ((*iter)->token == "RPARENT") {
    return;
  }
  while ((*iter)->token == "INTTK" || (*iter)->token == "CHARTK") {
    Symbol* aSymbol = new Symbol;
    aSymbol->type = (*iter)->token == "INTTK" ? INT : CHAR;
    funcSymbol->remark.push_back((*iter)->token == "INTTK" ? '0' : '1');
    appendLeaf(iter, root); // int or char
    if ((*iter)->token != "IDENFR") {
      exit(-1);
    }
    if (lookTable((*iter)->value, 1) != NULL) {
      printError((*iter)->lineNumber, 'b');
    }
    aSymbol->name = (*iter)->value;
    aSymbol->kind = Para;
    symbolTable[1].push_back(aSymbol);
    appendLeaf(iter, root); // IDENTFR
    if ((*iter)->token == "COMMA") {
      appendLeaf(iter, root); // ,
    }
  }
}

void complexSentenceParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root, bool needReturn, int type) { // 复合语句
  if ((*iter)->token == "CONSTTK") {
    SyntaxNode* constRoot = new SyntaxNode("<常量说明>", "", "");
    root->appendChild(constRoot);
    constParse(iter, constRoot, 1);
  }
  if ((*iter)->token == "INTTK" || (*iter)->token == "CHARTK") {
    SyntaxNode* varRoot = new SyntaxNode("<变量说明>", "", "");
    root->appendChild(varRoot);
    varParse(iter, varRoot, 1);
  }
  SyntaxNode* sentenceListRoot = new SyntaxNode("<语句列>", "", "");
  root->appendChild(sentenceListRoot);
  bool hasReturn = sentenceListParse(iter, sentenceListRoot, type);
  if (needReturn && !hasReturn) {
    printError((*iter)->lineNumber, 'h');
  }
}

bool sentenceListParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root,int type) { // 语句列
  // may be empty
  bool has = false;
  while ((*iter)->token != "RBRACE") {
    SyntaxNode* sentenceRoot = new SyntaxNode("<语句>", "", "");
    root->appendChild(sentenceRoot);
    if (sentenceParse(iter, sentenceRoot, type)) {
      has = true;
    }
  }
  return has;
}

bool sentenceParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root, int type) { // 语句
  bool has;
  if ((*iter)->token == "IFTK") {
    SyntaxNode* ifRoot = new SyntaxNode("<条件语句>", "", "");
    root->appendChild(ifRoot);
    has = ifParse(iter, ifRoot, type);
  } else if ((*iter)->token == "WHILETK" || (*iter)->token == "DOTK" || (*iter)->token == "FORTK") {
    SyntaxNode* circleRoot = new SyntaxNode("<循环语句>", "", "");
    root->appendChild(circleRoot);
    has = circleParse(iter, circleRoot, type);
  } else if ((*iter)->token == "LBRACE") {
    appendLeaf(iter, root); // {
    SyntaxNode* tempRoot = new SyntaxNode("<语句列>", "", "");
    root->appendChild(tempRoot);
    has = sentenceListParse(iter, tempRoot, type);
    if ((*iter)->token != "RBRACE") {
      exit(-1);
    }
    appendLeaf(iter, root); // }
  } else if ((*iter)->token == "IDENFR") {
    has = false;
    (*iter)++;
    bool isFunc = (*iter)->token == "LPARENT";
    (*iter)--;
    if (!isFunc) {
      //TODO: const should not be assign
      SyntaxNode* tempRoot = new SyntaxNode("<赋值语句>", "", "");
      root->appendChild(tempRoot);
      assignParse(iter, tempRoot);
      if ((*iter)->token != "SEMICN") {
        (*iter)--;
        printError((*iter)->lineNumber, 'k');
        (*iter)++;
      }
      else {
        appendLeaf(iter, root); // ;
      }
    } else {
      Symbol* aSymbol = lookTable((*iter)->value, 0); // func define must in global
      if (aSymbol == NULL) {
        printError((*iter)->lineNumber, 'c');
      }
      if (aSymbol != NULL && aSymbol->kind == FUNC) {
        SyntaxNode* tempRoot = new SyntaxNode("<有返回值函数调用语句>", "", "");
        root->appendChild(tempRoot);
        useFuncParse(iter, tempRoot);
        if ((*iter)->token != "SEMICN") {
          (*iter)--;
          printError((*iter)->lineNumber, 'k');
          (*iter)++;
        } else {
          appendLeaf(iter, root); // ;
        }
      } else {
        SyntaxNode* tempRoot = new SyntaxNode("<无返回值函数调用语句>", "", "");
        root->appendChild(tempRoot);
        useFuncParse(iter, tempRoot);
        if ((*iter)->token != "SEMICN") {
          (*iter)--;
          printError((*iter)->lineNumber, 'k');
          (*iter)++;
        } else {
          appendLeaf(iter, root); // ;
        }
      }
    }
  } else if ((*iter)->token == "SCANFTK") {
    has = false;
    SyntaxNode* tempRoot = new SyntaxNode("<读语句>", "", "");
    root->appendChild(tempRoot);
    scanfParse(iter, tempRoot);
    if ((*iter)->token != "SEMICN") {
      (*iter)--;
      printError((*iter)->lineNumber, 'k');
      (*iter)++;
    } else {
      appendLeaf(iter, root); // ;
    }
  } else if ((*iter)->token == "PRINTFTK") {
    has = false;
    SyntaxNode* tempRoot = new SyntaxNode("<写语句>", "", "");
    root->appendChild(tempRoot);
    printfParse(iter, tempRoot);
    if ((*iter)->token != "SEMICN") {
      (*iter)--;
      printError((*iter)->lineNumber, 'k');
      (*iter)++;
    } else {
      appendLeaf(iter, root); // ;
    }
  } else if ((*iter)->token == "SEMICN") {
    has = false;
    appendLeaf(iter, root); // ;
  } else {
    SyntaxNode* tempRoot = new SyntaxNode("<返回语句>", "", "");
    root->appendChild(tempRoot);
    int returnType = returnParse(iter, tempRoot);
    has = true;
    if (returnType != type) {
      // 不匹配的返回语句
      (*iter)--;
      if (type == -1) {
        printError((*(iter))->lineNumber, 'g');
      } else {
        printError((*(iter))->lineNumber, 'h');
      }
      (*iter)++;
    }
    if ((*iter)->token != "SEMICN") {
      (*iter)--;
      printError((*iter)->lineNumber, 'k');
      (*iter)++;
    }
    else {
      appendLeaf(iter, root); // ;
    }
  }
  return has;
}

bool ifParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root, int type) { // 条件语句
  bool has = false;
  appendLeaf(iter, root); // if
  if ((*iter)->token != "LPARENT") {
    exit(-1);
  }
  appendLeaf(iter, root); // (
  SyntaxNode* tempRoot = new SyntaxNode("<条件>", "", "");
  root->appendChild(tempRoot);
  int elseLabel = ++labelCnt;
  conditionParse(iter, tempRoot, true);
  if ((*iter)->token != "RPARENT") {
    printError((*iter)->lineNumber, 'l');
  } else {
    appendLeaf(iter, root); // )
  }
  tempRoot = new SyntaxNode("<语句>", "", "");
  root->appendChild(tempRoot);
  has = sentenceParse(iter, tempRoot, type);
  int endIf = ++labelCnt;
  fprintf(out, "$j label%d\n", endIf);
  fprintf(out, "label%d:\n", elseLabel);
  if ((*iter)->token == "ELSETK") {
    appendLeaf(iter, root); // else
    tempRoot = new SyntaxNode("<语句>", "", "");
    root->appendChild(tempRoot);
    has = sentenceParse(iter, tempRoot, type) ? has : false;
  } else {
    has = false;
  }
  fprintf(out, "label%d:\n", endIf);
  return has;
}

// 返回时 regCnt 保存条件结果
void conditionParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root, bool isZeroBrach) { // 条件
  SyntaxNode* expressionRoot = new SyntaxNode("<表达式>", "", "");
  root->appendChild(expressionRoot);
  int res = expParse(iter, expressionRoot);
  SyntaxNode* firstPart = expressionRoot;
  if (res != INT) {
    (*iter)--;
    printError((*iter)->lineNumber, 'f');
    (*iter)++;
  }
  if ((*iter)->token == "EQL" || (*iter)->token == "LSS" || (*iter)->token == "LEQ" || (*iter)->token == "GRE" || (*iter)->token == "GEQ" || (*iter)->token == "NEQ") {
    string op = (*iter)->value;
    appendLeaf(iter, root);
    expressionRoot = new SyntaxNode("<表达式>", "", "");
    root->appendChild(expressionRoot);
    res = expParse(iter, expressionRoot);
    if ((op == "==" && isZeroBrach) || (op == "!=" && !isZeroBrach)) {
      // 话句话说 a == b 为假的时候跳转 或者 a != b 为真的时候跳转
      // 即 a != b 跳转
      fprintf(out, "$bne %s %s label%d\n", firstPart->value.data(), expressionRoot->value.data(), labelCnt);
    } else if ((op == "<" && isZeroBrach) || (op == ">" && !isZeroBrach)) {
      // a > b 跳转
      fprintf(out, "$bgt %s %s label%d\n", firstPart->value.data(), expressionRoot->value.data(), labelCnt);
    } else if ((op == "<=" && isZeroBrach) || (op == ">=" && !isZeroBrach)) {
      // a >= b 跳转
      fprintf(out, "$bge %s %s label%d\n", firstPart->value.data(), expressionRoot->value.data(), labelCnt);
    } else if ((op == ">" && isZeroBrach) || (op == "<" && !isZeroBrach)) {
      // a < b 跳转
      fprintf(out, "$blt %s %s label%d\n", firstPart->value.data(), expressionRoot->value.data(), labelCnt);
    } else if ((op == ">=" && isZeroBrach) || (op == "<=" && !isZeroBrach)) {
      // a <= b 跳转
      fprintf(out, "$ble %s %s label%d\n", firstPart->value.data(), expressionRoot->value.data(), labelCnt);
    } else if ((op == "!=" && isZeroBrach) || (op == "==" && !isZeroBrach)) {
      // a == b 跳转
      fprintf(out, "$beq %s %s label%d\n", firstPart->value.data(), expressionRoot->value.data(), labelCnt);
    }
    if (res != INT) {
      (*iter)--;
      printError((*iter)->lineNumber, 'f');
      (*iter)++;
    }
  } else {
    // 只有一个表达式
    if (isZeroBrach) {
      fprintf(out, "$bez %s label%d\n", firstPart->value.data());
    } else {
      fprintf(out, "$bnz %s label%d\n", firstPart->value.data());
    }
  }
}

int expParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 表达式
  int leadOp = -1;
  if ((*iter)->token == "PLUS" || (*iter)->token == "MINU") {
    leadOp = (*iter)->token == "PLUS" ? 1 : 2; // 1: PLUS
    appendLeaf(iter, root); // + or -
  }
  int type;
  int cnt = 1;
  SyntaxNode* singlePoly = NULL;
  string oldOp;
  while (true) {
    SyntaxNode* polyRoot = new SyntaxNode("<项>", "", "");
    root->appendChild(polyRoot);
    int oldCnt = regCnt;
    type = polyParse(iter, polyRoot);
    if (oldCnt == regCnt && cnt == 1) {
      if (leadOp != -1) {
        fprintf(out, "%dt = %c %s\n", ++regCnt, leadOp == 1 ? '+' : '-', polyRoot->getChildren().front()->value.data());
      } else {
        // 第一项没有必要单独输出
        singlePoly = polyRoot;
      }
    } else if (oldCnt == regCnt && cnt == 2) {
      if (singlePoly != NULL) {
        // 第一项也没有必要输出
        fprintf(out, "%dt = %s %s %s\n", ++regCnt, singlePoly->getChildren().front()->value.data(),oldOp.data(), polyRoot->getChildren().front()->value.data());
      } else {
        // 第一项有输出
        fprintf(out, "%dt = %dt %s %s\n", regCnt + 1, regCnt,oldOp.data(), polyRoot->getChildren().front()->value.data());
        regCnt++;
      }
    } else if (oldCnt == regCnt) {
      fprintf(out, "%dt = %dt %s %s\n", regCnt + 1, regCnt,oldOp.data(), polyRoot->getChildren().front()->value.data());
      regCnt++;
    } else {
      // 当前项有输出
      if (cnt == 2 && singlePoly != NULL) {
        // 当前项为第二项有输出，且第一项未输出
        fprintf(out, "%dt = %s %s %dt\n", regCnt + 1, singlePoly->getChildren().front()->value.data(),oldOp.data(), regCnt);
        regCnt++;
      } else if (cnt != 1) {
        fprintf(out, "%dt = %dt %s %dt\n", regCnt + 1, oldCnt,oldOp.data(), regCnt);
        regCnt++;
      } else {
        // 第一项且有输出
        if (leadOp != -1) {
          fprintf(out, "%dt = %c %dt\n", regCnt + 1, leadOp == 1 ? '+' : '-', regCnt);
        }
      }
    }
    if ((*iter)->token == "PLUS" || (*iter)->token == "MINU") {
      oldOp = (*iter)->value;
      appendLeaf(iter, root); // + or -
      cnt++;
    } else {
      break;
    }
  }
  if (cnt == 1 && singlePoly != NULL) {
    // 第一项没有输出，且只有一项的表达式
    root->value = singlePoly->getChildren().front()->value; // 将因子的值传递给表达式
  } else {
    root->value = to_string(regCnt) + "t";
  }
  if (cnt == 1 && type == CHAR) {
    return CHAR;
  } else {
    return INT;
  }
}

int polyParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 项
  int type;
  int cnt = 1;
  SyntaxNode* firstFactor = NULL;
  string oldOp;
  while (true) {
    SyntaxNode* factorRoot = new SyntaxNode("<因子>", "", "");
    root->appendChild(factorRoot);
    int oldCnt = regCnt;
    type = factorParse(iter, factorRoot);
    if (oldCnt == regCnt) {
      // 这个因子没有输出
      if (cnt == 1) {
        // 第一个因子没有输出, 不做任何操作
        firstFactor = factorRoot;
      } else if (cnt == 2) {
        if (firstFactor != NULL) {
          fprintf(out, "%dt = %s %s %s\n", ++regCnt, firstFactor->value.data(),oldOp.data(), factorRoot->value.data());
        } else {
          fprintf(out, "%dt = %dt %s %s\n", ++regCnt, oldCnt,oldOp.data(), factorRoot->value.data());
        }
      } else {
        fprintf(out, "%dt = %dt %s %s\n", ++regCnt, oldCnt,oldOp.data(), factorRoot->value.data());
      }
    } else {
      // 这个因子有输出
      if (cnt == 2) {
        if (firstFactor != NULL) {
          fprintf(out, "%dt = %s %s %dt\n", regCnt + 1, firstFactor->value.data(),oldOp.data(), regCnt);
          regCnt++;
        } else {
          fprintf(out, "%dt = %dt %s %dt\n", regCnt + 1, oldCnt,oldOp.data(), regCnt);
          regCnt++;
        }
      } else if (cnt != 1) {
        fprintf(out, "%dt = %dt %s %dt\n", regCnt + 1, oldCnt,oldOp.data(), regCnt);
        regCnt++;
      }
    }
    if ((*iter)->token == "MULT" || (*iter)->token == "DIV") {
      oldOp = (*iter)->value;
      appendLeaf(iter, root);
      cnt++;
    } else {
      break;
    }
  }
  if (cnt == 1 && type == CHAR) {
    return CHAR;
  } else {
    return INT;
  }
}

int factorParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 因子
  int type;
  if ((*iter)->token == "IDENFR") {
    (*iter)++;
    bool isFunc = (*iter)->token == "LPARENT";
    (*iter)--;
    if (isFunc) {
      Symbol* funcSymbol = lookTable((*iter)->value, 0);
      if (funcSymbol == NULL) {
        // 未定义的函数调用
        type = INT;
      } else {
        type = funcSymbol->type;
      }
      SyntaxNode* funcRoot = new SyntaxNode("<有返回值函数调用语句>", "", "");
      root->appendChild(funcRoot);
      useFuncParse(iter, funcRoot);
      fprintf(out, "%dt = RET\n", ++regCnt);
    } else {
      Symbol* aSymbol = lookTable((*iter)->value, 1);
      if (aSymbol == NULL) {
        aSymbol = lookTable((*iter)->value, 0);
        if (aSymbol == NULL) {
          printError((*iter)->lineNumber, 'c');
        }
      }
      type = aSymbol == NULL ? INT : aSymbol->type;
      string iden = (*iter)->value;
      appendLeaf(iter, root); // IDENFR
      if ((*iter)->token == "LBRACK") {
        appendLeaf(iter, root); // [
        SyntaxNode* expRoot = new SyntaxNode("<表达式>", "", "");
        root->appendChild(expRoot);
        int res = expParse(iter, expRoot);
        fprintf(out, "%dt = %s[%s]\n", ++regCnt, iden.data(), expRoot->value.data());
        if (res == CHAR) {
          printError((*iter)->lineNumber, 'i');
        }
        if ((*iter)->token != "RBRACK") {
          printError((*iter)->lineNumber, 'm');
        } else {
          appendLeaf(iter, root); // ]
        }
      } else {
        // 因子为单一标识符，不需要输出
        root->value = iden;
      }
    }
  } else if ((*iter)->token == "LPARENT") {
    appendLeaf(iter, root); // (
    SyntaxNode* expRoot = new SyntaxNode("<表达式>", "", "");
    root->appendChild(expRoot);
    expParse(iter, expRoot);
    type = INT;
    root->value = expRoot->value; // TODO: 觉得这里没有必要将 ('c') 转化为 int
    if ((*iter)->token != "RPARENT") {
      printError((*iter)->lineNumber, 'l');
    } else {
      appendLeaf(iter, root); // )
    }
  } else if ((*iter)->token == "CHARCON") {
    type = CHAR;
    root->value = "\'"  + (*iter)->value + "\'";
    appendLeaf(iter, root);
  } else {
    type = INT;
    SyntaxNode* tempRoot = new SyntaxNode("<整数>", "", "");
    root->appendChild(tempRoot);
    int res = numParse(iter, tempRoot);
    root->value = to_string(res);
  }
  return type;
}

bool circleParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root, int type){ // 循环
  bool has = false;
  if ((*iter)->token == "WHILETK") {
    appendLeaf(iter, root); // while
    int whileLabel = ++labelCnt;
    fprintf(out, "label%d:\n", whileLabel);
    if ((*iter)->token != "LPARENT") {
      exit(-1);
    }
    appendLeaf(iter, root); // (
    SyntaxNode* tempRoot = new SyntaxNode("<条件>", "", "");
    root->appendChild(tempRoot);
    int endWhile = ++labelCnt;
    conditionParse(iter, tempRoot, true);
    if ((*iter)->token != "RPARENT") {
      printError((*iter)->lineNumber, 'l');
    } else {
      appendLeaf(iter, root); // )
    }
    tempRoot = new SyntaxNode("<语句>", "", "");
    root->appendChild(tempRoot);
    sentenceParse(iter, tempRoot, type);
    fprintf(out, "$j label%d\n", whileLabel);
    fprintf(out, "label%d\n", endWhile);
  } else if ((*iter)->token == "DOTK") {
    appendLeaf(iter, root); // do
    int doLabel = ++labelCnt;
    fprintf(out, "label%d\n", doLabel);
    SyntaxNode* tempRoot = new SyntaxNode("<语句>", "", "");
    root->appendChild(tempRoot);
    has = sentenceParse(iter, tempRoot, type);
    if ((*iter)->token != "WHILETK") {
      printError((*iter)->lineNumber, 'n');
    } else {
      appendLeaf(iter, root); // while
    }
    if ((*iter)->token != "LPARENT") {
      exit(-1);
    }
    appendLeaf(iter, root); // (
    tempRoot = new SyntaxNode("<条件>", "", "");
    root->appendChild(tempRoot);
    conditionParse(iter, tempRoot, false);
    if ((*iter)->token != "RPARENT") {
      printError((*iter)->lineNumber, 'l');
    } else {
      appendLeaf(iter, root); // )
    }
  } else if ((*iter)->token == "FORTK") {
    appendLeaf(iter, root); // for
    if ((*iter)->token != "LPARENT") {
      exit(-1);
    }
    appendLeaf(iter, root); // (
    if ((*iter)->token != "IDENFR") {
      exit(-1);
    }
    if (lookTable((*iter)->value, 1) == NULL && lookTable((*iter)->value, 0) == NULL) {
      printError((*iter)->lineNumber, 'c');
    }
    string iden = (*iter)->value;
    appendLeaf(iter, root); // IDENFR
    if ((*iter)->token != "ASSIGN") {
      exit(-1);
    }
    appendLeaf(iter, root); // =
    SyntaxNode* tempRoot = new SyntaxNode("<表达式>", "", "");
    root->appendChild(tempRoot);
    expParse(iter, tempRoot);
    fprintf(out, "%s = %s\n", iden.data(), tempRoot->value.data());
    int forLabel = ++labelCnt;
    fprintf(out, "label%d:\n", forLabel);
    if ((*iter)->token != "SEMICN") {
      printError((*iter)->lineNumber, 'k');
    } else {
      appendLeaf(iter, root); // ;
    }
    tempRoot = new SyntaxNode("<条件>", "", "");
    root->appendChild(tempRoot);
    int endFor = ++labelCnt;
    conditionParse(iter, tempRoot, true);
    if ((*iter)->token != "SEMICN") {
      printError((*iter)->lineNumber, 'k');
    } else {
      appendLeaf(iter, root); // ;
    }
    if ((*iter)->token != "IDENFR") {
      exit(-1);
    }
    if (lookTable((*iter)->value, 1) == NULL && lookTable((*iter)->value, 0) == NULL) {
      printError((*iter)->lineNumber, 'c');
    }
    iden = (*iter)->value;
    appendLeaf(iter, root); // IDENFR
    if ((*iter)->token != "ASSIGN") {
      exit(-1);
    }
    appendLeaf(iter, root); // =
    if ((*iter)->token != "IDENFR") {
      exit(-1);
    }
    if (lookTable((*iter)->value, 1) == NULL && lookTable((*iter)->value, 0) == NULL) {
      printError((*iter)->lineNumber, 'c');
    }
    string iden2 = (*iter)->value;
    appendLeaf(iter, root); // IDENFR
    if ((*iter)->token != "PLUS" && (*iter)->token != "MINU") {
      exit(-1);
    }
    char op = (*iter)->token == "PLUS" ? '+' : '-';
    appendLeaf(iter, root); // +/-
    tempRoot = new SyntaxNode("<步长>", "", "");
    root->appendChild(tempRoot);
    int res = stepParse(iter, tempRoot);
    if ((*iter)->token != "RPARENT") {
      printError((*iter)->lineNumber, 'l');
    } else {
      appendLeaf(iter, root); // )
    }
    tempRoot = new SyntaxNode("<语句>", "", "");
    root->appendChild(tempRoot);
    sentenceParse(iter, tempRoot, type);
    fprintf(out, "%s = %s %c %d\n", iden.data(), iden2.data(), op, res);
    fprintf(out, "$j label%d\n", forLabel);
    fprintf(out, "label%d:\n", endFor);
  }
  // 因为不知道 while for 语句会不会执行故无法判断有无 return
  return has;
}

int stepParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 步长
  SyntaxNode* unsignedNum = new SyntaxNode("<无符号整数>", "", "");
  root->appendChild(unsignedNum);
  if ((*iter)->token != "INTCON") {
    exit(-1);
  }
  int res = stoi((*iter)->value);
  appendLeaf(iter, unsignedNum); // INTCON
  return res;
}

void useFuncParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 函数调用
  Symbol* aSymbol = lookTable((*iter)->value, 0);
  if (aSymbol == NULL) {
    printError((*iter)->lineNumber, 'c');
  }
  string iden = (*iter)->value;
  appendLeaf(iter, root); // IDENFR
  if ((*iter)->token != "LPARENT") {
    exit(-1);
  }
  appendLeaf(iter, root); // (
  fprintf(out, "$save all %s\n", iden.data());
  SyntaxNode* tempRoot = new SyntaxNode("<值参数表>", "", "");
  root->appendChild(tempRoot);
  valuePrameterParse(iter, tempRoot, aSymbol);
  fprintf(out, "$call %s\n", iden.data());
  if ((*iter)->token != "RPARENT") {
    printError((*iter)->lineNumber, 'l');
  } else {
    appendLeaf(iter, root); // )
  }
}

void valuePrameterParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root, Symbol* funcSymbol) { // 值参数表
  if ((*iter)->token == "RPARENT") {
    // may be empty
    if (funcSymbol != NULL && funcSymbol->remark.length() != 0) {
      printError((*iter)->lineNumber, 'd');
    }
    return;
  }
  string s;
  int cnt = 0;
  while (true) {
    SyntaxNode* tempRoot = new SyntaxNode("<表达式>", "", "");
    root->appendChild(tempRoot);
    int res = expParse(iter, tempRoot);
    fprintf(out, "$push %s %d\n", tempRoot->value.data(), ++cnt);
    s.push_back(res == INT ? '0' : '1');
    if ((*iter)->token == "COMMA") {
      appendLeaf(iter, root);
    } else {
      break;
    }
  }
  if (funcSymbol == NULL) {
    return;
  }
  if (s.length() != funcSymbol->remark.length()) {
    printError((*iter)->lineNumber, 'd');
  } else if (s != funcSymbol->remark) {
    printError((*iter)->lineNumber, 'e');
  }
}

void assignParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 赋值语句
  Symbol* aSymbol = lookTable((*iter)->value, 1);
  if (aSymbol == NULL) {
    aSymbol = lookTable((*iter)->value, 0);
    if (aSymbol == NULL) {
      printError((*iter)->lineNumber, 'c');
    }
  }
  if (aSymbol->kind == CONST) {
    printError((*iter)->lineNumber, 'j');
  }
  string iden = (*iter)->value;
  string expValue = "";
  appendLeaf(iter, root); // IDENFR
  if ((*iter)->token == "LBRACK") {
    appendLeaf(iter, root); // [
    SyntaxNode* tempRoot = new SyntaxNode("<表达式>", "", "");
    root->appendChild(tempRoot);
    if (expParse(iter, tempRoot) == CHAR) {
      printError((*iter)->lineNumber, 'i');
    }
    expValue = tempRoot->value;
    if ((*iter)->token != "RBRACK") {
      printError((*iter)->lineNumber, 'm');
    } else {
      appendLeaf(iter, root); // ]
    }
  }
  if ((*iter)->token != "ASSIGN") {
    exit(-1);
  }
  appendLeaf(iter, root); // =
  SyntaxNode* expRoot = new SyntaxNode("<表达式>", "", "");
  root->appendChild(expRoot);
  expParse(iter, expRoot);
  if (expValue != "") {
    fprintf(out, "%s[%s] = %s\n", iden.data(), expValue.data(), expRoot->value.data());
  } else {
    fprintf(out, "%s = %s\n", iden.data(), expRoot->value.data());
  }
}

void scanfParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 读语句
  if ((*iter)->token != "SCANFTK") {
    exit(-1);
  }
  appendLeaf(iter, root); // scanf
  if ((*iter)->token != "LPARENT") {
    exit(-1);
  }
  appendLeaf(iter, root); // (
  while (true) {
    if ((*iter)->token != "IDENFR") {
      exit(-1);
    }
    Symbol* iden = lookTable((*iter)->value, 1);
    if (iden == NULL) {
      iden = lookTable((*iter)->value, 0);
      if (iden == NULL) {
        printError((*iter)->lineNumber, 'c');
      }
    }
    if (iden != NULL) {
      if (iden->type == INT) {
        fprintf(out, "scanf int %s\n", iden->name.data());
      } else {
        fprintf(out, "scanf char %s\n", iden->name.data());
      }
    }
    appendLeaf(iter, root); // IDENFR
    if ((*iter)->token != "COMMA") {
      break;
    } else {
      appendLeaf(iter, root); // ,
    }
  }
  if ((*iter)->token != "RPARENT") {
    printError((*iter)->lineNumber, 'l');
  } else {
    appendLeaf(iter, root); // )
  }
}

void printfParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 写语句
  appendLeaf(iter, root); // printf
  if ((*iter)->token != "LPARENT") {
    exit(-1);
  }
  appendLeaf(iter, root); // (
  if ((*iter)->token == "STRCON") {
    SyntaxNode* tempRoot = new SyntaxNode("<字符串>", "", "");
    root->appendChild(tempRoot);
    int strNo = ++strCnt;
    strMap.insert({strNo, (*iter)->value});
    appendLeaf(iter, tempRoot); // STRCON
    fprintf(out, "$print str%d\n", strNo);
    if ((*iter)->token == "COMMA") {
      appendLeaf(iter, root);
      tempRoot = new SyntaxNode("<表达式>", "", "");
      root->appendChild(tempRoot);
      int type = expParse(iter, tempRoot);
      if (type == INT) {
        fprintf(out, "$print int %s\n", tempRoot->value.data());
      } else {
        fprintf(out, "$print char %s\n", tempRoot->value.data());
      }
    }
    fprintf(out, "$print newline\n");
  } else {
    SyntaxNode* tempRoot = new SyntaxNode("<表达式>", "", "");
    root->appendChild(tempRoot);
    int type =expParse(iter, tempRoot);
    if (type == INT) {
      fprintf(out, "$print int %s\n", tempRoot->value.data());
    } else {
      fprintf(out, "$print char %s\n", tempRoot->value.data());
    }
    fprintf(out, "$print newline\n");
  }
  if ((*iter)->token != "RPARENT") {
    printError((*iter)->lineNumber, 'l');
  } else {
    appendLeaf(iter, root); // )
  }
}

int returnParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 返回语句
  int type = -1;
  appendLeaf(iter, root); // return
  if ((*iter)->token == "LPARENT") {
    appendLeaf(iter, root); // (
    SyntaxNode* tempRoot = new SyntaxNode("<表达式>", "", "");
    root->appendChild(tempRoot);
    type = expParse(iter, tempRoot);
    fprintf(out, "return %s\n", tempRoot->value.data());
    if ((*iter)->token != "RPARENT") {
      printError((*iter)->lineNumber, 'l');
    } else {
      appendLeaf(iter, root); // )
    }
  }
  return type;
}

void mainParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 主函数
  appendLeaf(iter, root); // void
  if ((*iter)->token != "MAINTK") {
    exit(-1);
  }
  Symbol* aSymbol = new Symbol;
  aSymbol->name = "main";
  aSymbol->kind = VOID;
  symbolTable[0].push_back(aSymbol);
  fprintf(out, "void main ()\n");
  symbolTable[1].clear();
  appendLeaf(iter, root); // main
  if ((*iter)->token != "LPARENT") {
    exit(-1);
  }
  appendLeaf(iter, root); // (
  if ((*iter)->token != "RPARENT") {
    printError((*iter)->lineNumber, 'l');
  } else {
    appendLeaf(iter, root); // )
  }
  if ((*iter)->token != "LBRACE") {
    exit(-1);
  }
  appendLeaf(iter, root); // {
  SyntaxNode* tempRoot = new SyntaxNode("<复合语句>", "", "");
  root->appendChild(tempRoot);
  complexSentenceParse(iter, tempRoot, false, -1);
  fprintf(out, "return\n");
  if ((*iter)->token != "RBRACE") {
    exit(-1);
  }
  appendLeaf(iter, root); // }
  symbolMap.insert({ aSymbol->name, symbolTable[1] });
}

void voidFuncParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 无返回值函数定义
  appendLeaf(iter, root); // void
  Symbol* aSymbol = new Symbol;
  aSymbol->kind = VOID;
  symbolTable[1].clear(); // clear local symbol table
  if ((*iter)->token != "IDENFR") {
    exit(-1);
  }
  if (lookTable((*iter)->value, 0) != NULL) {
    printError((*iter)->lineNumber, 'b');
  }
  aSymbol->name = (*iter)->value;
  appendLeaf(iter, root); // IDENFR
  if ((*iter)->token != "LPARENT") {
    exit(-1);
  }
  appendLeaf(iter, root); // (
  SyntaxNode* tempRoot = new SyntaxNode("<参数表>", "", "");
  root->appendChild(tempRoot);
  parameterParse(iter, tempRoot, aSymbol);
  symbolTable[0].push_back(aSymbol);
  fprintf(out, "void %s ()\n", aSymbol->name.data());
  if ((*iter)->token != "RPARENT") {
    printError((*iter)->lineNumber, 'l');
  } else {
    appendLeaf(iter, root); // )
  }
  if ((*iter)->token != "LBRACE") {
    exit(-1);
  }
  appendLeaf(iter, root); // {
  tempRoot = new SyntaxNode("<复合语句>", "", "");
  root->appendChild(tempRoot);
  complexSentenceParse(iter, tempRoot, false, -1);
  fprintf(out, "return\n"); // 无返回值函数调用不一定有 return 但我需要告诉汇编这个函数结束了
  if ((*iter)->token != "RBRACE") {
    exit(-1);
  }
  appendLeaf(iter, root); // }
  symbolMap.insert({ aSymbol->name, symbolTable[1] });
}

void printError(int line, char errorCode) {
  fprintf(err, "%d %c\n", line, errorCode);
}

Symbol* lookTable(string name, int level) {
  list<Symbol*>::iterator iter = symbolTable[level].begin();
  while (iter != symbolTable[level].end()) {
    if ((*iter)->name == name) {
      return *iter;
    }
    iter++;
  }
  return NULL;
}