#include <algorithm>
#include <functional>
#include <iostream>
#include <sstream>

#include "../inc/sheet.h"
#include "../inc/common.h"

using namespace std::literals;

struct ValueToStringVisitor {
    std::string operator() (std::string str) const {
        return str;
    }
    std::string operator() (double d) const {
        std::ostringstream oss;
        oss << d;
        return oss.str();
    }
    std::string operator() (FormulaError fe) const {
        std::ostringstream oss;
        oss << fe;
        return oss.str();
    }
};

Sheet::~Sheet() {
    cells_.clear();
}

void Sheet::SetCell(Position pos, std::string text) {
    IsValidPosition(pos, "SetCell"s);

    auto tmp_cell = std::make_unique<Cell>(*this);
    tmp_cell->Set(text);

    if (CheckForCircularDependencies(tmp_cell.get(), pos)) {
        throw CircularDependencyException("Error! Circular dependency"s);
    }

    cells_[pos] = std::move(tmp_cell);
}

const CellInterface* Sheet::GetCell(Position pos) const {
    IsValidPosition(pos, "GetCell"s);

    auto iter = cells_.find(pos);
    if (iter != cells_.end()) {
        return cells_.at(pos).get();
    }
    return nullptr;
}
CellInterface* Sheet::GetCell(Position pos) {
    IsValidPosition(pos, "GetCell"s);

    auto iter = cells_.find(pos);
    if(iter != cells_.end()) {
        return cells_.at(pos).get();
    }
    return nullptr;
}

void Sheet::ClearCell(Position pos) {
    IsValidPosition(pos, "ClearCell"s);

    cells_.erase(pos);
}

Size Sheet::GetPrintableSize() const {
    if (cells_.empty()) {
        return {0, 0};
    }
    auto left_right = GetLeftRightCorners();
    return {left_right.second.row - left_right.first.row + 1,
            left_right.second.col - left_right.first.col + 1};
}

void Sheet::PrintValues(std::ostream& output) const {
    if (cells_.empty()) {
        return;
    }
    Position left_corner{};
    Position right_corner{};
    std::tie(left_corner, right_corner) = GetLeftRightCorners();

    for (int i = left_corner.row; i <= right_corner.row; ++i) {
        for (int j = left_corner.col; j <= right_corner.col; ++j) {
            auto current_cell_ptr = GetCell({i, j});
            if (current_cell_ptr) {
                output << std::visit(ValueToStringVisitor(), current_cell_ptr->GetValue());
            }
            if (j < right_corner.col) {
                output << '\t';
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    if (cells_.empty()) {
        return;
    }
    Position left_corner{};
    Position right_corner{};
    std::tie(left_corner, right_corner) = GetLeftRightCorners();

    for (int i = left_corner.row; i <= right_corner.row; ++i) {
        for (int j = left_corner.col; j <= right_corner.col; ++j) {
            auto current_cell_ptr = GetCell({i, j});
            if (current_cell_ptr) {
                output << current_cell_ptr->GetText();
            }
            if (j < right_corner.col) {
                output << '\t';
            }
        }
        output << '\n';
    }
}

size_t Sheet::position_hash::operator() (const Position& p) const {
    return std::hash<int>()(p.row) ^ std::hash<int>()(p.col);
}

std::pair<Position, Position> Sheet::GetLeftRightCorners() const {
    if (cells_.empty()) {
        return {{0,0}, {0, 0}};
    }
    else if (cells_.size() == 1u) {
        auto pos = cells_.begin()->first;
        return {{0, 0}, pos};
    }
    Position right = Position::NONE;

    for (const auto& [key, _] : cells_) {
        right.col = right.col < key.col ? key.col : right.col;
        right.row = right.row < key.row ? key.row : right.row;
    }
    return {{0,0}, right};
}

bool Sheet::CheckForCircularDependencies(const CellInterface* cell, Position head) const {
    if (!cell) {
        return false;
    }
    bool res = false;
    for (const auto& next_cell_pos : cell->GetReferencedCells()) {
        if (next_cell_pos == head) {
            return true;
        }
        res = res || CheckForCircularDependencies(GetCell(next_cell_pos), head);
    }
    return res;
}

void Sheet::IsValidPosition(const Position pos, const std::string& function_name) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Error! Invalid position passed to "s + function_name + "!!!"s);
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
