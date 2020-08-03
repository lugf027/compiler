#include "Arm7Gen.h"
#include "../util/Error.h"

Arm7Gen::Arm7Gen() {
    ast = nullptr;
    symbolTables = nullptr;
    armTree = nullptr;

    whilePos = new std::vector<std::string>();
    whileEndPos = new std::vector<std::string>();
    levelNow = 0;
    armRegManager = new ArmRegManager();
    /// TODO 我不知自什么始
    blockName = 0;
}

void Arm7Gen::startGen(AST *ast_, std::vector<SymbolTable *> *symbolTables_) {
    ast = ast_;
    symbolTables = symbolTables_;

    auto *armGlobals = new std::vector<ArmGlobal *>();

    /// 一次性声明所有全局变量
    for (Symbol *symbol:*symbolTables->at(1)->getSymbols()) {
        if (symbol->getArm7Var() != nullptr) {
            auto *arm7GlobalVar =
                    new Arm7GlobalVar(symbol->getArm7Var()->getIdent().c_str(), symbol->getArm7Var()->getIfConst(),
                                      symbol->getArm7Var()->getSubs(), symbol->getArm7Var()->getValue());
            auto *armGlobal = new ArmGlobal(arm7GlobalVar, nullptr);
            armGlobals->emplace_back(armGlobal);
        }
    }

    for (Decl *decl:*(ast->getDecls())) {
        if (decl->getFuncDef() != nullptr) {
            auto *armBlocks = new std::vector<ArmBlock *>();
            genArm7Func(decl->getFuncDef(), armBlocks);

            auto *arm7GlobalFunc =
                    new Arm7GlobalFunc(decl->getFuncDef()->getIdent().c_str(), armBlocks);
            auto *armGlobal = new ArmGlobal(nullptr, arm7GlobalFunc);
            armGlobals->emplace_back(armGlobal);
        }
    }
    armTree = new Arm7Tree(armGlobals);
}

///===-----------------------------------------------------------------------===///
/// 基本声明定义
///===-----------------------------------------------------------------------===///

//void Arm7Gen::genArm7Var(Decl *decl, std::vector<ArmGlobal *> *armGlobals) {
//    ArmGlobal *armGlobal = nullptr;
//
//
//}

void Arm7Gen::genVar(VarDef *varDef, std::vector<ArmStmt *> *armStmts) {
    if (varDef->getInitVal() != nullptr) {
        ///	str rRX, [fp, #-LOC]
        auto *rReg = genAddExp(varDef->getInitVal()->getExp()->getAddExp(), armStmts);
        auto *armSTRStmt = new ArmStmt(ARM_STMT_STR, rReg->getRegName().c_str(),
                                       ("[fp, #" + std::to_string(varDef->getBaseMemoryPos()) + "]").c_str());
        armStmts->emplace_back(armSTRStmt);
    }
}

void Arm7Gen::genConstVar(ConstDef *constDef, std::vector<ArmStmt *> *armStmts) {
    int value = GLOBAL_DEFAULT_VALUE;
    if (constDef->getConstInitVal() != nullptr) {
        value = calConstExp(constDef->getConstInitVal()->getConstExp());
    }

    ///	mov	rX, #CONST_VALUE
    ///	str	rX, [fp, #-LOC]
    ArmReg *armReg = armRegManager->getFreeArmReg(armStmts);
    auto *armStmtMove = new ArmStmt(ARM_STMT_MOVE, armReg->getRegName().c_str(),
                                    ("#" + std::to_string(value)).c_str());
    auto *armStmtStr = new ArmStmt(ARM_STMT_STR, armReg->getRegName().c_str(),
                                   ("[fp, #" + std::to_string(constDef->getBaseMemoryPos()) + "]").c_str());
    armStmts->emplace_back(armStmtMove);
    armStmts->emplace_back(armStmtStr);
}

