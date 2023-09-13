#include <cassert>
#include <iostream>
#include <string>
#include <optional>

#include "../inc/cell.h"
#include "../inc/sheet.h"

using namespace std::literals;

// --------------------...Impl.BEGIN---------------------
class Cell::Impl {
public:
    virtual ~Impl() = default;

    virtual Cell::Value GetValue(SheetInterface& sheet) const = 0;
    virtual std::string GetText() const = 0;
    virtual std::vector<Position> GetReferencedCells() const = 0;
};

class Cell::EmptyImpl final : public Impl {
public:
    EmptyImpl() = default;

    Cell::Value GetValue(SheetInterface& /*sheet*/) const override {
        return ""s;
    }

    std::string GetText() const override {
        return ""s;
    }

    std::vector<Position> GetReferencedCells() const override {
        return {};
    }
};

class Cell::TextImpl final : public Impl {
public:
    explicit TextImpl(std::string text)
        : data_(std::move(text))
    {}

    Cell::Value GetValue(SheetInterface& /*sheet*/) const override {
        return data_;
    }

    std::string GetText() const override {
        return data_;
    }

    std::vector<Position> GetReferencedCells() const override {
        return {};
    }

private:
    const std::string data_;
};

class Cell::FormulaImpl final : public Cell::Impl {
private:
    struct FormulaVisitor {
        Cell::Value operator() (double d) {
            return Cell::Value{d};
        }

        Cell::Value operator() (FormulaError fe) {
            return Cell::Value{fe};
        }
    };
public:
    explicit FormulaImpl(std::string formula)
        : data_(ParseFormula(std::move(formula)))
    {}

    Cell::Value GetValue(SheetInterface& sheet) const override {
        return std::visit(FormulaVisitor(), data_->Evaluate(sheet));
    }

    std::string GetText() const override {
        return FORMULA_SIGN + data_->GetExpression();
    }

    std::vector<Position> GetReferencedCells() const override {
        return data_->GetReferencedCells();
    }
private:
    std::unique_ptr<FormulaInterface> data_;
};
// --------------------...Impl.END---------------------


Cell::Cell(Sheet& sheet)
    : impl_(std::make_unique<EmptyImpl>(EmptyImpl{}))
    , sheet_(sheet) {}

Cell::~Cell() = default;

bool Cell::IsModified() const {
    return cache_;
}

void Cell::SetCache(Value&& val) const {
    cache_.val_ = std::forward<Value>(val);
    cache_.modification_flag_ = false;
}

void Cell::Set(const std::string& text) {
    if (text.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
    }
    else if (text.front() == FORMULA_SIGN && text.size() > 1u) {
        try {
            impl_ = std::make_unique<FormulaImpl>(text.substr(1u));
            for(const auto& ref_cell : impl_->GetReferencedCells()) {
                auto cell_ptr = sheet_.GetCell(ref_cell);
                if (!cell_ptr) {
                    sheet_.SetCell(ref_cell, ""s);
                }
            }
        }
        catch(...) {
            throw FormulaException{"Unable to parse: "s.append(text)};
        }
    }
    else {
        impl_ = std::make_unique<TextImpl>(text);
    }
    cache_.modification_flag_ = true;
}

void Cell::Clear() {
    impl_.reset(nullptr);
    cache_.modification_flag_ = true;
}

struct ValueVisitor {
    Cell::Value operator() (std::string str) const {
        return str.front() == '\'' ? str.substr(1u) : str;
    }

    Cell::Value operator() (double d) const {
        return d;
    }

    Cell::Value operator() (FormulaError fe) const {
        return fe;
    }
};

Cell::Value Cell::GetValue() const {
    const auto& dependency = GetReferencedCells();
    if (!cache_ || std::any_of(dependency.begin(), dependency.end(),
        [this] (const auto& pos) {
                                     const Cell* cell_ptr_ = reinterpret_cast<const Cell*>(sheet_.GetCell(pos));
                                     return cell_ptr_->IsModified();
                                 }))
    {
        auto value = impl_ ? std::visit(ValueVisitor(), impl_->GetValue(sheet_)) : Cell::Value{};
        SetCache(std::move(value));
    }

    return cache_;
}

std::string Cell::GetText() const {
    if (impl_) {
        return impl_->GetText();
    }

    return {};
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

Cell::CellCache::operator bool() const {
    return !modification_flag_;
}

Cell::CellCache::operator Value() const {
    return val_;
}
