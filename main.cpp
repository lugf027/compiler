#include <iostream>
#include <vector>
#include "util/Util.h"
#include "util/MyConstants.h"
#include "syTestFiles/LexTest.h"
#include "parser/Parse.h"
#include "semantic/Semantic.h"


// Run->Edit Configurations->Program arguments
// -S -o <OutputFilepath> <InputFilepath> [-O1]
int main(int argc, char **argv) {
//    if (Util::handleParams(argc, argv) != PARAMS_PASS) {
//        std::cout << "Error Program arguments" << std::endl;
//        return -1;
//    }
//    Semantic semantic;
//    semantic.startSemantic();

    int a = 5;
    int b = -a;
    printf("%d", b);

    // 移步 semantic 构造方法
    //    Parse parse;
    //    parse.parseAST();

    // 移步 parse 构造方法
    //    Lex lex;
    //    test(lex);

    return 0;
}
