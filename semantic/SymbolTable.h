#ifndef COMPILER_SIMBOLTABLE_H
#define COMPILER_SIMBOLTABLE_H

#include <string>
#include <utility>
#include <vector>

class Symbol;

/**
 * 全局的/某一代码块中的符号表
 */
class SymbolTable {
private:
    /// 符号表类型 global, local, extern...
    int type;

    std::vector<Symbol *> *symbols;

public:
    /**
     *
     * @param type
     * @param symbol
     */
    SymbolTable(int type, std::vector<Symbol *> *symbols) : type(type), symbols(symbols) {};

    [[nodiscard]] int getType() const {
        return type;
    }

    [[nodiscard]] const std::vector<Symbol *> *getSymbols() const {
        return symbols;
    }

    void setSymbols(std::vector<Symbol *> *symbol) {
        SymbolTable::symbols = symbol;
    }

    void addSymbol(Symbol *symbol) {
        symbols->emplace_back(symbol);
    }

};

/**
 * 变量符号 Var
 */
class Var {
private:
    std::string ident;

    int varType;

    int value, hasInited;

    unsigned int row, column;

public:
    /**
     *
     * @param ident TYPE_STR 时存格式串（相当于写死了）
     * @param varType
     * @param value
     * @param hasInited
     * @param row
     * @param column
     */
    Var(const char *ident, int varType, int value, int hasInited, unsigned int row, unsigned int column) :
            ident(ident), varType(varType), value(value), hasInited(hasInited), row(row),
            column(column) {};

    [[nodiscard]] std::string getIdent() const {
        return ident;
    }

    [[nodiscard]] int getVarType() const {
        return varType;
    }

    [[nodiscard]] int getValue() const {
        return value;
    }

    void setValue(int value_) {
        Var::value = value_;
    }

    [[nodiscard]] int getHasInited() const {
        return hasInited;
    }

    /**
     * 维护是否已经初始化
     * @param hasInited
     */
    void setHasInited(int hasInited_) {
        Var::hasInited = hasInited_;
    }

    [[nodiscard]] unsigned int getRow() const {
        return row;
    }

    [[nodiscard]] unsigned int getColumn() const {
        return column;
    }

};

/**
 * 常量符号 ConstVar
 */
class ConstVar {
private:
    std::string ident;

    int varType;

    int value;

    unsigned int row, column;

public:
    /**
     *
     * @param ident
     * @param varType
     * @param value
     * @param row
     * @param column
     */
    ConstVar(const char *ident, int varType, int value, unsigned int row, unsigned int column) :
            ident(ident), varType(varType), value(value), row(row), column(column) {};

    [[nodiscard]] std::string getIdent() const {
        return ident;
    }

    [[nodiscard]] int getVarType() const {
        return varType;
    }

    [[nodiscard]] int getValue() const {
        return value;
    }

    [[nodiscard]] unsigned int getRow() const {
        return row;
    }

    [[nodiscard]] unsigned int getColumn() const {
        return column;
    }

};

/**
 * 数组变量符号 VarArray
 */
class VarArray {
private:
    std::string ident;

    int varType;
    /// 目前仅int
    std::vector<int> *value;
    /// 维度们
    std::vector<int> *subs;

    int hasInited;

    unsigned int row, column;
public:
    /**
     *
     * @param ident
     * @param varType
     * @param value
     * @param subs
     * @param hasInited
     * @param row
     * @param column
     */
    VarArray(const char *ident, int varType, std::vector<int> *value, std::vector<int> *subs, int hasInited,
             unsigned int row, unsigned int column) :
            ident(ident), varType(varType), value(value), subs(subs), hasInited(hasInited), row(row), column(column) {};

    [[nodiscard]] const std::string &getIdent() const {
        return ident;
    }

    [[nodiscard]] int getVarType() const {
        return varType;
    }

    [[nodiscard]] const std::vector<int> *getValue() const {
        return value;
    }

    void setValue(std::vector<int> *value_) {
        VarArray::value = value_;
    }

    [[nodiscard]] const std::vector<int> *getSubs() const {
        return subs;
    }

    [[nodiscard]] int getHasInited() const {
        return hasInited;
    }

    void setHasInited(int hasInited_) {
        VarArray::hasInited = hasInited_;
    }

    [[nodiscard]] unsigned int getRow() const {
        return row;
    }

    [[nodiscard]] unsigned int getColumn() const {
        return column;
    }
};

/**
 * 数组常量符号 ConstVarArray
 */
class ConstVarArray {
private:
    std::string ident;

    int varType;
    /// 目前仅int
    std::vector<int> *value;
    /// 维度们
    std::vector<int> *subs;

    unsigned int row, column;
public:
    /**
     *
     * @param ident
     * @param varType
     * @param value
     * @param subs 数组参数，null 表一维; 变量不为null
     * @param row
     * @param column
     */
    ConstVarArray(const char *ident, int varType, std::vector<int> *value, std::vector<int> *subs,
                  unsigned int row, unsigned int column) :
            ident(ident), varType(varType), value(value), subs(subs), row(row), column(column) {};

    [[nodiscard]] const std::string &getIdent() const {
        return ident;
    }

    [[nodiscard]] int getVarType() const {
        return varType;
    }

    [[nodiscard]] const std::vector<int> *getValue() const {
        return value;
    }

    [[nodiscard]] const std::vector<int> *getSubs() const {
        return subs;
    }

    [[nodiscard]] unsigned int getRow() const {
        return row;
    }

    [[nodiscard]] unsigned int getColumn() const {
        return column;
    }

    void setValue(std::vector<int> *value_) {
        ConstVarArray::value = value_;
    }
};

/**
 * 函数符号 Func
 */
class Func {
private:
    std::string ident;

    int retType;

    std::vector<Symbol *> *params;

    unsigned int row, column;

public:
    /**
     *
     * @param ident
     * @param retType
     * @param paramsType
     * @param paramsName
     * @param row
     * @param column
     */
    Func(const char *ident, int retType, std::vector<Symbol *> *params, unsigned int row,
         unsigned int column) :
            ident(ident), retType(retType), params(params), row(row), column(column) {};

    [[nodiscard]] std::vector<Symbol *> *getParams() const {
        return params;
    }

    [[nodiscard]] const std::string &getIdent() const {
        return ident;
    }

    [[nodiscard]] int getRetType() const {
        return retType;
    }

    [[nodiscard]] unsigned int getRow() const {
        return row;
    }

    [[nodiscard]] unsigned int getColumn() const {
        return column;
    }
};

class Symbol {
private:
    Var *varInner;
    VarArray *varArrayInner;
    ConstVar *constVarInner;
    ConstVarArray *constVarArrayInner;
    Func *funcInner;

public:
    Symbol(Var *var, VarArray *varArray, ConstVar *constVar, ConstVarArray *constVarArray, Func *func) :
            varInner(var), varArrayInner(varArray), constVarInner(constVar), constVarArrayInner(constVarArray),
            funcInner(func) {};

    [[nodiscard]] Var *getVarInner() const {
        return varInner;
    }

    [[nodiscard]] VarArray *getVarArrayInner() const {
        return varArrayInner;
    }

    [[nodiscard]] ConstVar *getConstVarInner() const {
        return constVarInner;
    }

    [[nodiscard]] ConstVarArray *getConstVarArrayInner() const {
        return constVarArrayInner;
    }

    [[nodiscard]] Func *getFuncInner() const {
        return funcInner;
    }
};

#endif //COMPILER_SIMBOLTABLE_H
