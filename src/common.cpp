#include "common.h"
#include <iostream>
#include <string>
#include <charconv>

using namespace std;



bool Position::operator==(const Position& rhs) const {
    return rhs.col == col && rhs.row == row;
}


bool Position::operator<(const Position& rhs) const {
    if (rhs.col == col)
        return row < rhs.row;
    if (rhs.row == row)
        return col < rhs.col;
    return (col + row) < (rhs.col + row); 
}


bool Position::IsValid() const {
    return (col >= 0 && row >= 0) && (col < kMaxCols  && row < kMaxRows); 
}


std::string Position::ToString() const {

    if (col < 0 || row < 0)
        return "";

    string pos;
    if (col < 26) {
        pos = {static_cast<char>('A' + col)};
    }
    else if (col < 702) {
        int first = (col - 26) / 26;
        int second = (col - 26) % 26;
        pos = {static_cast<char>('A' + first), static_cast<char>('A' + second)};
    }
    else if (col < 16384) {
        int first = (col - 702) / 676;
        int temp = (col - 702) % 676;
        int second = temp / 26;
        int third = temp % 26;
        pos = {static_cast<char>('A' + first), static_cast<char>('A' + second), 
                static_cast<char>('A' + third)};
    }
    else {
        return "!#REF";
    }

    return pos + to_string(row + 1);
}


Position Position::FromString(std::string_view str) {

    auto alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"; 
    std::size_t found = str.find_first_not_of(alphabet);

    if (found == 0) 
        return {-2, -2};
    if (found == string::npos)
        return {-2, -2};

    string_view letters = str.substr(0, found);
    string_view digits = str.substr(found); 

    if (letters.size() == 0)
        return {-2, -2};
    if (digits.size() == 0)
        return {-2, -2};
    if (digits.find_first_of(alphabet) != string::npos)
        return {-2, -2};

    int col=0, row = 0;

    for (size_t i = 0; i < letters.size(); ++i) {
        col *= 26;
        col += letters[i] - 'A'; 
    } 

    if (letters.size() == 2)
        col += 26;
    else if (letters.size() == 3)
        col += 702;
    else if (letters.size() != 1)
        return {-2, -2};

    from_chars(digits.data(), digits.data() + digits.size(), row); //auto error = 
    --row;

    return {row, col}; 
}



bool Size::operator==(const Size& rhs) const {
    return rows == rhs.rows && cols == rhs.cols;
}



FormulaError::FormulaError(FormulaError::Category category) 
    : category_(category) {}


bool FormulaError::operator==(FormulaError rhs) const {
    return true;
}


FormulaError::Category FormulaError::GetCategory() const {
    return category_;
}

std::string_view FormulaError::ToString() const {
    if (category_ == FormulaError::Category::Ref)
        return "#!REF";
    if (category_ == FormulaError::Category::Value)
        return "#VALUE!";
    if (category_ == FormulaError::Category::Div0)
        return "#DIV/0!";
    return "NOTSET";
}


std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    output << fe.ToString();
    return output;
}

