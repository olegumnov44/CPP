#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <iterator>
#include <map>
#include <set>
#include <utility>


#include "remove_duplicates.h"
#include "log_duration.h"


void RemoveDuplicates(SearchServer& search_server)
{
    //LOG_DURATION("RemoveDupl - O(wN(log⁡N+log⁡W))"s);
    std::vector<int> v_dubl_ids;
    std::set<std::set<std::string>> words_to_id_;

    for (const int document_id : search_server)
    {
        std::set<std::string> words_;
        for (const auto& word_freq : search_server.GetWordFrequencies(document_id))
            words_.insert(word_freq.first);

        if (!words_to_id_.emplace(words_).second)
            v_dubl_ids.push_back(document_id);
    }

    //LOG_DURATION("RemoveDoc - n * O(w(log⁡N+log⁡W))"s);
    sort(v_dubl_ids.begin(), v_dubl_ids.end());
    for (const auto id : v_dubl_ids)
    {
        std::cout << "Found duplicate document id "s << id << std::endl;
        search_server.RemoveDocument(id);
    }
}