void Arm7Gen::genVarArray(VarDef *varDef, std::vector<ArmStmt *> *armStmts) {
    // sub 相关
    auto *subs = new std::vector<int>();
    int len = 1;
    for (ConstExp *constExp:*varDef->getConstExps()) {
        int sub = calConstExp(constExp);
        len *= sub;
        subs->push_back(sub);
    }

    // value 相关
    auto *values = new std::vector<Exp *>();
    if (varDef->getInitVal() != nullptr) {
        values = genVarArrayInitVals(varDef->getInitVal(), subs, armStmts);
    } else {
        for (int i = 0; i < len; i++) {
            // addExp null 表 DEFAULT_VALUE
            auto *exp = new Exp(nullptr);
            values->push_back(exp);
        }
    }

    // arm 相关
    auto *armAssignStmts = new std::vector<ArmStmt *>();
    for (int i = 0; i < len; i++) {
        int subPosNow = varDef->getBaseMemoryPos() - i * 4;
        if (values->at(i)->getAddExp() == nullptr) {
            ///	mov	rX, #CONST_VALUE
            ///	str	rX, [fp, #-LOC]
            ArmReg *armReg = armRegManager->getFreeArmReg(armStmts);
            auto *armStmtMove = new ArmStmt(ARM_STMT_MOVE, armReg->getRegName().c_str(),
                                            ("#" + std::to_string(GLOBAL_DEFAULT_VALUE)).c_str());
            auto *armStmtStr = new ArmStmt(ARM_STMT_STR, armReg->getRegName().c_str(),
                                           ("[fp, #" + std::to_string(subPosNow) + "]").c_str());
            armAssignStmts->emplace_back(armStmtMove);
            armAssignStmts->emplace_back(armStmtStr);
        } else {
            ///	str rRX, [fp, #-LOC]
            auto *rReg = genAddExp(varDef->getInitVal()->getExp()->getAddExp(), armStmts);
            auto *armSTRStmt = new ArmStmt(ARM_STMT_STR, rReg->getRegName().c_str(),
                                           ("[fp, #" + std::to_string(subPosNow) + "]").c_str());
            armStmts->emplace_back(armSTRStmt);
        }
    }
}

void Arm7Gen::genConstVarArray(ConstDef *constDef, std::vector<ArmStmt *> *armStmts) {
    // sub 相关
    auto *subs = new std::vector<int>();
    int len = 1;
    for (ConstExp *constExp:*constDef->getConstExps()) {
        int sub = calConstExp(constExp);
        len *= sub;
        subs->push_back(sub);
    }

    // value 相关
    auto *values = new std::vector<int>();
    if (constDef->getConstInitVal() != nullptr) {
        values = calConstArrayInitVals(constDef->getConstInitVal(), subs);
    } else {
        for (int i = 0; i < len; i++) {
            values->push_back(GLOBAL_DEFAULT_VALUE);
        }
    }

    // arm
    ArmReg *armReg = armRegManager->getFreeArmReg(armStmts);
    for (int i = 0; i < len; i++) {
        int subPosNow = constDef->getBaseMemoryPos() - i * 4;
        ///	mov	rX, #SUB_VALUE
        ///	str	rX, [fp, #-LOC]
        auto *armStmtMove = new ArmStmt(ARM_STMT_MOVE, armReg->getRegName().c_str(),
                                        ("#" + std::to_string(values->at(i))).c_str());
        auto *armStmtStr = new ArmStmt(ARM_STMT_STR, armReg->getRegName().c_str(),
                                       ("[fp, #" + std::to_string(subPosNow) + "]").c_str());
        armStmts->emplace_back(armStmtMove);
        armStmts->emplace_back(armStmtStr);
    }
}

void Arm7Gen::genArm7Var(BlockItem *blockItem, std::vector<ArmStmt *> *armStmts) {
    if (blockItem->getConstDecl() != nullptr) {
        for (ConstDef *constDef:*blockItem->getConstDecl()->getConstDefs()) {
            if (constDef->getConstExps()->empty()) {
                genConstVar(constDef, armStmts);
            } else {
                genConstVarArray(constDef, armStmts);
            }
        }
    } else if (blockItem->getVarDecl() != nullptr) {
        for (VarDef *varDef: *blockItem->getVarDecl()->getVarDefs()) {
            if (varDef->getConstExps()->empty()) {
                genVar(varDef, armStmts);
            } else {
                genVarArray(varDef, armStmts);
            }
        }
    }
}

