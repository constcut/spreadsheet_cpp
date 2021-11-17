#include "sheet.h"

#include <iostream>
#include <algorithm>
#include <unordered_set>

#include "formula_impl.h"
#include "formula.h"


//#include "memchecker.h"

using namespace std;


Sheet::Sheet() {
}


bool Sheet::textHasFormula(const string& text) {
    return text.empty() == false && text[0]=='=' 
        && text.size() > 1;
}



bool Sheet::CellExists(const Position& pos) const {
    if (cells.size() > static_cast<size_t>(pos.row)) {
        const auto& row = cells[pos.row];
        if (row.size() > static_cast<size_t>(pos.col)) 
            return row[pos.col].get() != nullptr;
    }
    return false;
}


bool Sheet::CellShouldExist(const Position& pos) const {
    if (cells.size() > static_cast<size_t>(pos.row)) {
        const auto& row = cells[pos.row];
        if (row.size() > static_cast<size_t>(pos.col)) 
            return true;
    }
    return false;
}


Sheet::CellPtr& Sheet::CreateCell(const Position& pos) {

    if (pos.IsValid() == false)
        throw InvalidPositionException("Sheet::CreateCell");

    if (rowsCount <= pos.row) 
        rowsCount = pos.row + 1;
    if (colsCount <= pos.col) 
        colsCount = pos.col + 1;

    size_t rowIdx = static_cast<size_t>(pos.row);
    size_t colIdx = static_cast<size_t>(pos.col);

    if (cells.size() <= rowIdx) {
        cells.resize(rowsCount);
    }

    auto& row = cells.at(rowIdx);
    if (row.empty() == false) {
        if (row.size() <= colIdx) {
            size_t resizeAmount = pos.col + 1; //static_cast<size_t>(max(colsCount, pos.col + 1));
            row.resize(resizeAmount);
        }
    }
    else {
        size_t resizeAmount = pos.col + 1; //static_cast<size_t>(max(colsCount, pos.col + 1));
        cells.at(rowIdx) = Sheet::TableRow(resizeAmount);
    }

    if (cells.at(rowIdx).at(colIdx).get() == nullptr)
        cells.at(rowIdx).at(colIdx) = make_unique<CellHolder>();

    return cells.at(rowIdx).at(colIdx) ;
}



CellHolder* Sheet::GetCellPtr(const Position& pos) const {
    size_t rowIdx = static_cast<size_t>(pos.row);
    size_t colIdx = static_cast<size_t>(pos.col);
    return cells[rowIdx][colIdx].get();
}



bool Sheet::CheckDependency(Position pos, const vector<Position> &refs) const {
    for (const auto& refPos: refs) {
       if (pos  == refPos)
            return true;
        if (CellExists(refPos)) {
            auto cellPtr = GetCellPtr(refPos);
            if (cellPtr->DepCheckFlag())
                if (cellPtr->usedBy.empty() == false) 
                    if (CheckDependency(pos, cellPtr->GetReferencedCells()))
                        return true;
        }
    }
    return false;
}


void Sheet::ClearUsedGraph(CellHolder* cell, const std::vector<Position>& refs) {
    if (refs.empty() == false) {
        for (const auto& refPos: refs) 
            if (CellExists(refPos)) {
                auto& usedVec = GetCellPtr(refPos)->usedBy;
                auto it = find(usedVec.begin(), usedVec.end(), cell);
                if (it != usedVec.end())
                    usedVec.erase(it);
            }
    } 
}



