#ifndef TABLE_FORMULA_IMPL
#define TABLE_FORMULA_IMPL

#include "formula.h"

#include "listener.h"


class Formula : public IFormula {
public:

    virtual ~Formula() = default;
    Formula(Listener *l);

    virtual IFormula::Value Evaluate(const ISheet& sheet) const override;
    virtual std::string GetExpression() const override;
    virtual std::vector<Position> GetReferencedCells() const override;

    virtual IFormula::HandlingResult HandleInsertedRows(int before, int count = 1) override;
    virtual IFormula::HandlingResult HandleInsertedCols(int before, int count = 1) override;
    virtual IFormula::HandlingResult HandleDeletedRows(int first, int count = 1) override;
    virtual IFormula::HandlingResult HandleDeletedCols(int first, int count = 1) override;


private:

    void UpdateRefs();

    std::vector<Position> refCells;
    std::unique_ptr<Statement> rootStatement;
    std::vector<CellStatement*> referencedPtrs; 
};

#endif