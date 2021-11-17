
#include "ast.h"


#include <numeric>
#include <algorithm>
#include <cmath>


using namespace std;


LiteralStatement::LiteralStatement(double v) 
    : value(v) {}


IFormula::Value LiteralStatement::Execute(const ISheet& sheet) const  {
    return value;
}


string LiteralStatement::Formula() const  {
    if (value == round(value))
        return to_string(static_cast<int>(value));
    return to_string(value);
}



CellStatement::CellStatement(string name)
    : pos(Position::FromString(move(name))) {}


IFormula::Value CellStatement::Execute(const ISheet& sheet) const  {

    if (pos.col == -1 && pos.row == -1) //-1, -1 ref deletion
        return FormulaError(FormulaError::Category::Value);

    if (pos.IsValid() == false) //-2, -2 parse error
        throw FormulaException("Invalid position");

    auto cellPtr = sheet.GetCell(pos);
    if (cellPtr == nullptr)
        return 0.0;
    auto cellValue =  cellPtr->GetValue();
    
    if (holds_alternative<string>(cellValue)) {
        const auto& cellText = get<string>(cellValue);
        bool containsLetter = find_if(cellText.begin(), cellText.end(),
                [](char c) { return isalpha(c); }) != cellText.end();
        if (containsLetter)
                return FormulaError(FormulaError::Category::Value);
        try {
            double temp = stod(cellText);
            return temp;
        } 
        catch(invalid_argument& e) {
            return FormulaError(FormulaError::Category::Value);
        }
    }
    else if (holds_alternative<double>(cellValue)) 
        return get<double>(cellValue);

    return get<FormulaError>(cellValue); 
}


string CellStatement::Formula() const  {
    const auto& posStr = pos.ToString();
    if (posStr.empty())
        return "#!REF";
    return posStr; 
}


void CellStatement::setNewName(string newName) {
    pos = Position::FromString(newName);
}



UnaryOperation::UnaryOperation(char op, unique_ptr<Statement> argument) 
    : argument(move(argument)), operation(op) {}


IFormula::Value UnaryOperation::Execute(const ISheet& sheet) const  {
    IFormula::Value value = argument->Execute(sheet);
    if (holds_alternative<FormulaError>(value)) 
        return value;
    double temp  = get<double>(value);
    if (operation == '-')
        return -temp;
    return temp;    
}


string UnaryOperation::Formula() const  {
    return string(1, operation) + argument->Formula();
}



BinaryOperation::BinaryOperation(char op, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs)
: lhs(move(lhs)), rhs(move(rhs)), operation(op) { }


IFormula::Value BinaryOperation::Execute(const ISheet& sheet) const  {

    auto rhsExec = rhs->Execute(sheet);
    auto lhsExec = lhs->Execute(sheet);
    if (holds_alternative<FormulaError>(lhsExec))
        return lhsExec;
    if (holds_alternative<FormulaError>(rhsExec))
        return rhsExec;
    double lhsValue = get<double>(lhsExec);
    double rhsValue = get<double>(rhsExec);

    if (operation == '+') {
        double temp = lhsValue + rhsValue;
        if (isfinite(temp) == false)
            return FormulaError(FormulaError::Category::Div0);
        return temp;
    }
    else if (operation == '-') {
        double temp = lhsValue - rhsValue;
        if (isfinite(temp) == false)
            return FormulaError(FormulaError::Category::Div0);
        return temp;
    }
    else if (operation == '*') {
        double temp = lhsValue * rhsValue;
        if (isfinite(temp) == false)
            return FormulaError(FormulaError::Category::Div0);
        return temp;
    }
    else if (operation == '/') {
        if (rhsValue <= 1e-200)
            return FormulaError(FormulaError::Category::Div0);   
        return lhsValue / rhsValue;
    }
    return 0.0; //Ошибка должна быть на уровне синтаксиса - в реальности мы не должны сюда дойти
}


string BinaryOperation::Formula() const  {
    return lhs->Formula() + string(1, operation)
        + rhs->Formula();
}


char BinaryOperation::getOperation() {
    return operation;
}


ParensStatement::ParensStatement(unique_ptr<Statement> argument)
    : argument(move(argument)) {}


IFormula::Value ParensStatement::Execute(const ISheet& sheet) const  {
    return argument->Execute(sheet);
}


string ParensStatement::Formula() const  {
    return "(" + argument->Formula() + ")";
}