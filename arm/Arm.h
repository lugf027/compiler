//
// Created by 77350 on 2020/8/1.
//

#ifndef COMPILER_ARM_H
#define COMPILER_ARM_H


#include "../ir/IRGlobal.h"

class ArmBuilder {
    // ir的stmts
    std::vector<IRStmt *> *stmts;
    // 存储解析后的armStmts
    std::vector<ArmStmt *> *armStmts;

    StackStatus *status;

    int count = 0;
public:
    std::vector<ArmStmt *> *getArmStmts() const {
        return armStmts;
    }

public:

    explicit ArmBuilder(std::vector<IRStmt *> *stmts, StackStatus *stackStatus) {
        this->stmts = stmts;
        this->status = stackStatus;
        armStmts = new std::vector<ArmStmt *>();
    }

    void generateArmStmts(const char *string, std::vector<IRGlobalFuncParam *> *funcParms) {

    }



    /// 生成函数入栈和获取实参的armstmt

    std::vector<ArmStmt*> genEntryParam(std::vector<IRGlobalFuncParam *> *parmsInfo);

    /// 全局变量处理

    static std::vector<std::string> genGlobal(IRGlobalVar *var);

    static std::vector<std::string> genConGlobal(IRGlobalVar *var);

    static std::vector<std::string> genGlobalArray(IRGlobalVar *var);

    static std::vector<std::string> genConGlobalArray(IRGlobalVar *var);

    /// 声明语句
    std::vector<ArmStmt*> genAlloca(IRStmt *irStmt);

    std::vector<ArmStmt*> genConAlloca(IRStmt *irStmt);

    std::vector<ArmStmt*> genAllocaArray(IRStmt *irStmt);

    std::vector<ArmStmt*> genConAllocaArray(IRStmt *irStmt);

    /// 运算
    std::vector<ArmStmt*> genAddNode(IRStmt *irStmt);

    std::vector<ArmStmt*> genSubNode(IRStmt *irStmt);

    std::vector<ArmStmt*> genMulNode(IRStmt *irStmt);

    std::vector<ArmStmt*> genDivNode(IRStmt *irStmt);

    std::vector<ArmStmt*> genModNode(IRStmt *irStmt);

    std::vector<ArmStmt*> genAndNode(IRStmt *irStmt);

    std::vector<ArmStmt*> genOrNode(IRStmt *irStmt);

    std::vector<ArmStmt*> genEQNode(IRStmt *irStmt);

    std::vector<ArmStmt*> genGTNode(IRStmt *irStmt);

    std::vector<ArmStmt*> genLTNode(IRStmt *irStmt);

    std::vector<ArmStmt*> genNEQNode(IRStmt *irStmt);

    std::vector<ArmStmt*> genLTENode(IRStmt *irStmt);

    std::vector<ArmStmt*> genGTENode(IRStmt *irStmt);

    // 单目运算

    std::vector<ArmStmt*> genPlus(IRStmt *irStmt);

    std::vector<ArmStmt*> genMinus(IRStmt *irStmt);

    std::vector<ArmStmt*> genExclamation(IRStmt *irStmt);


    /// 控制语句
    std::vector<ArmStmt*> genBRNode(IRStmt *irStmt);

    std::vector<ArmStmt*> genBRCondNode(IRStmt *irStmt);

    std::vector<ArmStmt*> genRetNode(IRStmt *irStmt);

    /// 函数调用
    std::vector<ArmStmt*> genCallNode(IRStmt *irStmt);

    ///存取语句
    std::vector<ArmStmt*> genStoreNode(IRStmt *irStmt);

    std::vector<ArmStmt*> genLoadNode(IRStmt *irStmt);

    std::vector<ArmStmt*> genGetPtrNode(IRStmt *irStmt);

};


