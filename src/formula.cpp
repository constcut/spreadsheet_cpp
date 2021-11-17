#include "formula_impl.h"

#include <sstream>
#include <algorithm>

#include "FormulaParser.h"
#include "FormulaLexer.h"
#include "listener.h"



using namespace std;

Formula::Formula(Listener *l) {
    rootStatement = move(l->extractRootStatement());
    referencedPtrs = move(l->extractCellsPtrs());
    UpdateRefs();
}
    

IFormula::Value Formula::Evaluate(const ISheet& sheet) const  {
    return rootStatement->Execute(sheet);;
}


std::string Formula::GetExpression() const  {
    return rootStatement->Formula();
}


std::vector<Position> Formula::GetReferencedCells() const {
    return refCells;
}


void Formula::UpdateRefs() {
    refCells.clear();
    set<Position> s; 
    for (const auto& cellPtr: referencedPtrs)
        if (cellPtr->pos.IsValid())
            s.insert(cellPtr->pos);    

    for (const auto& pos: s) 
        if (pos.IsValid())
            refCells.push_back(pos);
}



IFormula::HandlingResult Formula::HandleInsertedRows(int before, int count) {
    auto handlingResult = IFormula::HandlingResult::NothingChanged;
    const auto& cellsPtrs = referencedPtrs;
    for (const auto ptr: cellsPtrs) {
        auto& pos = ptr->pos;
        if (pos.row >= before) {
            pos.row += count;
            if (pos.row >= 16384)
                throw TableTooBigException("Cannot move col");
            handlingResult = IFormula::HandlingResult::ReferencesRenamedOnly;
        }
    }

    if (handlingResult == IFormula::HandlingResult::ReferencesRenamedOnly)
        UpdateRefs();

    return handlingResult;
}


IFormula::HandlingResult Formula::HandleInsertedCols(int before, int count) {

    auto handlingResult = IFormula::HandlingResult::NothingChanged;

    const auto& cellsPtrs = referencedPtrs;
    for (const auto ptr: cellsPtrs) {
        auto& pos = ptr->pos; 
        if (pos.col >= before) {
            pos.col += count;
            if (pos.col >= 16384)
                throw TableTooBigException("Cannot move col");
            handlingResult = IFormula::HandlingResult::ReferencesRenamedOnly;
        }
    }

    if (handlingResult == IFormula::HandlingResult::ReferencesRenamedOnly)
        UpdateRefs();

    return handlingResult;
}


IFormula::HandlingResult Formula::HandleDeletedRows(int first, int count) {
    auto handlingResult = IFormula::HandlingResult::NothingChanged;

    const auto& cellsPtrs = referencedPtrs;
    for (const auto ptr: cellsPtrs) {
        auto& pos = ptr->pos; 
        if (pos.row >= first && pos.row <= (first + count - 1)) {
            handlingResult = IFormula::HandlingResult::ReferencesChanged;
            pos = {-1, -1}; 
        }
        else if (pos.row > (first + count - 1))  {
            pos.row -= count; 
            if (handlingResult != IFormula::HandlingResult::ReferencesChanged)
                handlingResult = IFormula::HandlingResult::ReferencesRenamedOnly;
        }
    }

    if (handlingResult != IFormula::HandlingResult::NothingChanged)
        UpdateRefs();

    return handlingResult;
}


IFormula::HandlingResult Formula::HandleDeletedCols(int first, int count) {
    auto handlingResult = IFormula::HandlingResult::NothingChanged;
    const auto& cellsPtrs = referencedPtrs; 
    for (const auto ptr: cellsPtrs) {
        auto& pos = ptr->pos; 
        if (pos.col >= first && pos.col <= (first + count - 1)) {
            handlingResult = IFormula::HandlingResult::ReferencesChanged;
            pos = {-1, -1}; 
        }
        else if (pos.col > (first + count - 1))  {
            pos.col -= count; 
            if (handlingResult != IFormula::HandlingResult::ReferencesChanged)
                handlingResult = IFormula::HandlingResult::ReferencesRenamedOnly;
        }
    }
    if (handlingResult != IFormula::HandlingResult::NothingChanged)
        UpdateRefs();
    return handlingResult;
}


std::unique_ptr<IFormula> ParseFormula(std::string expression) {
    static ErrorListener errListener;
    static Listener l;
    antlr4::ANTLRInputStream input(move(expression));
    FormulaLexer lexer(&input);
    lexer.removeErrorListeners();
    lexer.addErrorListener(&errListener);
    antlr4::CommonTokenStream tokens(&lexer);
    FormulaParser parser(&tokens);
    parser.removeErrorListeners();
    parser.addErrorListener(&errListener);
    antlr4::tree::ParseTree* tree = parser.main();
    antlr4::tree::ParseTreeWalker::DEFAULT.walk(&l, tree);
    return std::make_unique<Formula>(&l);
}