void Arm7Gen::genArm7Func(FuncDef *funcDef, std::vector<ArmBlock *> *armBlocks) {
    funcNameNow = funcDef->getIdent();
    levelNow++;

    auto *armStmts = new std::vector<ArmStmt *>();
    auto *blockEntry = new ArmBlock(BLOCK_ENTRY.c_str(), armStmts);
    armBlocks->emplace_back(blockEntry);


    for (Symbol *symbol:*symbolTables->at(1)->getSymbols()) {
        if (symbol->getArm7Func() != nullptr && symbol->getArm7Func()->getIdent() == funcDef->getIdent()) {
            /// push	{fp, lr}
            /// add	fp, sp, #4
            /// sub	sp, sp, #DIGIT_CAPACITY
            auto *armStmtPush = new ArmStmt(ARM_STMT_PUSH, "{fp, lr}");
            auto *armStmtAdd = new ArmStmt(ARM_STMT_ADD, "sp", "#4");
            auto *armStmtSub = new ArmStmt(ARM_STMT_SUB, "sp", "sp",
                                           ("#" + std::to_string(symbol->getArm7Func()->getCapacity())).c_str());
            armStmts->emplace_back(armStmtPush);
            armStmts->emplace_back(armStmtAdd);
            armStmts->emplace_back(armStmtSub);

            /// 寄存器传递的参数
            auto *params = symbol->getArm7Func()->getParams();
            for (int i = 0; i < symbol->getArm7Func()->getParams()->size() && i < 4; i++) {
                auto *armStmt =
                        new ArmStmt(ARM_STMT_STR, ("r" + std::to_string(i)).c_str(),
                                    ("[fp, #" + std::to_string(params->at(i)->getMemoryLoc()) + "]").c_str());
                armStmts->emplace_back(armStmt);

                armRegManager->getArmRegs()->at(i)->setArm7Var(params->at(i));
            }
        }
    }

    genBlock(funcDef->getBlock(), armBlocks, blockEntry, armStmts);

    /// 考虑到如下固定格式结尾无需计算，保留在 output 时输出到文件
    /// sub	sp, fp, #4
    /// @ sp needed
    /// pop	{fp, pc}
    /// .size	whileFunc, .-whileFunc
    /// .align	2

    funcNameNow = nullptr;
    levelNow--;
}

///===-----------------------------------------------------------------------===///
/// 语句、语句块
///===-----------------------------------------------------------------------===///

const char *Arm7Gen::genBlock(Block *block, std::vector<ArmBlock *> *basicBlocks, ArmBlock *lastBlock,
                              std::vector<ArmStmt *> *lastBlockStmts) {
    levelNow++;

    ArmBlock *tmpBlock = lastBlock;
    std::vector<ArmStmt *> *tmpArmStmts = lastBlockStmts;

    for (BlockItem *blockItem: *block->getBlockItems()) {
        if (blockItem->getStmt() != nullptr) {
            const char *newBlockName = genStmt(blockItem->getStmt(), basicBlocks, tmpBlock, tmpArmStmts);
            if (strcmp(newBlockName, tmpBlock->getBlockName().c_str()) != 0) {  // 返回得知当前 basicBlock 有变化
                auto *newStmts = new std::vector<ArmStmt *>();
                auto *newBlock = new ArmBlock(newBlockName, newStmts);
                tmpBlock = newBlock;
                tmpArmStmts = newStmts;
            }
        } else if (blockItem->getConstDecl() != nullptr || blockItem->getVarDecl() != nullptr) {
            genArm7Var(blockItem, tmpArmStmts);
        }
    }


    levelNow--;
    return lastBlock->getBlockName().c_str();
}

const char *Arm7Gen::genStmt(Stmt *stmt, std::vector<ArmBlock *> *basicBlocks, ArmBlock *lastBlock,
                             std::vector<ArmStmt *> *lastBlockStmts) {
    return nullptr;
}

