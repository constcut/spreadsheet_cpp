#ifndef TABLE_AST
#define TABLE_AST

#include "formula.h"
#include "common.h"



struct Statement {
    virtual ~Statement() = default;
    virtual IFormula::Value Execute(const ISheet& sheet) const = 0;
    virtual std::string Formula() const = 0;
};


struct LiteralStatement : Statement {
    double value;
    explicit LiteralStatement(double v);
    IFormula::Value Execute(const ISheet& sheet) const override;
    std::string Formula() const override;
};


struct CellStatement : Statement {
    Position pos;

    explicit CellStatement(std::string name);
    IFormula::Value Execute(const ISheet& sheet) const override;
    std::string Formula() const override;
    void setNewName(std::string newName);
};



class UnaryOperation : public Statement {
public:
    UnaryOperation(char op, std::unique_ptr<Statement> argument);
    IFormula::Value Execute(const ISheet& sheet) const override;
    std::string Formula() const override;
private:
    std::unique_ptr<Statement> argument;
    char operation;
};



class BinaryOperation : public Statement {
public:
    BinaryOperation(char op, std::unique_ptr<Statement> lhs, 
        std::unique_ptr<Statement> rhs);
    IFormula::Value Execute(const ISheet& sheet) const override;
    std::string Formula() const override;
    char getOperation();
private:
    std::unique_ptr<Statement> lhs, rhs;
    char operation;
};


struct ParensStatement : public Statement {
    ParensStatement(std::unique_ptr<Statement> argument);
    IFormula::Value Execute(const ISheet& sheet) const override;
    std::string Formula() const override;
    std::unique_ptr<Statement> argument;
};


#endif 