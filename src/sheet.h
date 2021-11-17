#ifndef TABLE_SHEET
#define TABLE_SHEET

#include "common.h"
#include "cell.h"

#include <unordered_map>
#include <unordered_set>
#include <array>
#include <vector>
#include <set>
#include <map>
#include <optional>


class Sheet : public ISheet
{
public:
    Sheet();
    virtual ~Sheet() = default;

    virtual void SetCell(Position pos, std::string text) override;

    virtual const ICell *GetCell(Position pos) const override;
    virtual ICell *GetCell(Position pos) override;

    virtual void ClearCell(Position pos) override;
    virtual void InsertRows(int before, int count = 1) override;
    virtual void InsertCols(int before, int count = 1) override;
    virtual void DeleteRows(int first, int count = 1) override;
    virtual void DeleteCols(int first, int count = 1) override;

    virtual Size GetPrintableSize() const override;
    virtual void PrintValues(std::ostream &output) const override;
    virtual void PrintTexts(std::ostream &output) const override;

    void InvalidateCache(const CellHolder * const cellPtr) const;
    void UpdateDependent(const CellHolder * const cellPtr) const;


private:
    using CellPtr = std::unique_ptr<CellHolder>;
    using TableRow = std::vector<CellPtr>;
    std::vector<TableRow> cells;

    int rowsCount = 0;
    int colsCount = 0;

    bool CellShouldExist(const Position &pos) const;
    CellPtr &CreateCell(const Position &pos);

    void UpdateFormulaOnDelete(CellHolder *cellPtr, std::unordered_set<CellHolder*>& allreadyChanged,
        int first, int count, bool row) const;
    void UpdateFormulaOnInsert(CellHolder *cellPtr, std::unordered_set<CellHolder*>& allreadyChanged,
         int first, int count, bool row) const;

    void UpdateChache(const CellHolder * const cellPtr) const;

    static bool textHasFormula(const std::string& text);
    void HandleFormulaCreation(Position pos, std::string text, bool cellExisted);

    bool CellExists(const Position &pos) const;
    CellHolder *GetCellPtr(const Position &pos) const;

    bool CheckDependency(Position pos, const std::vector<Position> &refs) const;
    void ClearUsedGraph(CellHolder* cell, const std::vector<Position>& refs);
    void ClearGraph(CellHolder *cellPtr);
};

#endif