const char *Arm7Gen::genStmtAuxIf(Stmt *stmt, std::vector<ArmBlock *> *basicBlocks, ArmBlock *lastBlock,
                                  std::vector<ArmStmt *> *lastBlockStmts) {
    return nullptr;
}

const char *Arm7Gen::genStmtAuxWhile(Stmt *stmt, std::vector<ArmBlock *> *basicBlocks, ArmBlock *lastBlock,
                                     std::vector<ArmStmt *> *lastBlockStmts) {
    return nullptr;
}

///===-----------------------------------------------------------------------===///
/// 表达式 不计算 生成代码
///===-----------------------------------------------------------------------===///

std::vector<Exp *> *Arm7Gen::genVarArrayInitVals(InitVal *initVal, std::vector<int> *subs,
                                                 std::vector<ArmStmt *> *ArmStmts) {
    return nullptr;
}

const char *Arm7Gen::genCondExp(Cond *cond, std::vector<ArmStmt *> *ArmStmts) {
    return nullptr;
}

const char *Arm7Gen::genLOrExp(LOrExp *lOrExp, std::vector<ArmStmt *> *ArmStmts) {
    return nullptr;
}

const char *Arm7Gen::genLAndExp(LAndExp *lAndExp, std::vector<ArmStmt *> *ArmStmts) {
    return nullptr;
}

const char *Arm7Gen::genEqExp(EqExp *eqExp, std::vector<ArmStmt *> *ArmStmts) {
    return nullptr;
}

const char *Arm7Gen::genRelExp(RelExp *relExp, std::vector<ArmStmt *> *ArmStmts) {
    return nullptr;
}

ArmReg *Arm7Gen::genAddExp(AddExp *addExp, std::vector<ArmStmt *> *ArmStmts) {
    if (addExp->getOpType() == OP_NULL) {
        return genMulExp(addExp->getMulExp(), ArmStmts);
    } else {
        /// 加数乘法式中间结果，Arm7Var 成员变量不一定为 null
        ArmReg *mulRet = genMulExp(addExp->getMulExp(), ArmStmts);
        /// 中间结果压栈，不用管是否释放寄存器，null时可被直接用；非null时可被重复利用
        auto *pushStmt = new ArmStmt(ARM_STMT_PUSH, ("{" +mulRet->getRegName()+" }").c_str());
        ArmStmts->emplace_back(pushStmt);
        /// 加数加法式中间结果，Arm7Var 成员变量可为 null-->因此，可能会被误分配为 armRegRet
        ArmReg *addRet = genAddExp(addExp->getAddExp(), ArmStmts);
        /// 锁定加数加法式中间结果，Arm7Var,防止被误分配为 armRegRet
        addRet->setIfLock(ARM_REG_LOCK_TRUE);

        ArmReg *armRegRet = armRegManager->getFreeArmReg(ArmStmts);
        /// 解锁加数加法式中间结果
        addRet->setIfLock(ARM_REG_LOCK_FALSE);

        auto *popStmt = new ArmStmt(ARM_STMT_POP, ("{" +armRegRet->getRegName()+" }").c_str());
        /// 以 armRegRet 为最终结果，因为 addRet 可能为某变量，其 ArmReg 有对应某个变量地址
        auto *armAddStmt = new ArmStmt(addExp->getOpType(), armRegRet->getRegName().c_str(),
                                       armRegRet->getRegName().c_str(), addRet->getRegName().c_str());
        ArmStmts->emplace_back(popStmt);
        ArmStmts->emplace_back(armAddStmt);

        return armRegRet;
    }
}

