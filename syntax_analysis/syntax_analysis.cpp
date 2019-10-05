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
  while ((*iter)->value == "const") {
    appendLeaf(iter, root); // const
    temp = new SyntaxNode("<常量定义>", "", "");
    root->appendChild(temp);
    constDefineParse(iter, temp);
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
    appendLeaf(iter, root); // =
    if ((*iter)->token == "CHARCON") {
      appendLeaf(iter, root); // CHARCON
    } else {
      SyntaxNode* tempRoot = new SyntaxNode("<整数>", "", "");
      root->appendChild(tempRoot);
      numParse(iter, tempRoot);
    }
    if ((*iter)->value == ",") {
      appendLeaf(iter, root); // ,
    }
  }
}

void numParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 整数
  if ((*iter)->value == "+" || (*iter)->value == "-") {
    appendLeaf(iter, root); // + or -
  }
  SyntaxNode* tempRoot = new SyntaxNode("<无符号整数>", "", "");
  root->appendChild(tempRoot);
  appendLeaf(iter, tempRoot); // INTCON
}

bool isVarDefine(list<struct Lexeme>::iterator* iter) {
  if ((*iter)->value != "int" && (*iter)->value != "char" ) {
    return false;
  }
  (*iter)++; // IDENFR
  (*iter)++; // (
  if ((*iter)->value == "(") {
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
    appendLeaf(iter, root); // ;
  }
}

void varDefineParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 变量定义
  appendLeaf(iter, root); // append int or char
  while ((*iter)->token == "IDENFR") {
    varConstMap.insert({(*iter)->value, VAR});
    appendLeaf(iter, root); // IDENFR
    if ((*iter)->value == "[") {
      appendLeaf(iter, root); // [
      SyntaxNode* unsignNumRoot = new SyntaxNode("<无符号整数>", "", "");
      root->appendChild(unsignNumRoot);
      appendLeaf(iter, unsignNumRoot); // INTCON
      appendLeaf(iter, root); // ]
    }
    if ((*iter)->value == ",") {
      appendLeaf(iter, root);
    }
  }
}

void funcParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 有返回值函数定义
  SyntaxNode* tempRoot = new SyntaxNode("<声明头部>", "", "");
  root->appendChild(tempRoot);
  declareParse(iter, tempRoot);
  appendLeaf(iter, root); // (
  tempRoot = new SyntaxNode("<参数表>", "", "");
  root->appendChild(tempRoot);
  parameterParse(iter, tempRoot);
  appendLeaf(iter, root); // )
  appendLeaf(iter, root); // {
  tempRoot = new SyntaxNode("<复合语句>", "", "");
  root->appendChild(tempRoot);
  complexSentenceParse(iter, tempRoot);
  appendLeaf(iter, root); // }
}

void declareParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 声明头部
  appendLeaf(iter, root); // int
  funcMap.insert({(*iter)->value, FUNC});
  appendLeaf(iter, root); // IDENFR
}

void parameterParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 参数表
  // may be empty
  if ((*iter)->value == ")") {
    return;
  }
  while ((*iter)->value == "int" || (*iter)->value == "char") {
    appendLeaf(iter, root); // int or char
    varConstMap.insert({(*iter)->value, VAR});
    appendLeaf(iter, root); // IDENTFR
    if ((*iter)->value == ",") {
      appendLeaf(iter, root); // ,
    }
  }
}

void complexSentenceParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 复合语句
  if ((*iter)->value == "const") {
    SyntaxNode* constRoot = new SyntaxNode("<常量说明>", "", "");
    root->appendChild(constRoot);
    constParse(iter, constRoot);
  }
  if ((*iter)->value == "int" || (*iter)->value == "char") {
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
  while ((*iter)->value != "}") {
    SyntaxNode* sentenceRoot = new SyntaxNode("<语句>", "", "");
    root->appendChild(sentenceRoot);
    sentenceParse(iter, sentenceRoot);
  }
}

void sentenceParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 语句
  if ((*iter)->value == "if") {
    SyntaxNode* ifRoot = new SyntaxNode("<条件语句>", "", "");
    root->appendChild(ifRoot);
    ifParse(iter, ifRoot);
  } else if ((*iter)->value == "while" || (*iter)->value == "do" || (*iter)->value == "for") {
    SyntaxNode* circleRoot = new SyntaxNode("<循环语句>", "", "");
    root->appendChild(circleRoot);
    circleParse(iter, circleRoot);
  } else if ((*iter)->value == "{") {
    appendLeaf(iter, root); // {
    SyntaxNode* tempRoot = new SyntaxNode("<语句列>", "", "");
    root->appendChild(tempRoot);
    sentenceListParse(iter, tempRoot);
    appendLeaf(iter, root); // }
  } else if ((*iter)->token == "IDENFR") {
    (*iter)++;
    bool isFunc = (*iter)->value == "(";
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
        appendLeaf(iter, root); // ;
      } else {
        SyntaxNode* tempRoot = new SyntaxNode("<无返回值函数调用语句>", "", "");
        root->appendChild(tempRoot);
        useFuncParse(iter, tempRoot);
        appendLeaf(iter, root); // ;
      }
    }
  } else if ((*iter)->value == "scanf") {
    SyntaxNode* tempRoot = new SyntaxNode("<读语句>", "", "");
    root->appendChild(tempRoot);
    scanfParse(iter, tempRoot);
    appendLeaf(iter, root); // ;
  } else if ((*iter)->value == "printf") {
    SyntaxNode* tempRoot = new SyntaxNode("<写语句>", "", "");
    root->appendChild(tempRoot);
    printfParse(iter, tempRoot);
    appendLeaf(iter, root); // ;
  } else if ((*iter)->value == ";") {
    appendLeaf(iter, root); // ;
  } else {
    SyntaxNode* tempRoot = new SyntaxNode("<返回语句>", "", "");
    root->appendChild(tempRoot);
    returnParse(iter, tempRoot);
    appendLeaf(iter, root); // ;
  }
}

void ifParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 条件语句
  appendLeaf(iter, root); // if
  appendLeaf(iter, root); // (
  SyntaxNode* tempRoot = new SyntaxNode("<条件>", "", "");
  root->appendChild(tempRoot);
  conditionParse(iter, tempRoot);
  appendLeaf(iter, root); // )
  tempRoot = new SyntaxNode("<语句>", "", "");
  root->appendChild(tempRoot);
  sentenceParse(iter, tempRoot);
  if ((*iter)->value == "else") {
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
  if ((*iter)->value == "==" || (*iter)->value == "<" || (*iter)->value == "<=" || (*iter)->value == ">" || (*iter)->value == ">=" || (*iter)->value == "!=") {
    appendLeaf(iter, root);
    expressionRoot = new SyntaxNode("<表达式>", "", "");
    root->appendChild(expressionRoot);
    expParse(iter, expressionRoot);
  }
}

void expParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 表达式
  if ((*iter)->value == "+" || (*iter)->value == "-") {
    appendLeaf(iter, root); // + or -
  }
  while (true) {
    SyntaxNode* polyRoot = new SyntaxNode("<项>", "", "");
    root->appendChild(polyRoot);
    polyParse(iter, polyRoot);
    if ((*iter)->value == "+" || (*iter)->value == "-") {
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
    if ((*iter)->value == "*" || (*iter)->value == "/") {
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
    bool isFunc = (*iter)->value == "(";
    (*iter)--;
    if (isFunc) {
      SyntaxNode* funcRoot = new SyntaxNode("<有返回值函数调用语句>", "", "");
      root->appendChild(funcRoot);
      useFuncParse(iter, funcRoot);
    } else {
      appendLeaf(iter, root);
      if ((*iter)->value == "[") {
        appendLeaf(iter, root);
        SyntaxNode* expRoot = new SyntaxNode("<表达式>", "", "");
        root->appendChild(expRoot);
        expParse(iter, expRoot);
        appendLeaf(iter, root); // ]
      }
    }
  } else if ((*iter)->value == "(") {
    appendLeaf(iter, root); // (
    SyntaxNode* expRoot = new SyntaxNode("<表达式>", "", "");
    root->appendChild(expRoot);
    expParse(iter, expRoot);
    appendLeaf(iter, root); // )
  } else if ((*iter)->token == "CHARCON") {
    appendLeaf(iter, root);
  } else {
    // INTCON
    SyntaxNode* tempRoot = new SyntaxNode("<整数>", "", "");
    root->appendChild(tempRoot);
    numParse(iter, tempRoot);
  }
}

void circleParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root){ // 循环
  if ((*iter)->value == "while") {
    appendLeaf(iter, root); // while
    appendLeaf(iter, root); // (
    SyntaxNode* tempRoot = new SyntaxNode("<条件>", "", "");
    root->appendChild(tempRoot);
    conditionParse(iter, tempRoot);
    appendLeaf(iter, root); // )
    tempRoot = new SyntaxNode("<语句>", "", "");
    root->appendChild(tempRoot);
    sentenceParse(iter, tempRoot);
  } else if ((*iter)->value == "do") {
    appendLeaf(iter, root); // do
    SyntaxNode* tempRoot = new SyntaxNode("<语句>", "", "");
    root->appendChild(tempRoot);
    sentenceParse(iter, tempRoot);
    appendLeaf(iter, root); // while
    appendLeaf(iter, root); // (
    tempRoot = new SyntaxNode("<条件>", "", "");
    root->appendChild(tempRoot);
    conditionParse(iter, tempRoot);
    appendLeaf(iter, root); // )
  } else if ((*iter)->value == "for") {
    appendLeaf(iter, root); // for
    appendLeaf(iter, root); // (
    appendLeaf(iter, root); // IDENFR
    appendLeaf(iter, root); // =
    SyntaxNode* tempRoot = new SyntaxNode("<表达式>", "", "");
    root->appendChild(tempRoot);
    expParse(iter, tempRoot);
    appendLeaf(iter, root); // ;
    tempRoot = new SyntaxNode("<条件>", "", "");
    root->appendChild(tempRoot);
    conditionParse(iter, tempRoot);
    appendLeaf(iter, root); // ;
    appendLeaf(iter, root); // IDENFR
    appendLeaf(iter, root); // =
    appendLeaf(iter, root); // IDENFR
    appendLeaf(iter, root); // +/-
    tempRoot = new SyntaxNode("<步长>", "", "");
    root->appendChild(tempRoot);
    stepParse(iter, tempRoot);
    appendLeaf(iter, root); // )
    tempRoot = new SyntaxNode("<语句>", "", "");
    root->appendChild(tempRoot);
    sentenceParse(iter, tempRoot);
  }
}

void stepParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 步长
  SyntaxNode* unsignedNum = new SyntaxNode("<无符号整数>", "", "");
  root->appendChild(unsignedNum);
  appendLeaf(iter, unsignedNum); // INTCON
}

void useFuncParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 函数调用
  appendLeaf(iter, root); // IDENFR
  appendLeaf(iter, root); // (
  SyntaxNode* tempRoot = new SyntaxNode("<值参数表>", "", "");
  root->appendChild(tempRoot);
  valuePrameterParse(iter, tempRoot);
  appendLeaf(iter, root); // )
}

void valuePrameterParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 值参数表
  if ((*iter)->value == ")") {
    // may be empty
    return;
  }
  while (true) {
    SyntaxNode* tempRoot = new SyntaxNode("<表达式>", "", "");
    root->appendChild(tempRoot);
    expParse(iter, tempRoot);
    if ((*iter)->value == ",") {
      appendLeaf(iter, root);
    } else {
      break;
    }
  }
}

void assignParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 赋值语句
  appendLeaf(iter, root); // IDENFR
  if ((*iter)->value == "[") {
    appendLeaf(iter, root); // [
    SyntaxNode* tempRoot = new SyntaxNode("<表达式>", "", "");
    root->appendChild(tempRoot);
    expParse(iter, tempRoot);
    appendLeaf(iter, root); // ]
  }
  appendLeaf(iter, root); // =
  SyntaxNode* expRoot = new SyntaxNode("<表达式>", "", "");
  root->appendChild(expRoot);
  expParse(iter, expRoot);
}

void scanfParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 读语句
  while ((*iter)->value != ")") {
    appendLeaf(iter, root);
  }
  appendLeaf(iter, root); // )
}

void printfParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 写语句
  appendLeaf(iter, root); // printf
  appendLeaf(iter, root); // (
  if ((*iter)->token == "STRCON") {
    SyntaxNode* tempRoot = new SyntaxNode("<字符串>", "", "");
    root->appendChild(tempRoot);
    appendLeaf(iter, tempRoot); // STRCON
    if ((*iter)->value == ",") {
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
  appendLeaf(iter, root); // )
}

void returnParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 返回语句
  appendLeaf(iter, root); // return
  if ((*iter)->value == "(") {
    appendLeaf(iter, root); // (
    SyntaxNode* tempRoot = new SyntaxNode("<表达式>", "", "");
    root->appendChild(tempRoot);
    expParse(iter, tempRoot);
    appendLeaf(iter, root); // )
  }
}

void mainParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 主函数
  appendLeaf(iter, root); // void
  appendLeaf(iter, root); // main
  appendLeaf(iter, root); // (
  appendLeaf(iter, root); // )
  appendLeaf(iter, root); // {
  SyntaxNode* tempRoot = new SyntaxNode("<复合语句>", "", "");
  root->appendChild(tempRoot);
  complexSentenceParse(iter, tempRoot);
  appendLeaf(iter, root); // }
}

void voidFuncParse(list<struct Lexeme>::iterator* iter, SyntaxNode* root) { // 无返回值函数定义
  appendLeaf(iter, root); // void
  funcMap.insert({(*iter)->value, VOID_FUNC});
  appendLeaf(iter, root); // IDENFR
  appendLeaf(iter, root); // (
  SyntaxNode* tempRoot = new SyntaxNode("<参数表>", "", "");
  root->appendChild(tempRoot);
  parameterParse(iter, tempRoot);
  appendLeaf(iter, root); // )
  appendLeaf(iter, root); // {
  tempRoot = new SyntaxNode("<复合语句>", "", "");
  root->appendChild(tempRoot);
  complexSentenceParse(iter, tempRoot);
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