#include "cell.h"

#include <variant>
#include <algorithm>
#include <iostream>

#include "sheet.h"



using namespace std;


FormulaCell::FormulaCell(Sheet& sheet, std::unique_ptr<IFormula> formula, IFormula::Value cellValue) 
    : sheet(sheet), formula(move(formula)), cellValue(cellValue), cacheInvalid(false)
{
}


void FormulaCell::Update(const CellHolder* parent) const {
        if (formula) {
            sheet.UpdateDependent(parent);
            cellValue = formula->Evaluate(sheet);
        }
        cacheInvalid = false;
}

ICell::Value FormulaCell::GetValue() const {
    if (std::holds_alternative<double>(cellValue))
        return std::get<double>(cellValue);
    if (std::holds_alternative<FormulaError>(cellValue))
        return std::get<FormulaError>(cellValue);
    return 0.0;
}


std::string FormulaCell::GetText() const  {
    if (formula) 
        return "=" + formula->GetExpression();
    return "";
}


std::vector<Position> FormulaCell::GetReferencedCells() const {
    if (formula) 
        return formula->GetReferencedCells();
    return {};
}


void FormulaCell::Invalidate() const {
    cacheInvalid = true;
}


bool FormulaCell::IsInvalid() const {
    return cacheInvalid;
}

IFormula::HandlingResult FormulaCell::HandleInsertedRows(int before, int count) { 

    if (formula) 
        return formula->HandleInsertedRows(before, count);
    return IFormula::HandlingResult::NothingChanged;
}

IFormula::HandlingResult FormulaCell::HandleInsertedCols(int before, int count)  {
    if (formula) 
        return formula->HandleInsertedCols(before, count);
    return IFormula::HandlingResult::NothingChanged;
}

IFormula::HandlingResult FormulaCell::HandleDeletedRows(int first, int count) {
    if (formula) 
        return formula->HandleDeletedRows(first, count);
    return IFormula::HandlingResult::NothingChanged;
}

IFormula::HandlingResult FormulaCell::HandleDeletedCols(int first, int count) {
    if (formula) 
        return formula->HandleDeletedCols(first, count);
    return IFormula::HandlingResult::NothingChanged;
}



LiteralCell::LiteralCell(std::string literal) 
: textValue(literal) 
{
    if (textValue[0] != '\'')  {
        bool containsLetter = find_if(textValue.begin(), textValue.end(),
                [](char c) { return isalpha(c); }) != textValue.end();
        if (!containsLetter) 
            try {
                cellValue = stod(textValue); 
            } 
            catch(invalid_argument& e) {
            }
        }
}


ICell::Value LiteralCell::GetValue() const  {
    if (textValue[0] == '\'') 
        return textValue.substr(1);
    if (cellValue != std::numeric_limits<double>::max())
        return cellValue;
    return textValue;
}



void CellHolder::reset(Sheet& sheet) {
    cell = nullptr;
    sheet.InvalidateCache(this);
}

void CellHolder::reset(Sheet& sheet, std::string literal) {
    cell = std::make_unique<LiteralCell>(literal); 
    sheet.InvalidateCache(this);
}

void CellHolder::reset(Sheet& sheet, std::unique_ptr<IFormula> formula, IFormula::Value cellValue) {
    cell = std::make_unique<FormulaCell>(sheet, move(formula), cellValue); //Only if new value
    sheet.InvalidateCache(this);
}

void CellHolder::reset(Sheet& sheet, std::string text, FormulaError::Category errorCategory) {
    cell = std::make_unique<ErrorCell>(text, errorCategory);
    sheet.InvalidateCache(this);
}


 ICell::Value CellHolder::GetValue() const  {
    if (cell.get() != nullptr) {
        if (auto formulaCell = dynamic_cast<FormulaCell*>(cell.get()); formulaCell != nullptr) 
            if (formulaCell->IsInvalid())
                formulaCell->Update(this);
        return cell->GetValue();
    }
    return 0.0;
}

 std::string CellHolder::GetText() const  {
    string text;
    if (cell) 
        text = cell->GetText();        
    return text;
}

 std::vector<Position> CellHolder::GetReferencedCells() const  {
    if (cell) 
        return cell->GetReferencedCells();
    return {};
}


IFormula::HandlingResult CellHolder::HandleInsertedRows(int before, int count) { 
    if (cell.get() != nullptr) 
       return cell->HandleInsertedRows(before, count);
    return IFormula::HandlingResult::NothingChanged;
}
IFormula::HandlingResult CellHolder::HandleInsertedCols(int before, int count)  {
    if (cell.get() != nullptr) 
        return cell->HandleInsertedCols(before, count);
    return IFormula::HandlingResult::NothingChanged;
}
IFormula::HandlingResult CellHolder::HandleDeletedRows(int first, int count) {
    if (cell.get() != nullptr) 
        return cell->HandleDeletedRows(first, count);
    return IFormula::HandlingResult::NothingChanged;
}
IFormula::HandlingResult CellHolder::HandleDeletedCols(int first, int count) {
    if (cell.get() != nullptr) 
        return cell->HandleDeletedCols(first, count);
    return IFormula::HandlingResult::NothingChanged;
}


bool CellHolder::IsInvalid() const {
    if (cell.get() != nullptr) 
        return cell->IsInvalid();
    return false;
}


void CellHolder::Invalidate() const {
    if (cell.get() != nullptr) 
        cell->Invalidate();
}


void CellHolder::Update() const {
    if (cell.get() != nullptr) 
        if (auto formulaCell = dynamic_cast<FormulaCell*>(cell.get()); formulaCell != nullptr) 
            if (formulaCell->IsInvalid())
                formulaCell->Update(this);
}



bool FormulaCell::holdsError() const {
    return std::holds_alternative<FormulaError>(cellValue);
}


bool CellHolder::DepCheckFlag() const  {
    if (auto formulaCell = dynamic_cast<FormulaCell*>(cell.get()); formulaCell != nullptr) {
        if (formulaCell->IsInvalid())
            formulaCell->Update(this);
        return formulaCell->holdsError();
    }
    return false; 
}


bool CellHolder::HasFormula() const {
    return cell.get() != nullptr && dynamic_cast<FormulaCell*>(cell.get()) != nullptr;
}


std::string CellHolder::GetLastCall() const {
    if (cell.get()) 
        return cell->LastCallParams();
    return "";
}


int CellHolder::totalObjects = 0;