ArmReg *Arm7Gen::genMulExp(MulExp *mulExp, std::vector<ArmStmt *> *ArmStmts) {
    if (mulExp->getOpType() == OP_NULL) {
        return genUnaryExp(mulExp->getUnaryExp(), ArmStmts);
    } else if(mulExp->getOpType() == OP_BO_MUL){
        /// 乘数一元表达式中间结果，Arm7Var 成员变量不一定为 null
        ArmReg *unaryRet = genUnaryExp(mulExp->getUnaryExp(), ArmStmts);
        /// 中间结果压栈，不用管是否释放寄存器，null时可被直接用；非null时可被重复利用
        auto *pushStmt = new ArmStmt(ARM_STMT_PUSH, ("{" +unaryRet->getRegName()+" }").c_str());
        ArmStmts->emplace_back(pushStmt);
        /// 乘数乘法式中间结果，Arm7Var 成员变量可为 null-->因此，可能会被误分配为 armRegRet
        ArmReg *mulRet = genMulExp(mulExp->getMulExp(), ArmStmts);
        /// 锁定乘数乘法式中间结果，Arm7Var,防止被误分配为 armRegRet
        mulRet->setIfLock(ARM_REG_LOCK_TRUE);

        ArmReg *armRegRet = armRegManager->getFreeArmReg(ArmStmts);
        /// 解锁加数加法式中间结果
        mulRet->setIfLock(ARM_REG_LOCK_FALSE);

        auto *popStmt = new ArmStmt(ARM_STMT_POP, ("{" +armRegRet->getRegName()+" }").c_str());
        /// 以 armRegRet 为最终结果，因为 mulRet 可能为某变量，其 ArmReg 有对应某个变量地址
        auto *armAddStmt = new ArmStmt(mulExp->getOpType(), armRegRet->getRegName().c_str(),
                                       armRegRet->getRegName().c_str(), mulRet->getRegName().c_str());
        ArmStmts->emplace_back(popStmt);
        ArmStmts->emplace_back(armAddStmt);

        return armRegRet;
    }
}

ArmReg *Arm7Gen::genUnaryExp(UnaryExp *unaryExp, std::vector<ArmStmt *> *ArmStmts) {
    if (unaryExp->getPrimaryExp() != nullptr) {
        return genPrimaryExp(unaryExp->getPrimaryExp(), ArmStmts);
    } else if (unaryExp->getOpType() != OP_NULL) {
        if (unaryExp->getOpType() == OP_BO_ADD) {
            return genUnaryExp(unaryExp->getUnaryExp(), ArmStmts);
        } else if (unaryExp->getOpType() == OP_BO_SUB) {
            ArmReg *unaryArmReg = genUnaryExp(unaryExp->getUnaryExp(), ArmStmts);


        } else if (unaryExp->getOpType() == OP_UO_NOT) {

        }
    } else {

    }
    return nullptr;
}

ArmReg *Arm7Gen::genPrimaryExp(PrimaryExp *primaryExp, std::vector<ArmStmt *> *ArmStmts) {
    return nullptr;
}

ArmReg *Arm7Gen::genLVal(LVal *lVal, std::vector<ArmStmt *> *ArmStmts) {
    return nullptr;
}

///===-----------------------------------------------------------------------===///
/// Expr 表达式 CalConst
/// @param one kind of Exp or initVal
/// @return const value of that Exp or initVal
///===-----------------------------------------------------------------------===///

std::vector<int> *Arm7Gen::calConstArrayInitVals(ConstInitVal *constInitVal, std::vector<int> *subs) {
    auto *valuesRet = new std::vector<int>();
    int len = 1;  // 维度展开一维的长度
    for (int i : *subs) {
        len *= i;
    }
    if (constInitVal->getConstInitVals()->empty()) {  // {}
        for (int i = 0; i < len; i++) {
            valuesRet->push_back(0);
        }
    } else {
        for (ConstInitVal *initValInner: *constInitVal->getConstInitVals()) {
            if (initValInner->getConstExp() != nullptr) {  // 实值
                int valueInner = calConstExp(initValInner->getConstExp());
                valuesRet->push_back(valueInner);
            } else {  // 数组嵌套
                int sub_first_len = 1;  // 子维展开一维的长度
                for (int i = 1; i < subs->size(); i++) {
                    sub_first_len *= subs->at(i);
                }
                if (valuesRet->size() % sub_first_len == 0) {  // 保证嵌套子维前，已有长度为子维整数倍
                    int tmp = subs->at(0);  // 当前第一维度值
                    subs->erase(subs->begin());
                    std::vector<int> *subs_values_inner = calConstArrayInitVals(initValInner, subs);
                    subs->insert(subs->begin(), tmp);
                    valuesRet->insert(valuesRet->end(), subs_values_inner->begin(), subs_values_inner->end());  // 拼接
                } else {
                    Error::errorSim("calConstArrayInitVals");
                    exit(-1);
                }
            }
        }
    }
    if (valuesRet->size() < len) {  // 长度不足补零
        for (int i = 0; i < len - valuesRet->size(); i++) {
            valuesRet->push_back(0);
        }
    } else if (valuesRet->size() > len) {
        Error::errorSim("ConstArray len error");
    }
    return valuesRet;
}

