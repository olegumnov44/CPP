#pragma once
#include <functional>
#include <unordered_set>

#include "common.h"
#include "formula.h"

class Sheet;

class Cell : public CellInterface {
private:
    struct CellCache {
        Value val_;
        bool modification_flag_ = true;

        operator bool() const;
        operator Value() const;
    };

    bool IsModified() const;
    void SetCache(Value&& val) const;

public:
    Cell(Sheet& sheet);
    ~Cell();

    void Set(const std::string& text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override;

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    std::unique_ptr<Impl> impl_;
    Sheet& sheet_;

    mutable CellCache cache_;
};
