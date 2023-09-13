#include <iostream>
#include <string>
#include <vector>
#include <execution>

#include "string_processing.h"

using namespace std;


std::vector<std::string_view> SplitIntoWords(std::string_view text) {
    std::vector<std::string_view> words;
    size_t first = text.find_first_not_of(' ', 0);

    while(first < text.size()) {
        size_t second = text.find_first_of(' ', first);

        if (first != second) {
            words.emplace_back(text.substr(first, second - first));
        }
        first = text.find_first_not_of(' ', second);
    }
    return words;
}