void Sheet::SetCell(Position pos, string text) {

    //cerr << "Set " << pos.ToString() << " " << text << endl;

    //static int count = 0; 
    //++count;

    //if (count >= 16000 && count < 16300)
        ////cerr << pos.ToString() << " " << text << " " << count << endl;
                    
    //if (getRAM() > 133'500'000) throw runtime_error("hi?");

    if (pos.IsValid() == false) 
        throw InvalidPositionException("Position invalid");

    bool cellExisted = false;
    if (CellExists(pos) == false)
        CreateCell(pos);
    else
        cellExisted = true;

    auto cell = GetCellPtr(pos);
    if (cell->GetLastCall() == text)
        return;

    if (auto refs = cell->GetReferencedCells(); refs.empty() == false)  
        ClearUsedGraph(cell, refs);
    
    if (textHasFormula(text)) {
        HandleFormulaCreation(pos, move(text), cellExisted);
    }
    else {
        if (text.empty())
            cell->reset(*this);
        else
            cell->reset(*this, text); 
    }

    if (cell->usedBy.empty() == false)
        for (const auto& depCell: cell->usedBy) 
            if (depCell->IsInvalid() == false)
                InvalidateCache(depCell);
}


void Sheet::HandleFormulaCreation(Position pos, string text, bool cellExisted) {
    auto cell = GetCellPtr(pos);
    unique_ptr<IFormula> preFormula;
    try {
        preFormula = ParseFormula(move(text.substr(1)));
    }
    catch(out_of_range& e) {
        cell->reset(*this, text, FormulaError::Category::Div0);
        return;
    } 
    const auto& refs = preFormula->GetReferencedCells();
    /*if (refs.empty()) { //Possible optimization for memory in some cases
        auto expr = "=" + preFormula->GetExpression();
        auto value = preFormula->Evaluate(*this);
        cell->reset(expr, value);
    }*/

    if (refs.empty() == false) 
        for (const auto& refPos: refs) 
            if (CellExists(refPos)) 
                GetCellPtr(refPos)->usedBy.push_back(cell);

    if (CheckDependency(pos, refs)) {
        ClearUsedGraph(cell, refs);
        if (cellExisted == false)
            ClearCell(pos);
        throw CircularDependencyException("Failed");
    }
    for (const auto& refPos: refs) 
            if (CellExists(refPos)) {
                CellHolder* refCell = GetCellPtr(refPos);
                if (refCell->IsInvalid())
                    UpdateChache(refCell);
            }
    IFormula::Value cellValue;
    try {
        cellValue = preFormula->Evaluate(*this);
    }
    catch(FormulaException& e) {
        ClearUsedGraph(cell, refs);
        if (cellExisted == false)
            ClearCell(pos);
        throw e;
    }
    cell->reset(*this, move(preFormula), cellValue);
    if (refs.empty() == false) 
        for (const auto& refPos: refs) 
            if (CellExists(refPos) == false ) {
                auto refCell = CreateCell(refPos).get();
                refCell->usedBy.push_back(cell);
            }
}


ICell* Sheet::GetCell(Position pos)  {
    if (pos.IsValid() == false) {
        throw InvalidPositionException("Position invalid");
    }
    if (CellExists(pos) == 0) {
        return nullptr;
    }
    return GetCellPtr(pos);
}


const ICell* Sheet::GetCell(Position pos) const  {
    if (pos.IsValid() == false) {
        throw InvalidPositionException("Position invalid");
    }
    if (CellExists(pos) == 0) {
        return nullptr;
    }
    return GetCellPtr(pos);
}


void Sheet::UpdateChache(const CellHolder * const cellPtr) const{
    for (const auto& depPos: cellPtr->GetReferencedCells()) {
        if (CellExists(depPos)) {
            CellHolder* depPtr = GetCellPtr(depPos);
            if (depPtr->IsInvalid()) 
                UpdateChache(depPtr);   
        }   
    }      
    if (cellPtr->HasFormula()) { 
        cellPtr->Update();
    }   
}


void Sheet::UpdateDependent(const CellHolder * const cellPtr) const{
    for (const auto& depPos: cellPtr->GetReferencedCells()) {
        if (CellExists(depPos)) {
            CellHolder* depPtr = GetCellPtr(depPos);
            if (depPtr != nullptr && depPtr->IsInvalid()) 
                UpdateChache(depPtr);   
        }   
    }      
}