class ArmGen {
private:
    IRTree *irTree;
    StackStatus *globalStatus;
private:
    /// 处理全局变量
    void IRGlobalValGen(IRGlobalVar *irGlobalVar) {

        if (irGlobalVar->getVarType() == TYPE_INT) { // int

            //记录变量位置,写入维度信息
            std::multimap<std::string, VarInfo> varMap = *(globalStatus->getVarMap());

            varMap.emplace(irGlobalVar->getVarName(), VarInfo{globalStatus->getCurrentLoc(), 0});

            globalStatus->setCurrentLoc(-1);
            globalStatus->setVarMap(&varMap);
        } else if (irGlobalVar->getVarType() == TYPE_INT_STAR) { //int*

            //记录变量位置,写入维度信息
            std::multimap<std::string, VarInfo> varMap = *(globalStatus->getVarMap());
            std::multimap<std::string, ArrayInfo> dimMap = *(globalStatus->getArrDimensionMap());

            //维度信息写入到dims]
            std::vector<int> dims = *(irGlobalVar->getArraySubs());

            varMap.emplace(irGlobalVar->getVarName(), VarInfo{globalStatus->getCurrentLoc(), 0});
            dimMap.emplace(irGlobalVar->getVarName(), ArrayInfo{dims, 0});

            globalStatus->setCurrentLoc(-1);
            globalStatus->setArrDimensionMap(&dimMap);
            globalStatus->setVarMap(&varMap);

        } else {
            perror("Error in ArmDAGGen.IRGloabalVarGen()!\n"
                   "unexpected vartype. \n");
        }
    }

    /// 处理函数
    void IRGlobalFuncGen(IRGlobalFunc *irGlobalFunc) {
        StackStatus *stackStatus = irGlobalFunc->getStackStatus();

        //  深度拷贝globalStatus的varMap
        auto *varMap = new std::multimap<std::string, VarInfo>();
        *varMap = *globalStatus->getVarMap();
        stackStatus->setVarMap(varMap);
        //  深度拷贝globalStatus的varMap
        auto *arrDimensionMap = new std::multimap<std::string, ArrayInfo>();
        *arrDimensionMap = *globalStatus->getArrDimensionMap();
        stackStatus->setArrDimensionMap(arrDimensionMap);

        // 开始遍历BaseBlocks并处理
        for (auto &it : *irGlobalFunc->getBaseBlocks()) {
            // 开始处理IrStmts
            IrStmtsGen(it->getStmts(), it->getBlockName().c_str(), irGlobalFunc->getArmBlocks(),
                       irGlobalFunc->getStackStatus(), irGlobalFunc->getIrGlobalFuncParams());
        }
    }

    /**
     * 处理DagRoot
     * @param dagRoot：待解析的dagRoot(解析dagRoot生成armDagRoot)
     * @param armBlocks：将生成的dagRoot放到这里
     * @param stackStatus：该dagRoot所在函数的栈状态信息
     */
    void IrStmtsGen(std::vector<IRStmt *> *stmts, const char *blockName, std::vector<ArmBlock *> *armBlocks,
                    StackStatus *stackStatus,
                    std::vector<IRGlobalFuncParam *> *parmsInfo) {
        auto armBuilder = new ArmBuilder(stmts, stackStatus);
        // ==== todo 处理  ========

        armBuilder->generateArmStmts(blockName, parmsInfo);


        // ===========
        ArmBlock *armBlock = new ArmBlock(blockName, armBuilder->getArmStmts());
        armBlocks->emplace_back(armBlock);

        delete armBuilder;
    }

public:
    explicit ArmGen(IRTree *irTree) {
        this->irTree = irTree;
        globalStatus = new StackStatus();
    };

    /// 启动函数
    void startGen() {
        auto iRGlobals = this->irTree->getIrGlobals();
        // 开始遍历iRGlobals
        for (auto &irGlobal : *iRGlobals) {
            if (irGlobal->getIrGlobalVar() != nullptr) {
                this->IRGlobalValGen(irGlobal->getIrGlobalVar());
            }
            if (irGlobal->getIrGlobalFunc() != nullptr) {
                this->IRGlobalFuncGen(irGlobal->getIrGlobalFunc());
            }
        }
    };
};


#endif //COMPILER_ARM_H