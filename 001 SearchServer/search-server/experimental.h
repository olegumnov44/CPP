#pragma once

#include "search_server.h"

const short MAX_LEVEL = 8;

//Медленнее std::sort&&std::stable_sort
template <typename ExecutionPolicy, typename RandomIt, typename Function>
void MergeSort(ExecutionPolicy&& policy, RandomIt range_begin, RandomIt range_end, ushort level, Function function) {


    if constexpr (std::is_same_v<std::decay_t<ExecutionPolicy>, std::execution::sequenced_policy>)
    {
        std::sort(range_begin, range_end, function);
        return;
    }
    else
    {
        const int range_length = range_end - range_begin;
        if (range_length < 2) {
            return;
        }
        std::vector elements(range_begin, range_end);
        const auto mid = elements.begin() + range_length / 2;
        auto left = [&]() { MergeSort(policy, elements.begin(), mid, level + 1, function); };
        auto right = [&]() { MergeSort(policy, mid, elements.end(), level + 1, function); };

        if (level <= MAX_LEVEL) {
            auto left_future = std::async(left);
            right();
            left_future.get();
        }
        else {
            left();
            right();
        }
        std::merge(std::execution::par, elements.begin(), mid, mid, elements.end(), range_begin, function);
    }
}

template <typename ExecutionPolicy, typename Iterator, typename Function>
void ForEach(ExecutionPolicy&& policy, Iterator first, Iterator last, const short PART_COUNT, Function function) {

    if constexpr (std::is_same_v<std::decay_t<ExecutionPolicy>, std::execution::sequenced_policy>)
    {
        std::for_each(first, last, function);
    }
    else
    {
        const auto part_length = std::distance(first, last) / PART_COUNT;
        auto part_begin = first;
        auto part_end = std::next(part_begin, part_length);

        std::vector<std::future<void>> futures;
        for (int i = 0;
             i < PART_COUNT;
             ++i,
             part_begin = part_end,
             part_end = (i == PART_COUNT - 1
                             ? last
                             : std::next(part_begin, part_length))
             )
        {
            futures.push_back(std::async([function, part_begin, part_end] {
                for_each(part_begin, part_end, function);
            }));
        }
    }
}


template <typename ExecutionPolicy, typename Iterator, typename Function>
bool AnyOf(ExecutionPolicy&& policy, Iterator first, Iterator last, const short PART_COUNT, Function function) {
    if constexpr (std::is_same_v<std::decay_t<ExecutionPolicy>, std::execution::sequenced_policy>)
    {
        return std::any_of(policy, first, last, function);
    }
    else
    {
        //static constexpr int PART_COUNT = 4;
        const auto part_length = std::distance(first, last) / PART_COUNT;
        auto part_begin = first;
        auto part_end = std::next(part_begin, part_length);
        std::vector<std::future<bool>> futures;
        bool result = false;
        for (int i = 0;
             i < PART_COUNT;
             ++i,
             part_begin = part_end,
             part_end = (i == PART_COUNT - 1
                             ? last
                             : std::next(part_begin, part_length))
             )
        {
            futures.push_back(std::async([function, part_begin, part_end] {
               return (std::any_of(part_begin, part_end, function));
            }));
        }
        int size = futures.size();
        for (int i = 0; i < size; ++i) {
            if (futures[i].get()) {
                result = true;
                break;
            }
        }
        return result;
    }
}

template <typename ExecutionPolicy, typename Iterator, typename Function>
bool NoneOf(ExecutionPolicy&& policy, Iterator first, Iterator last, const short PART_COUNT, Function function) {
    if constexpr (std::is_same_v<std::decay_t<ExecutionPolicy>, std::execution::sequenced_policy>)
    {
        return std::none_of(first, last, function);
    }
    else
    {
        const auto part_length = std::distance(first, last) / PART_COUNT;
        auto part_begin = first;
        auto part_end = std::next(part_begin, part_length);
        std::vector<std::future<bool>> futures;
        bool result = false;
        for (int i = 0;
             i < PART_COUNT;
             ++i,
             part_begin = part_end,
             part_end = (i == PART_COUNT - 1
                             ? last
                             : std::next(part_begin, part_length))
             )
        {
            futures.push_back(std::async([function, part_begin, part_end] {
               return (std::none_of(part_begin, part_end, function));
            }));
        }
        int size = futures.size();
        for (int i = 0; i < size; ++i) {
            if (futures[i].get()) {
                result = true;
                break;
            }
        }
        return result;
    }
}