void Sheet::ClearCell(Position pos)  {
    if (pos.IsValid() == false) 
        throw InvalidPositionException("Position invalid");
    if (CellExists(pos)) {
        CellHolder* cell = GetCellPtr(pos);
        InvalidateCache(cell);
        ClearGraph(cell);
        if (GetCellPtr(pos) != nullptr) {
            size_t rowIdx = static_cast<size_t>(pos.row);
            size_t colIdx = static_cast<size_t>(pos.col);
            cells[rowIdx][colIdx] = nullptr;
        }
    }
    if (CellHolder::getTotalObject() == 0) {
        colsCount = 0;
        rowsCount = 0;
    }
}


void Sheet::ClearGraph(CellHolder* cellPtr) {
    for (const auto& depPos: cellPtr->GetReferencedCells()) {
        if (CellExists(depPos) == false)
            continue;
        CellHolder* depPtr = GetCellPtr(depPos);
        auto& usedVec = depPtr->usedBy;
        auto it = find(usedVec.begin(), usedVec.end(), cellPtr);
        if (it != usedVec.end())
            usedVec.erase(it);
    }      
    cellPtr->usedBy.clear();
}


void Sheet::UpdateFormulaOnInsert(CellHolder* cellPtr, unordered_set<CellHolder*>& allreadyChanged,
     int before, int count, bool row) const {
    if (cellPtr == 0)
        return;
    if (cellPtr->usedBy.empty() == false) { 
        for (const auto refPtr: cellPtr->usedBy) {
            if (allreadyChanged.count(refPtr))
                continue;
            if (row)
                refPtr->HandleInsertedRows(before, count);
            else
                refPtr->HandleInsertedCols(before, count); 
            allreadyChanged.insert(refPtr);
        }
    }
}


void Sheet::InsertRows(int before, int count)  {
    if ((rowsCount + count) >= 16384)
                throw TableTooBigException("Cannot insert new row");
    rowsCount += count; 
    cells.resize(rowsCount); 
    unordered_set<CellHolder*> allreadyChanged;
    for (int i = before; i < (before + count); ++i) {
        if (cells[i].empty() == false) {
            for (auto& cell: cells[i])
                UpdateFormulaOnInsert(cell.get(), allreadyChanged, before, count, true);
            cells[i + count] = move(cells[i]);
        }
        else {
            cells[i] = Sheet::TableRow();
            cells[i + count] = move(cells[i]);
        }
    }
}


void Sheet::InsertCols(int before, int count) {
    if ((colsCount + count) >= 16384)
                throw TableTooBigException("Cannot insert new row"); 
    colsCount += count;
    unordered_set<CellHolder*> allreadyChanged;
    for (size_t i = 0; i < static_cast<size_t>(rowsCount); ++i) {
        if (cells[i].empty() == false) {
            auto& row = cells[i];
            if (row.size() <= static_cast<size_t>(colsCount)) {
                row.resize(colsCount);
            }
            int border = colsCount - count; 
            for (int j = border-1; j >= before; --j) {
                size_t idx1 = static_cast<size_t> (j + count);
                size_t idx2 = static_cast<size_t> (j);
                auto ptr = row[idx2].get();
                if (ptr != nullptr)
                    UpdateFormulaOnInsert(ptr, allreadyChanged, before, count, false);
                row[idx1] = move(row[idx2]);
            } 
        }
    }
}



void Sheet::InvalidateCache(const CellHolder * const cellPtr) const { 
    cellPtr->Invalidate();
    if (cellPtr->usedBy.empty() == false)
        for (const auto& depCell: cellPtr->usedBy) 
            if (depCell->IsInvalid() == false)
                InvalidateCache(depCell); 
}