std::vector<int> *Arm7Gen::calVarArrayInitVals(InitVal *initVal, std::vector<int> *subs) {
    auto *valuesRet = new std::vector<int>();
    int len = 1;  // 维度展开一维的长度
    for (int i : *subs) {
        len *= i;
    }
    if (initVal->getInitVals()->empty()) {
        for (int i = 0; i < len; i++) {
            valuesRet->push_back(0);
        }
    } else {
        for (InitVal *initValInner: *initVal->getInitVals()) {
            if (initValInner->getExp() != nullptr) {  // 实值
                int valueInner = calAddExp(initValInner->getExp()->getAddExp());
                valuesRet->push_back(valueInner);
            } else {  // 数组嵌套
                int sub_first_len = 1;  // 子维展开一维的长度
                for (int i = 1; i < subs->size(); i++) {
                    sub_first_len *= subs->at(i);
                }
                if (valuesRet->size() % sub_first_len == 0) {  // 保证嵌套子维前，已有长度为子维整数倍
                    int tmp = subs->at(0);  // 当前第一维度值
                    subs->erase(subs->begin());
                    std::vector<int> *subs_values_inner = calVarArrayInitVals(initValInner, subs);
                    subs->insert(subs->begin(), tmp);
                    valuesRet->insert(valuesRet->end(), subs_values_inner->begin(), subs_values_inner->end());  // 拼接
                }
            }
        }
    }
    if (valuesRet->size() < len) {  // 长度不足补零
        for (int i = 0; i < len - valuesRet->size(); i++) {
            valuesRet->push_back(0);
        }
    }
    return valuesRet;
}

int Arm7Gen::calConstExp(ConstExp *constExp) {
    return calAddExp(constExp->getAddExp());
}

int Arm7Gen::calAddExp(AddExp *addExp) {
    int intRet;
    if (addExp->getOpType() == OP_NULL) {
        intRet = calMulExp(addExp->getMulExp());
    } else {
        int mulRet = calMulExp(addExp->getMulExp());
        int addRet = calAddExp(addExp->getAddExp());
        switch (addExp->getOpType()) {
            case OP_BO_ADD:
                intRet = mulRet + addRet;
                break;
            case OP_BO_SUB:
                intRet = mulRet - addRet;
                break;
            default:
                Error::errorSim("semantic.cpp -> calAddExp");
                exit(-1);
        }
    }
    return intRet;
}

int Arm7Gen::calMulExp(MulExp *mulExp) {
    int intRet;
    if (mulExp->getOpType() == OP_NULL) {
        intRet = calUnaryExp(mulExp->getUnaryExp());
    } else {
        int unaryRet = calUnaryExp(mulExp->getUnaryExp());
        int mulRet = calMulExp(mulExp->getMulExp());
        switch (mulExp->getOpType()) {
            case OP_BO_MUL:
                intRet = unaryRet * mulRet;
                break;
            case OP_BO_DIV:
                intRet = unaryRet / mulRet;
                break;
            case OP_BO_REM:
                intRet = unaryRet % mulRet;
                break;
            default:
                Error::errorSim("semantic.cpp -> calMulExp");
                exit(-1);
        }
    }
    return intRet;
}

