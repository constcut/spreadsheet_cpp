#include "listener.h"

#include <iostream>
#include <string>

using namespace std;



void Listener::enterMain(FormulaParser::MainContext* ctx) {
}


void Listener::exitMain(FormulaParser::MainContext* ctx) { 
    if (lastStatements.size() == 1) {
        unique_ptr<Statement> s = move(lastStatements[0]);
        lastStatements.clear();
        rootStatement = move(s);
    }
}


void Listener::enterUnaryOp(FormulaParser::UnaryOpContext* ctx) { 
}


void Listener::exitUnaryOp(FormulaParser::UnaryOpContext* ctx) {

    antlr4::tree::TerminalNode* operation = nullptr;
    for (auto ptr: {ctx->ADD(), ctx->SUB()}) 
        if (ptr) {
            operation = ptr;
            break;
        }

    unique_ptr<Statement> last = move(lastStatements.back());
    lastStatements.pop_back();

    auto ptr = last.get();
    if (auto parens = dynamic_cast<ParensStatement*>(ptr); parens != nullptr) {
        auto subPtr = parens->argument.get();

        if (auto bi = dynamic_cast<BinaryOperation*>(subPtr); bi != nullptr) {
            char op = bi->getOperation();
            if (op == '*' || op == '/')
                last = move(parens->argument);
        }
    }

    char op = operation->toString()[0];
    auto s = make_unique<UnaryOperation>(op, move(last));
    lastStatements.push_back(move(s));
}


void Listener::enterParens(FormulaParser::ParensContext* ctx) {
}


void Listener::exitParens(FormulaParser::ParensContext* ctx) { 
    auto ptr = lastStatements.back().get();
    bool skip = false;

    if (auto li = dynamic_cast<LiteralStatement*>(ptr); li != nullptr) 
        skip = true;
    if (auto cell = dynamic_cast<CellStatement*>(ptr); cell != nullptr)
        skip = true;   
    if (auto unOp = dynamic_cast<UnaryOperation*>(ptr); unOp != nullptr)
        skip = true;

    if (skip == false) {
        unique_ptr<Statement> last = move(lastStatements.back());
        lastStatements.pop_back();
        auto s = make_unique<ParensStatement>(move(last));
        lastStatements.push_back(move(s));
    }
}


void Listener::enterLiteral(FormulaParser::LiteralContext* ctx) { 
}


void Listener::exitLiteral(FormulaParser::LiteralContext* ctx) {
    const auto& literal = ctx->NUMBER()->toString();
    auto s = make_unique<LiteralStatement>(stod(literal));
    lastStatements.push_back(move(s));
}


void Listener::enterCell(FormulaParser::CellContext* ctx) { 
}


void Listener::exitCell(FormulaParser::CellContext* ctx) { 
    string cellName = ctx->CELL()->toString();
    auto s = make_unique<CellStatement>(move(cellName));

    referencedPtrs.push_back(s.get());
    lastStatements.push_back(move(s));
}


void Listener::enterBinaryOp(FormulaParser::BinaryOpContext* ctx) { 
}


void Listener::exitBinaryOp(FormulaParser::BinaryOpContext* ctx) { // TODO refact this monster

    unique_ptr<Statement> rhs = move(lastStatements.back());
    lastStatements.pop_back();
    unique_ptr<Statement> lhs = move(lastStatements.back());
    lastStatements.pop_back();

    antlr4::tree::TerminalNode* operation = nullptr;
    for (auto ptr: {ctx->ADD(), ctx->SUB(), ctx->MUL(), ctx->DIV()}) 
        if (ptr) {
            operation = ptr;
            break;
        }

    char op = operation->toString()[0];
    auto ptrLhs = lhs.get();
    auto ptrRhs = rhs.get();

    if (op == '/') {
        if(auto parens = dynamic_cast<ParensStatement*>(ptrLhs); parens != nullptr) {
            auto subPtr = parens->argument.get();
            bool dontMove = false;
            if (auto subOp = dynamic_cast<BinaryOperation*>(subPtr); subOp != nullptr) {
                if (subOp->getOperation() == '+' || subOp->getOperation() == '-')
                    dontMove = true;
            }
            if (dontMove == false)
                lhs = move(parens->argument);
        }
        if(auto parens = dynamic_cast<ParensStatement*>(ptrRhs); parens != nullptr) {
            auto subPtr = parens->argument.get();
            if (auto sub = dynamic_cast<LiteralStatement*>(subPtr); sub != nullptr) 
                rhs = move(parens->argument);
            if (auto sub = dynamic_cast<CellStatement*>(subPtr); sub != nullptr)
                rhs = move(parens->argument);
        }
    }


    if (op == '*') {
        if(auto parens = dynamic_cast<ParensStatement*>(ptrRhs); parens != nullptr) {
            auto subPtr = parens->argument.get();
            bool dontMove = false;
            if (auto subOp = dynamic_cast<BinaryOperation*>(subPtr); subOp != nullptr) {
                if (subOp->getOperation() == '+' || subOp->getOperation() == '-')
                    dontMove = true;
            }
            if (dontMove == false)
                rhs = move(parens->argument);
        }
        if(auto parens = dynamic_cast<ParensStatement*>(ptrLhs); parens != nullptr) {
            auto subPtr = parens->argument.get();
            bool dontMove = false;
            if (auto subOp = dynamic_cast<BinaryOperation*>(subPtr); subOp != nullptr) {
                if (subOp->getOperation() == '+' || subOp->getOperation() == '-')
                    dontMove = true;
            }
            if (dontMove == false)
                lhs = move(parens->argument);
        }
    }

    if (op == '+') {
        if(auto parens = dynamic_cast<ParensStatement*>(ptrLhs); parens != nullptr)
            lhs = move(parens->argument);
        if(auto parens = dynamic_cast<ParensStatement*>(ptrRhs); parens != nullptr) 
            rhs = move(parens->argument);
    }

    if (op == '-') {
        if(auto parens = dynamic_cast<ParensStatement*>(ptrLhs); parens != nullptr) 
            lhs = move(parens->argument);
        if(auto parens = dynamic_cast<ParensStatement*>(ptrRhs); parens != nullptr) {
            auto subPtr = parens->argument.get();
            bool dontMove = false;
            if (auto subOp = dynamic_cast<BinaryOperation*>(subPtr); subOp != nullptr) {
                if (subOp->getOperation() == '+' || subOp->getOperation() == '-')
                    dontMove = true;
            }
            if (dontMove == false)
                rhs = move(parens->argument);
        }
    }

    auto s = make_unique<BinaryOperation>(op, move(lhs), move(rhs));
    lastStatements.push_back(move(s));
}


void Listener::enterEveryRule(antlr4::ParserRuleContext* ctx) {
}


void Listener::exitEveryRule(antlr4::ParserRuleContext* ctx) { 

}


void Listener::visitTerminal(antlr4::tree::TerminalNode* node) { 
}


void Listener::visitErrorNode(antlr4::tree::ErrorNode* node) {
    throw FormulaException("Parsing issue, unknown token: " + node->getText());
}


const vector<CellStatement*>& Listener::GetCellsPtrs() const {
    return referencedPtrs;
}


std::vector<CellStatement*> Listener::extractCellsPtrs() {
    return move(referencedPtrs);
}


std::unique_ptr<Statement> Listener::extractRootStatement() {
    return move(rootStatement);
}
