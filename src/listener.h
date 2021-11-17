#ifndef TABLE_LISTENER
#define TABLE_LISTENER

#include <set>


#include "sheet.h"
#include "ast.h"

#include "FormulaBaseListener.h"
#include "ANTLRErrorListener.h"



class Listener : public FormulaBaseListener {

public:
    ~Listener() = default;

    virtual void enterMain(FormulaParser::MainContext * /*ctx*/) override;
    virtual void exitMain(FormulaParser::MainContext * /*ctx*/) override;

    virtual void enterUnaryOp(FormulaParser::UnaryOpContext * /*ctx*/) override;
    virtual void exitUnaryOp(FormulaParser::UnaryOpContext * /*ctx*/) override;

    virtual void enterParens(FormulaParser::ParensContext * /*ctx*/) override;
    virtual void exitParens(FormulaParser::ParensContext * /*ctx*/) override;

    virtual void enterLiteral(FormulaParser::LiteralContext * /*ctx*/) override;
    virtual void exitLiteral(FormulaParser::LiteralContext * /*ctx*/) override;

    virtual void enterCell(FormulaParser::CellContext * /*ctx*/) override;
    virtual void exitCell(FormulaParser::CellContext * /*ctx*/) override;

    virtual void enterBinaryOp(FormulaParser::BinaryOpContext * /*ctx*/) override;
    virtual void exitBinaryOp(FormulaParser::BinaryOpContext * /*ctx*/) override;


    virtual void enterEveryRule(antlr4::ParserRuleContext * /*ctx*/) override;
    virtual void exitEveryRule(antlr4::ParserRuleContext * /*ctx*/) override;
    virtual void visitTerminal(antlr4::tree::TerminalNode * /*node*/) override;
    virtual void visitErrorNode(antlr4::tree::ErrorNode * /*node*/) override;

    IFormula::Value Execute(const ISheet& sheet) { 
        if (rootStatement)
            return rootStatement->Execute(sheet);
        return 0.0;
    }
    
    std::string Formula() {
        if (rootStatement)
            return rootStatement->Formula();
        return "";
    }

    const std::vector<CellStatement*>& GetCellsPtrs() const;
    std::vector<CellStatement*> extractCellsPtrs();
    std::unique_ptr<Statement> extractRootStatement();

private:
    std::vector<std::unique_ptr<Statement>> lastStatements;
    std::unique_ptr<Statement> rootStatement;
    std::vector<CellStatement*> referencedPtrs;  
};



class ErrorListener : public antlr4::ANTLRErrorListener {
public:
virtual void syntaxError(antlr4::Recognizer *recognizer, antlr4::Token *offendingSymbol, size_t line,
                             size_t charPositionInLine, const std::string &msg, std::exception_ptr e) override {
        throw FormulaException("Syntax error");
    }
virtual void reportAmbiguity(antlr4::Parser *recognizer, const antlr4::dfa::DFA &dfa, size_t startIndex, size_t stopIndex, bool exact,
      const antlrcpp::BitSet &ambigAlts, antlr4::atn::ATNConfigSet *configs) override {
    }
virtual void reportAttemptingFullContext(antlr4::Parser *recognizer, const antlr4::dfa::DFA &dfa, size_t startIndex, size_t stopIndex,
      const antlrcpp::BitSet &conflictingAlts, antlr4::atn::ATNConfigSet *configs) override {
    }
virtual void reportContextSensitivity(antlr4::Parser *recognizer, const antlr4::dfa::DFA &dfa, size_t startIndex, size_t stopIndex,
      size_t prediction, antlr4::atn::ATNConfigSet *configs) override {
    }
};


#endif