void Sheet::DeleteRows(int first, int count) {
    unordered_set<CellHolder*> allreadyChanged;
    for (int i = first; i < (rowsCount - count); ++i) { 
        if (cells[i+count].empty() == false)
            for (size_t j = 0; j < cells[i+count].size(); ++ j) {
                auto ptr = cells[i+count][j].get();
                if (ptr != nullptr) {
                    UpdateFormulaOnDelete(ptr, allreadyChanged, first, count, true);
                }
            }
             for (size_t j = 0; j < cells[i].size(); ++ j) {
                    CellHolder* cellPtr = cells[i][j].get();
                    if (cellPtr) {
                        UpdateFormulaOnDelete(cellPtr, allreadyChanged, first, count, true);
                        InvalidateCache(cellPtr);
                        ClearGraph(cellPtr);
                        cells.at(i).at(j) = nullptr;
                    }
                }
        cells[i] = move(cells[i + count]);
    }
    rowsCount -= count;
    cells.resize(rowsCount); 
    if (rowsCount < 0)
        rowsCount = 0;
    if (colsCount == 1 && rowsCount == 0) 
        colsCount = 0;
}



void Sheet::DeleteCols(int first, int count) { 
    unordered_set<CellHolder*> allreadyChanged;
    for (size_t i = 0; i < static_cast<size_t>(rowsCount); ++i) {
        if (cells[i].empty() == false) {
            auto& row = cells[i];     

            for (int j = first; j < first + count; ++j) {
                size_t idx = static_cast<size_t> (j);
                CellHolder* cellPtr = row.at(idx).get();
                if (cellPtr) {
                    UpdateFormulaOnDelete(cellPtr, allreadyChanged, first, count, false);
                    InvalidateCache(cellPtr);
                    ClearGraph(cellPtr);
                    row.at(idx) = nullptr;
                }
            }
            for (int j = first; 
                j < colsCount - count; ++j) { 
                size_t idx1 = static_cast<size_t> (j);
                size_t idx2 = static_cast<size_t> (j + count);
                if (idx2 == row.size())
                    break;
                auto ptr = row.at(idx2).get();
                if (ptr != nullptr) {
                    UpdateFormulaOnDelete(ptr, allreadyChanged, first, count, false);
                }
                row.at(idx1) = move(row.at(idx2));
            }  
            row.resize(colsCount - count);
        }
    }
    colsCount -= count;
    if (colsCount < 0)
        colsCount = 0;
    if (colsCount == 0 && rowsCount == 1) 
        rowsCount = 0;
}


void Sheet::UpdateFormulaOnDelete(CellHolder* cellPtr,  unordered_set<CellHolder*>& allreadyChanged,
     int first, int count, bool row) const {
    if (cellPtr == nullptr)
        return;
    if (cellPtr->usedBy.empty() == false) { 
        for (const auto refPtr: cellPtr->usedBy) {
            if (allreadyChanged.count(refPtr))
                continue;
            IFormula::HandlingResult hr;
            if (row)
                hr = refPtr->HandleDeletedRows(first, count);
            else
                hr = refPtr->HandleDeletedCols(first, count); 
            allreadyChanged.insert(refPtr);
            if (hr == IFormula::HandlingResult::NothingChanged)
                continue;
            if (hr == IFormula::HandlingResult::ReferencesChanged) 
                InvalidateCache(refPtr);
        }
    }
}


Size Sheet::GetPrintableSize() const  {
    return {rowsCount, colsCount};
}


void Sheet::PrintValues(ostream& output) const {
    for (int i = 0; i < rowsCount; ++i) {
        for (int j = 0; j < colsCount; ++j) {
            Position pos {i, j};
            if (CellExists(pos)) { 
                auto cellValue = GetCellPtr(pos)->GetValue(); 
                if (holds_alternative<string>(cellValue))
                    output << get<string>(cellValue);
                if (holds_alternative<double>(cellValue))
                    output << get<double>(cellValue);
            }
            if (j != colsCount - 1)
                output << "\t";
        }
        output << "\n";
    }
}


void Sheet::PrintTexts(ostream& output) const  {
    for (int i = 0; i < rowsCount; ++i) {
        for (int j = 0; j < colsCount; ++j) {
            Position pos {i, j};
            if (CellExists(pos)) 
                output << GetCellPtr(pos)->GetText(); 
            if (j != colsCount - 1)
                output << "\t";
        }
        output << "\n";
    }
}



unique_ptr<ISheet> CreateSheet() {
    return make_unique<Sheet>();
}

