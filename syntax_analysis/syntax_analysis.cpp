#include "syntax_analysis.hpp"
#include <map>
#include <cstdio>

#define VOID_FUNC 0
#define FUNC 1
#define VAR 2
#define CONST 3

FILE* out;

map<string, int> funcMap;
map<string, int> varConstMap;

void buildSyntaxTree(list<struct Lexeme> tokenList, SyntaxNode * root);
void appendLeaf(list<struct Lexeme>::iterator* iter, SyntaxNode* root);

void constParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);
void constDefineParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);
void numParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);

void varParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);
void varDefineParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);

void funcParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);
void declareParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);
void parameterParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);
void complexSentenceParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);

void mainParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);
void voidFuncParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);

void sentenceListParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);
void sentenceParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);
void ifParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);
void conditionParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);
void expParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);
void polyParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);
void factorParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);
void circleParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);
void stepParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);
void useFuncParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root); // include void and other
void valuePrameterParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);
void assignParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);
void scanfParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);
void printfParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);
void returnParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root);

void printTree(SyntaxNode* root);

void syntaxParse(list<struct Lexeme> tokenList) {
  SyntaxNode * root = new SyntaxNode("<程序>", "", "");
  buildSyntaxTree(tokenList ,root);
  out = fopen("output.txt", "w");
  printTree(root);
}

void buildSyntaxTree(list<struct Lexeme> tokenList, SyntaxNode * root) {
  list<struct Lexeme>::iterator iter = tokenList.begin();
  Status status = parseConst;
  while (iter != tokenList.end()) {
    // check whether is "常量说明"
    if (iter->value == "const") {
      SyntaxNode* constRoot = new SyntaxNode("<常量说明>", "", "");
      root->appendChild(constRoot);
      constParse(&iter, constRoot);
      // finish const parse
    } else if (iter->value == "int" || iter->value == "char") {
      // may be var def or func def
      if (status == parseConst) {
        iter++; // IDENFR
        iter++;
        if (iter->value == "(") {
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
        varParse(&iter, varRoot);
        status = parseFunc;
      } else {
        // must be func
        SyntaxNode* funcRoot = new SyntaxNode("<有返回值函数定义>", "", "");
        root->appendChild(funcRoot);
        funcParse(&iter, funcRoot);
      }
    } else {
      // must be VOID func or main
      if ((iter)->value != "void") {
        exit(-1);
      }
      iter++;
      if (iter->value == "main") {
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
  (*iter)++; // next lexeme node
  root->appendChild(temp);
}

void constParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 常量说明
  SyntaxNode* temp;
  while ((*iter)->token == "CONSTTK") {
    appendLeaf(iter, root); // const
    temp = new SyntaxNode("<常量定义>", "", "");
    root->appendChild(temp);
    constDefineParse(iter, temp);
    if ((*iter)->token != "SEMICN") {
      exit(-1);
    }
    appendLeaf(iter, root); // iter must be ;
  }
}

void constDefineParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 常量定义
  // iter must be int or char
  appendLeaf(iter, root); // int or char
  // iter now should be identifier
  while ((*iter)->token == "IDENFR"){
    varConstMap.insert({(*iter)->value, CONST});
    appendLeaf(iter, root); // IDENFR
    if ((*iter)->token != "ASSIGN") {
      exit(-1);
    }
    appendLeaf(iter, root); // =
    if ((*iter)->token == "CHARCON") {
      appendLeaf(iter, root); // CHARCON
    } else {
      SyntaxNode* tempRoot = new SyntaxNode("<整数>", "", "");
      root->appendChild(tempRoot);
      numParse(iter, tempRoot);
    }
    if ((*iter)->token == "COMMA") {
      appendLeaf(iter, root); // ,
    }
  }
}

void numParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 整数 TODO: 有问题
  if ((*iter)->token == "PLUS" || (*iter)->token == "MINU") {
    appendLeaf(iter, root); // + or -
  }
  // if ((*iter)->token != "INTCON") {
  //   exit(-1);
  // }
  SyntaxNode* tempRoot = new SyntaxNode("<无符号整数>", "", "");
  root->appendChild(tempRoot);
  appendLeaf(iter, tempRoot); // INTCON
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

void varParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 变量说明
  while (isVarDefine(iter)) {
    SyntaxNode* varDefineRoot = new SyntaxNode("<变量定义>", "", "");
    root->appendChild(varDefineRoot);
    varDefineParse(iter, varDefineRoot);
    if ((*iter)->token != "SEMICN") {
      exit(-1);
    }
    appendLeaf(iter, root); // ;
  }
}

void varDefineParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 变量定义
  appendLeaf(iter, root); // append int or char
  while ((*iter)->token == "IDENFR") {
    varConstMap.insert({(*iter)->value, VAR});
    appendLeaf(iter, root); // IDENFR
    if ((*iter)->token == "LBRACK") {
      appendLeaf(iter, root); // [
      SyntaxNode* unsignNumRoot = new SyntaxNode("<无符号整数>", "", "");
      root->appendChild(unsignNumRoot);
      if ((*iter)->token != "INTCON") {
        exit(-1);
      }
      appendLeaf(iter, unsignNumRoot); // INTCON
      if ((*iter)->token != "RBRACK") {
        exit(-1);
      }
      appendLeaf(iter, root); // ]
    }
    if ((*iter)->token == "COMMA") {
      appendLeaf(iter, root);
    }
  }
}

void funcParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 有返回值函数定义
  SyntaxNode* tempRoot = new SyntaxNode("<声明头部>", "", "");
  root->appendChild(tempRoot);
  declareParse(iter, tempRoot);
  if ((*iter)->token != "LPARENT") {
    exit(-1);
  }
  appendLeaf(iter, root); // (
  tempRoot = new SyntaxNode("<参数表>", "", "");
  root->appendChild(tempRoot);
  parameterParse(iter, tempRoot);
  if ((*iter)->token != "RPARENT") {
    exit(-1);
  }
  appendLeaf(iter, root); // )
  if ((*iter)->token != "LBRACE") {
    exit(-1);
  }
  appendLeaf(iter, root); // {
  tempRoot = new SyntaxNode("<复合语句>", "", "");
  root->appendChild(tempRoot);
  complexSentenceParse(iter, tempRoot);
  if ((*iter)->token != "RBRACE") {
    exit(-1);
  }
  appendLeaf(iter, root); // }
}

void declareParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 声明头部
  if ((*iter)->token != "INTTK" && (*iter)->token != "CHARTK") {
    exit(-1);
  }
  appendLeaf(iter, root); // int
  funcMap.insert({(*iter)->value, FUNC});
  if ((*iter)->token != "IDENFR") {
    exit(-1);
  }
  appendLeaf(iter, root); // IDENFR
}

void parameterParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 参数表
  // may be empty
  if ((*iter)->token == "RPARENT") {
    return;
  }
  while ((*iter)->token == "INTTK" || (*iter)->token == "CHARTK") {
    appendLeaf(iter, root); // int or char
    varConstMap.insert({(*iter)->value, VAR});
    if ((*iter)->token != "IDENFR") {
      exit(-1);
    }
    appendLeaf(iter, root); // IDENTFR
    if ((*iter)->token == "COMMA") {
      appendLeaf(iter, root); // ,
    }
  }
}

void complexSentenceParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 复合语句
  if ((*iter)->token == "CONSTTK") {
    SyntaxNode* constRoot = new SyntaxNode("<常量说明>", "", "");
    root->appendChild(constRoot);
    constParse(iter, constRoot);
  }
  if ((*iter)->token == "INTTK" || (*iter)->token == "CHARTK") {
    SyntaxNode* varRoot = new SyntaxNode("<变量说明>", "", "");
    root->appendChild(varRoot);
    varParse(iter, varRoot);
  }
  SyntaxNode* sentenceListRoot = new SyntaxNode("<语句列>", "", "");
  root->appendChild(sentenceListRoot);
  sentenceListParse(iter, sentenceListRoot); 
}

void sentenceListParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 语句列
  // may be empty
  while ((*iter)->token != "RBRACE") {
    SyntaxNode* sentenceRoot = new SyntaxNode("<语句>", "", "");
    root->appendChild(sentenceRoot);
    sentenceParse(iter, sentenceRoot);
  }
}

void sentenceParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 语句
  if ((*iter)->token == "IFTK") {
    SyntaxNode* ifRoot = new SyntaxNode("<条件语句>", "", "");
    root->appendChild(ifRoot);
    ifParse(iter, ifRoot);
  } else if ((*iter)->token == "WHILETK" || (*iter)->token == "DOTK" || (*iter)->token == "FORTK") {
    SyntaxNode* circleRoot = new SyntaxNode("<循环语句>", "", "");
    root->appendChild(circleRoot);
    circleParse(iter, circleRoot);
  } else if ((*iter)->token == "LBRACE") {
    appendLeaf(iter, root); // {
    SyntaxNode* tempRoot = new SyntaxNode("<语句列>", "", "");
    root->appendChild(tempRoot);
    sentenceListParse(iter, tempRoot);
    if ((*iter)->token != "RBRACE") {
      exit(-1);
    }
    appendLeaf(iter, root); // }
  } else if ((*iter)->token == "IDENFR") {
    (*iter)++;
    bool isFunc = (*iter)->token == "LPARENT";
    (*iter)--;
    if (!isFunc) {
      //TODO: const should not be assign
      SyntaxNode* tempRoot = new SyntaxNode("<赋值语句>", "", "");
      root->appendChild(tempRoot);
      assignParse(iter, tempRoot);
      appendLeaf(iter, root); // ;
    } else {
      auto search = funcMap.find((*iter)->value);
      if (search->second == FUNC) {
        SyntaxNode* tempRoot = new SyntaxNode("<有返回值函数调用语句>", "", "");
        root->appendChild(tempRoot);
        useFuncParse(iter, tempRoot);
        if ((*iter)->token != "SEMICN") {
          exit(-1);
        }
        appendLeaf(iter, root); // ;
      } else {
        SyntaxNode* tempRoot = new SyntaxNode("<无返回值函数调用语句>", "", "");
        root->appendChild(tempRoot);
        useFuncParse(iter, tempRoot);
        if ((*iter)->token != "SEMICN") {
          exit(-1);
        }
        appendLeaf(iter, root); // ;
      }
    }
  } else if ((*iter)->token == "SCANFTK") {
    SyntaxNode* tempRoot = new SyntaxNode("<读语句>", "", "");
    root->appendChild(tempRoot);
    scanfParse(iter, tempRoot);
    if ((*iter)->token != "SEMICN") {
      exit(-1);
    }
    appendLeaf(iter, root); // ;
  } else if ((*iter)->token == "PRINTFTK") {
    SyntaxNode* tempRoot = new SyntaxNode("<写语句>", "", "");
    root->appendChild(tempRoot);
    printfParse(iter, tempRoot);
    if ((*iter)->token != "SEMICN") {
      exit(-1);
    }
    appendLeaf(iter, root); // ;
  } else if ((*iter)->token == "SEMICN") {
    appendLeaf(iter, root); // ;
  } else {
    // if ((*iter)->value != "return") {
    //   exit(-1);
    // }
    SyntaxNode* tempRoot = new SyntaxNode("<返回语句>", "", "");
    root->appendChild(tempRoot);
    returnParse(iter, tempRoot);
    appendLeaf(iter, root); // ;
  }
}

void ifParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 条件语句
  appendLeaf(iter, root); // if
  if ((*iter)->token != "LPARENT") {
    exit(-1);
  }
  appendLeaf(iter, root); // (
  SyntaxNode* tempRoot = new SyntaxNode("<条件>", "", "");
  root->appendChild(tempRoot);
  conditionParse(iter, tempRoot);
  if ((*iter)->token != "RPARENT") {
    exit(-1);
  }
  appendLeaf(iter, root); // )
  tempRoot = new SyntaxNode("<语句>", "", "");
  root->appendChild(tempRoot);
  sentenceParse(iter, tempRoot);
  if ((*iter)->token == "ELSETK") {
    appendLeaf(iter, root); // else
    tempRoot = new SyntaxNode("<语句>", "", "");
    root->appendChild(tempRoot);
    sentenceParse(iter, tempRoot);
  }
}

void conditionParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 条件
  SyntaxNode* expressionRoot = new SyntaxNode("<表达式>", "", "");
  root->appendChild(expressionRoot);
  expParse(iter, expressionRoot);
  if ((*iter)->token == "EQL" || (*iter)->token == "LSS" || (*iter)->token == "LEQ" || (*iter)->token == "GRE" || (*iter)->token == "GEQ" || (*iter)->token == "NEQ") {
    appendLeaf(iter, root);
    expressionRoot = new SyntaxNode("<表达式>", "", "");
    root->appendChild(expressionRoot);
    expParse(iter, expressionRoot);
  }
}

void expParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 表达式
  if ((*iter)->token == "PLUS" || (*iter)->token == "MINU") {
    appendLeaf(iter, root); // + or -
  }
  while (true) {
    SyntaxNode* polyRoot = new SyntaxNode("<项>", "", "");
    root->appendChild(polyRoot);
    polyParse(iter, polyRoot);
    if ((*iter)->token == "PLUS" || (*iter)->token == "MINU") {
      appendLeaf(iter, root); // + or -
    } else {
      break;
    }
  }
}

void polyParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 项
  while (true) {
    SyntaxNode* factorRoot = new SyntaxNode("<因子>", "", "");
    root->appendChild(factorRoot);
    factorParse(iter, factorRoot);
    if ((*iter)->token == "MULT" || (*iter)->token == "DIV") {
      appendLeaf(iter, root);
    } else {
      break;
    }
  }
}

void factorParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 因子
  if ((*iter)->token == "IDENFR") {
    //TODO: should find
    (*iter)++;
    bool isFunc = (*iter)->token == "LPARENT";
    (*iter)--;
    if (isFunc) {
      SyntaxNode* funcRoot = new SyntaxNode("<有返回值函数调用语句>", "", "");
      root->appendChild(funcRoot);
      useFuncParse(iter, funcRoot);
    } else {
      appendLeaf(iter, root);
      if ((*iter)->token == "LBRACK") {
        appendLeaf(iter, root); // [
        SyntaxNode* expRoot = new SyntaxNode("<表达式>", "", "");
        root->appendChild(expRoot);
        expParse(iter, expRoot);
        if ((*iter)->token != "RBRACK") {
          exit(-1);
        }
        appendLeaf(iter, root); // ]
      }
    }
  } else if ((*iter)->token == "LPARENT") {
    appendLeaf(iter, root); // (
    SyntaxNode* expRoot = new SyntaxNode("<表达式>", "", "");
    root->appendChild(expRoot);
    expParse(iter, expRoot);
    if ((*iter)->token != "RPARENT") {
      exit(-1);
    }
    appendLeaf(iter, root); // )
  } else if ((*iter)->token == "CHARCON") {
    appendLeaf(iter, root);
  } else {
    // INTCON TODO: error
    // if ((*iter)->token != "INTCON" && (*iter)->value != "+" && (*iter)->value != "-") {
    //   exit(-1);
    // }
    SyntaxNode* tempRoot = new SyntaxNode("<整数>", "", "");
    root->appendChild(tempRoot);
    numParse(iter, tempRoot);
  }
}

void circleParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root){ // 循环
  if ((*iter)->token == "WHILETK") {
    appendLeaf(iter, root); // while
    if ((*iter)->token != "LPARENT") {
      exit(-1);
    }
    appendLeaf(iter, root); // (
    SyntaxNode* tempRoot = new SyntaxNode("<条件>", "", "");
    root->appendChild(tempRoot);
    conditionParse(iter, tempRoot);
    if ((*iter)->token != "RPARENT") {
      exit(-1);
    }
    appendLeaf(iter, root); // )
    tempRoot = new SyntaxNode("<语句>", "", "");
    root->appendChild(tempRoot);
    sentenceParse(iter, tempRoot);
  } else if ((*iter)->token == "DOTK") {
    appendLeaf(iter, root); // do
    SyntaxNode* tempRoot = new SyntaxNode("<语句>", "", "");
    root->appendChild(tempRoot);
    sentenceParse(iter, tempRoot);
    if ((*iter)->token != "WHILETK") {
      exit(-1);
    }
    appendLeaf(iter, root); // while
    if ((*iter)->token != "LPARENT") {
      exit(-1);
    }
    appendLeaf(iter, root); // (
    tempRoot = new SyntaxNode("<条件>", "", "");
    root->appendChild(tempRoot);
    conditionParse(iter, tempRoot);
    if ((*iter)->token != "RPARENT") {
      exit(-1);
    }
    appendLeaf(iter, root); // )
  } else if ((*iter)->token == "FORTK") {
    appendLeaf(iter, root); // for
    if ((*iter)->token != "LPARENT") {
      exit(-1);
    }
    appendLeaf(iter, root); // (
    if ((*iter)->token != "IDENFR") {
      exit(-1);
    }
    appendLeaf(iter, root); // IDENFR
    if ((*iter)->token != "ASSIGN") {
      exit(-1);
    }
    appendLeaf(iter, root); // =
    SyntaxNode* tempRoot = new SyntaxNode("<表达式>", "", "");
    root->appendChild(tempRoot);
    expParse(iter, tempRoot);
    if ((*iter)->token != "SEMICN") {
      exit(-1);
    }
    appendLeaf(iter, root); // ;
    tempRoot = new SyntaxNode("<条件>", "", "");
    root->appendChild(tempRoot);
    conditionParse(iter, tempRoot);
    if ((*iter)->token != "SEMICN") {
      exit(-1);
    }
    appendLeaf(iter, root); // ;
    if ((*iter)->token != "IDENFR") {
      exit(-1);
    }
    appendLeaf(iter, root); // IDENFR
    if ((*iter)->token != "ASSIGN") {
      exit(-1);
    }
    appendLeaf(iter, root); // =
    if ((*iter)->token != "IDENFR") {
      exit(-1);
    }
    appendLeaf(iter, root); // IDENFR
    if ((*iter)->token != "PLUS" && (*iter)->token != "MINU") {
      exit(-1);
    }
    appendLeaf(iter, root); // +/-
    tempRoot = new SyntaxNode("<步长>", "", "");
    root->appendChild(tempRoot);
    stepParse(iter, tempRoot);
    if ((*iter)->token != "RPARENT") {
      exit(-1);
    }
    appendLeaf(iter, root); // )
    tempRoot = new SyntaxNode("<语句>", "", "");
    root->appendChild(tempRoot);
    sentenceParse(iter, tempRoot);
  }
}

void stepParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 步长
  SyntaxNode* unsignedNum = new SyntaxNode("<无符号整数>", "", "");
  root->appendChild(unsignedNum);
  if ((*iter)->token != "INTCON") {
      exit(-1);
    }
  appendLeaf(iter, unsignedNum); // INTCON
}

void useFuncParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 函数调用
  appendLeaf(iter, root); // IDENFR
  if ((*iter)->token != "LPARENT") {
      exit(-1);
    }
  appendLeaf(iter, root); // (
  SyntaxNode* tempRoot = new SyntaxNode("<值参数表>", "", "");
  root->appendChild(tempRoot);
  valuePrameterParse(iter, tempRoot);
  if ((*iter)->token != "RPARENT") {
      exit(-1);
    }
  appendLeaf(iter, root); // )
}

void valuePrameterParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 值参数表
  if ((*iter)->token == "RPARENT") {
    // may be empty
    return;
  }
  while (true) {
    SyntaxNode* tempRoot = new SyntaxNode("<表达式>", "", "");
    root->appendChild(tempRoot);
    expParse(iter, tempRoot);
    if ((*iter)->token == "COMMA") {
      appendLeaf(iter, root);
    } else {
      break;
    }
  }
}

void assignParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 赋值语句
  appendLeaf(iter, root); // IDENFR
  if ((*iter)->token == "LBRACK") {
    appendLeaf(iter, root); // [
    SyntaxNode* tempRoot = new SyntaxNode("<表达式>", "", "");
    root->appendChild(tempRoot);
    expParse(iter, tempRoot);
    if ((*iter)->token != "RBRACK") {
      exit(-1);
    }
    appendLeaf(iter, root); // ]
  }
  if ((*iter)->token != "ASSIGN") {
    exit(-1);
  }
  appendLeaf(iter, root); // =
  SyntaxNode* expRoot = new SyntaxNode("<表达式>", "", "");
  root->appendChild(expRoot);
  expParse(iter, expRoot);
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
    appendLeaf(iter, root); // IDENFR
    if ((*iter)->token != "COMMA") {
      break;
    } else {
      appendLeaf(iter, root); // ,
    }
  }
  if ((*iter)->token != "RPARENT") {
    exit(-1);
  }
  appendLeaf(iter, root); // )
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
    appendLeaf(iter, tempRoot); // STRCON
    if ((*iter)->token == "COMMA") {
      appendLeaf(iter, root);
      tempRoot = new SyntaxNode("<表达式>", "", "");
      root->appendChild(tempRoot);
      expParse(iter, tempRoot);
    }
  } else {
    SyntaxNode* tempRoot = new SyntaxNode("<表达式>", "", "");
    root->appendChild(tempRoot);
    expParse(iter, tempRoot);
  }
  if ((*iter)->token != "RPARENT") {
    exit(-1);
  }
  appendLeaf(iter, root); // )
}

void returnParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 返回语句
  appendLeaf(iter, root); // return
  if ((*iter)->token == "LPARENT") {
    appendLeaf(iter, root); // (
    SyntaxNode* tempRoot = new SyntaxNode("<表达式>", "", "");
    root->appendChild(tempRoot);
    expParse(iter, tempRoot);
    if ((*iter)->token != "RPARENT") {
      exit(-1);
    }
    appendLeaf(iter, root); // )
  }
}

void mainParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 主函数
  appendLeaf(iter, root); // void
  if ((*iter)->token != "MAINTK") {
    exit(-1);
  }
  appendLeaf(iter, root); // main
  if ((*iter)->token != "LPARENT") {
    exit(-1);
  }
  appendLeaf(iter, root); // (
  if ((*iter)->token != "RPARENT") {
    exit(-1);
  }
  appendLeaf(iter, root); // )
  if ((*iter)->token != "LBRACE") {
    exit(-1);
  }
  appendLeaf(iter, root); // {
  SyntaxNode* tempRoot = new SyntaxNode("<复合语句>", "", "");
  root->appendChild(tempRoot);
  complexSentenceParse(iter, tempRoot);
  if ((*iter)->token != "RBRACE") {
    exit(-1);
  }
  appendLeaf(iter, root); // }
}

void voidFuncParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 无返回值函数定义
  appendLeaf(iter, root); // void
  funcMap.insert({(*iter)->value, VOID_FUNC});
  if ((*iter)->token != "IDENFR") {
    exit(-1);
  }
  appendLeaf(iter, root); // IDENFR
  if ((*iter)->token != "LPARENT") {
    exit(-1);
  }
  appendLeaf(iter, root); // (
  SyntaxNode* tempRoot = new SyntaxNode("<参数表>", "", "");
  root->appendChild(tempRoot);
  parameterParse(iter, tempRoot);
  if ((*iter)->token != "RPARENT") {
    exit(-1);
  }
  appendLeaf(iter, root); // )
  if ((*iter)->token != "LBRACE") {
    exit(-1);
  }
  appendLeaf(iter, root); // {
  tempRoot = new SyntaxNode("<复合语句>", "", "");
  root->appendChild(tempRoot);
  complexSentenceParse(iter, tempRoot);
  if ((*iter)->token != "RBRACE") {
    exit(-1);
  }
  appendLeaf(iter, root); // }
}

void printTree(SyntaxNode* root) {
  // post order
  if (root->isLeaf()) {
    if (!root->getLexicalToken().empty()) {
      fprintf(out, "%s %s\n", root->getLexicalToken().data(), root->getLexicalValue().data());
    } else {
      fprintf(out, "%s\n", root->getSyntaxValue().data());
    }
  } else {
    list<SyntaxNode*> children = root->getChildren();
    for (list<SyntaxNode*>::iterator iter = children.begin(); iter != children.end(); iter++) {
      SyntaxNode* son = *iter;
      printTree(son);
    }
    if (!root->getLexicalToken().empty()) {
      fprintf(out, "%s %s\n", root->getLexicalToken().data(), root->getLexicalValue().data());
    } else {
      fprintf(out, "%s\n", root->getSyntaxValue().data());
    }
  }
}