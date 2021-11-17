#ifndef TABLE_CELL
#define TABLE_CELL

#include "common.h"
#include "formula.h" // позже разделить файлы

#include <iostream>
#include <limits>


class Sheet; 
class CellHolder;


class InnerCell : public ICell {
    public:
    virtual bool IsInvalid() const = 0;
    virtual void Invalidate() const = 0;
    virtual std::string LastCallParams() const = 0;

    virtual IFormula::HandlingResult HandleInsertedRows(int before, int count = 1) = 0;
    virtual IFormula::HandlingResult HandleInsertedCols(int before, int count = 1) = 0;
    virtual IFormula::HandlingResult HandleDeletedRows(int first, int count = 1) = 0;
    virtual IFormula::HandlingResult HandleDeletedCols(int first, int count = 1) = 0;
};




class FormulaCell : public InnerCell { 
public:

    FormulaCell(Sheet& sheet, std::unique_ptr<IFormula> formula, IFormula::Value cellValue);

    virtual ~FormulaCell() = default;
    virtual ICell::Value GetValue() const override ;
    virtual std::string GetText() const override ;
    virtual std::vector<Position> GetReferencedCells() const override ;

    using FormulaPtr = std::unique_ptr<IFormula>;

    void Invalidate() const override;
    bool IsInvalid() const override;
    void Update(const CellHolder* parent) const;

    std::string LastCallParams() const override {
        return GetText(); 
    }

    IFormula::HandlingResult HandleInsertedRows(int before, int count = 1);
    IFormula::HandlingResult HandleInsertedCols(int before, int count = 1);
    IFormula::HandlingResult HandleDeletedRows(int first, int count = 1);
    IFormula::HandlingResult HandleDeletedCols(int first, int count = 1);

    bool holdsError() const;
    
private:
    Sheet& sheet;
    FormulaPtr formula;
    mutable IFormula::Value cellValue = 0.0;
    mutable bool cacheInvalid;
};




class LiteralCell : public InnerCell {
public:

    LiteralCell(std::string literal);

    void Invalidate() const override {}
    bool IsInvalid() const override {
        return false;
    }
    std::string LastCallParams() const override {
        return textValue; 
    }

    virtual ~LiteralCell() = default;
    virtual ICell::Value GetValue() const override;
    virtual std::string GetText() const override {
        return textValue;
    }
    virtual std::vector<Position> GetReferencedCells() const override {
        return {};
    }

    virtual IFormula::HandlingResult HandleInsertedRows(int before, int count = 1) {
        return IFormula::HandlingResult::NothingChanged;
    }
    virtual IFormula::HandlingResult HandleInsertedCols(int before, int count = 1){
        return IFormula::HandlingResult::NothingChanged;
    }
    virtual IFormula::HandlingResult HandleDeletedRows(int first, int count = 1){
        return IFormula::HandlingResult::NothingChanged;
    }
    virtual IFormula::HandlingResult HandleDeletedCols(int first, int count = 1) {
        return IFormula::HandlingResult::NothingChanged;
    }

private:
    std::string textValue;
    mutable double cellValue = std::numeric_limits<double>::max();
};



class ErrorCell  : public InnerCell {
public:
    ErrorCell(std::string text, FormulaError::Category errorCategory)
    :textValue(text), cellValue(errorCategory) {
    }
    virtual ICell::Value GetValue() const override {
        return cellValue;
    }
    virtual std::string GetText() const override {
        return textValue;
    }
    virtual std::vector<Position> GetReferencedCells() const override {
        return {};
    }

    void Invalidate() const override {}
    bool IsInvalid() const override {
        return false;
    }
    std::string LastCallParams() const override {
        return textValue; 
    }
    virtual IFormula::HandlingResult HandleInsertedRows(int before, int count = 1) {
        return IFormula::HandlingResult::NothingChanged;
    }
    virtual IFormula::HandlingResult HandleInsertedCols(int before, int count = 1){
        return IFormula::HandlingResult::NothingChanged;
    }
    virtual IFormula::HandlingResult HandleDeletedRows(int first, int count = 1){
        return IFormula::HandlingResult::NothingChanged;
    }
    virtual IFormula::HandlingResult HandleDeletedCols(int first, int count = 1) {
        return IFormula::HandlingResult::NothingChanged;
    }

private:
    std::string textValue;
    mutable FormulaError cellValue;
};



class CellHolder : public ICell {

private:
    std::unique_ptr<InnerCell> cell;

public:

    static int totalObjects;

    CellHolder() {
        ++totalObjects;
    }
    ~CellHolder() {
        --totalObjects;
    }

    static int getTotalObject() {
        return totalObjects;
    }
        
    void reset(Sheet& sheet);
    void reset(Sheet& sheet, std::string literal);
    void reset(Sheet& sheet, std::unique_ptr<IFormula> formula, IFormula::Value cellValue);
    void reset(Sheet& sheet, std::string text, FormulaError::Category errorCategory);

    virtual ICell::Value GetValue() const override ;
    virtual std::string GetText() const override;
    virtual std::vector<Position> GetReferencedCells() const override;

    IFormula::HandlingResult HandleInsertedRows(int before, int count = 1);
    IFormula::HandlingResult HandleInsertedCols(int before, int count = 1);
    IFormula::HandlingResult HandleDeletedRows(int first, int count = 1);
    IFormula::HandlingResult HandleDeletedCols(int first, int count = 1);


    bool IsInvalid() const; 
    void Update() const;
    void Invalidate() const;
    bool DepCheckFlag() const;
    bool HasFormula() const;
    void checkType() const;

    std::string GetLastCall() const;

    std::vector<CellHolder *> usedBy; 
};


#endif