int Arm7Gen::calUnaryExp(UnaryExp *unaryExp) {
    int intRet;
    if (unaryExp->getOpType() != OP_NULL) {
        switch (unaryExp->getOpType()) {
            case OP_BO_ADD:
                intRet = calUnaryExp(unaryExp->getUnaryExp());
                break;
            case OP_BO_SUB:
                intRet = -calUnaryExp(unaryExp->getUnaryExp());
                break;
            case OP_UO_NOT:
                Error::errorSim("\"!\" can't be in here");
                exit(-1);
            default:
                Error::errorSim("Semantic.cpp calUnaryExp Op");
                exit(-1);
        }
    } else if (unaryExp->getPrimaryExp() != nullptr) {
        intRet = calPrimaryExp(unaryExp->getPrimaryExp());
    } else {
        Error::errorSim("Semantic.cpp calUnaryExp call");
        exit(-1);
    }
    return intRet;
}

int Arm7Gen::calPrimaryExp(PrimaryExp *primaryExp) {
    int intRet;
    if (primaryExp->getExp() != nullptr) {
        intRet = calAddExp(primaryExp->getExp()->getAddExp());
    } else if (primaryExp->getLVal() != nullptr) {
        intRet = calLVal(primaryExp->getLVal());
    } else {
        intRet = primaryExp->getNumber();
    }
    return intRet;
}

int Arm7Gen::calLVal(LVal *lVal) {
    if (lVal->getExps()->empty()) {  /// 符号表找常量
        for (int i = (int) symbolTables->size() - 1; i > 0; i--) {
            if (symbolTables->at(i)->getFuncName() == funcNameNow ||
                symbolTables->at(i)->getFuncName() == SYMBOL_TABLE_GLOBAL_STR) {
                for (Symbol *symbol:*symbolTables->at(i)->getSymbols()) {
                    if (symbol->getArm7Var() != nullptr &&
                        symbol->getArm7Var()->getIdent() == lVal->getIdent() &&
                        symbol->getArm7Var()->getIfArray() == ARRAY_FALSE &&
                        symbol->getArm7Var()->getIfConst() == CONST_TRUE) {
                        return symbol->getArm7Var()->getValue()->at(0);
                    }
                }
            }
        }
    } else {  /// 符号表找数组
        // 索引下标
        auto *subs = new std::vector<int>();
        for (Exp *exp:*lVal->getExps()) {
            subs->push_back(calAddExp(exp->getAddExp()));
        }

        for (int i = (int) symbolTables->size() - 1; i > 0; i--) {
            if (symbolTables->at(i)->getFuncName() == funcNameNow ||
                symbolTables->at(i)->getFuncName() == SYMBOL_TABLE_GLOBAL_STR) {
                for (Symbol *symbol:*symbolTables->at(i)->getSymbols()) {
                    if (symbol->getArm7Var() != nullptr &&
                        symbol->getArm7Var()->getIdent() == lVal->getIdent() &&
                        symbol->getArm7Var()->getIfArray() == ARRAY_TRUE &&
                        symbol->getArm7Var()->getIfConst() == CONST_TRUE) {
                        if (subs->size() == symbol->getArm7Var()->getSubs()->size()) {
                            int pos = 0;  // 计算索引一维位置
                            for (int j = 0; j < subs->size(); j++) {
                                int subs_subs_len = 1;
                                for (int k = j + 1; k < subs->size(); k++) {
                                    subs_subs_len *= symbol->getArm7Var()->getSubs()->at(k);
                                }
                                pos += subs->at(j) * subs_subs_len;
                            }
                            return symbol->getArm7Var()->getValue()->at(pos);
                        } else {
                            Error::errorSim("error subs Semantic calLVal");
                            exit(-1);
                        }  // end find and return
                    }  // end find constArray ident
                }  // end symbol-table iter
            }  // end find in same func or global if judge
        }  // end all symbol-table except EXTERN
    }  // end find lVal
    Error::errorSim("undefined ident Semantic calLVal");
    exit(-1);
}
