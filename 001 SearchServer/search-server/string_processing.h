#pragma once

#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <execution>

using namespace std::string_literals;


std::vector<std::string_view> SplitIntoWords(std::string_view text);

template <typename StringContainer>
std::vector<std::string_view> MakeUniqueNonEmptyStrings(const StringContainer& strings)
{
    std::vector<std::string_view> non_empty_strings;
    for (std::string_view word_sv : strings) {
        if (!word_sv.empty()) {
            non_empty_strings.push_back(word_sv);     // Extract non-empty stop words
        }
    }
    return non_empty_strings;
}
