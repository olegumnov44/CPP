#include <algorithm>
#include <cmath>
#include <deque>

#include "search_server.h"
#include "log_duration.h"

using namespace std;
using DocumentsByStatus = std::tuple<std::vector<std::string_view>, DocumentStatus>;

SearchServer::SearchServer(const std::string_view stopwords_text)
    : SearchServer(SplitIntoWords(stopwords_text))
{}

SearchServer::SearchServer(const std::string& stopwords_text)
    : SearchServer(SplitIntoWords(std::string_view(stopwords_text)))
{}

void SearchServer::AddDocument(int document_id, const std::string_view document,
                               DocumentStatus status, const std::vector<int>& ratings) {
    if (document_id < 0) {
        throw invalid_argument("Id less then null"s);
    }
    if (_id_docdata_.count(document_id)) {
        throw invalid_argument("This id exist already"s);
    }
    std::vector<std::string_view> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    std::vector<std::string> words_doc;
    for (const std::string_view word : words) {
        words_doc.push_back(std::string(word));
    }
    _id_docdata_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status, words_doc});
    _id_.insert(document_id);
    for (const auto& word : _id_docdata_.at(document_id)._words_doc_) {
            _word_to__id_freq_[word][document_id] += inv_word_count;
            _id_to__word_freq_[document_id][word] += inv_word_count;
    }
}

void SearchServer::RemoveDocument(int document_id) {
    RemoveDocument(std::execution::seq, document_id);
}

std::vector<Document>
SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(
                            raw_query,
                            [status](int document_id, DocumentStatus document_status, int rating) {
                                return document_status == status;
                            });
}

std::vector<Document>
SearchServer::FindTopDocuments(std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return _id_docdata_.size();
}

int SearchServer::GetWordCount(const std::string_view word) const {
    return _word_to__id_freq_.count(word);
}

int SearchServer::GetDocRating(const int document_id) const {
    return _id_docdata_.at(document_id).rating;
}

std::set<int>::const_iterator SearchServer::begin() const {
    return _id_.cbegin();
}

std::set<int>::const_iterator SearchServer::end() const {
    return _id_.cend();
}

DocumentsByStatus
SearchServer::MatchDocument(std::string_view raw_query, int document_id) const {
    return MatchDocument(std::execution::seq, raw_query, document_id);
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    return _id_to__word_freq_.at(document_id);
}

//private

bool SearchServer::IsStopWord(const std::string_view word) const {
    return _stopwords_.count(word) > 0;
}

// A valid word must not contain special characters
bool SearchServer::IsValidWord(const std::string_view word) {
    return none_of(word.begin(), word.end(),
        [](char c) { return c >= '\0' && c < ' '; }
    );
}

std::vector<std::string_view>
SearchServer::SplitIntoWordsNoStop(const std::string_view text) const {
    std::vector<std::string_view> words;
    for (std::string_view word : SplitIntoWords(text)) {
        if (!SearchServer::IsValidWord(word)) {
            throw std::invalid_argument("Word "s + std::string(word) + " is invalid"s);
        }
        if (!SearchServer::IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    int rating_sum = 0;
    rating_sum = std::reduce(std::execution::par, ratings.begin(), ratings.end());

    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord
SearchServer::ParseQueryWord(std::string_view text) const {

    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    const string& word = std::string(text);
    if (text.empty() || text[0] == '-' || !SearchServer::IsValidWord(word)) {
        throw std::invalid_argument("Query word "s + word + " is invalid");
    }

    return {text, is_minus, SearchServer::IsStopWord(word)};
}

SearchServer::Query
SearchServer::ParseQuery(const std::string_view text) const {

    const std::vector<std::string_view> words = SplitIntoWords(text);
    if (words.empty()) {
        return {};
    }
    SearchServer::Query result;

    for_each(
             words.begin(), words.end(),
             [this, &result](std::string_view word) {
                 const QueryWord query_word = ParseQueryWord(word);
                 if (!query_word.is_stop) {
                     (!query_word.is_minus) ? result.plus_words.push_back(query_word.data)
                                            : result.minus_words.push_back(query_word.data);
                 }
             });

    sort(result.minus_words.begin(), result.minus_words.end());
    const auto it_minus = unique(result.minus_words.begin(), result.minus_words.end());
    result.minus_words.erase(it_minus, result.minus_words.end());

    sort(result.plus_words.begin(), result.plus_words.end());
    const auto it_plus = unique(result.plus_words.begin(), result.plus_words.end());
    result.plus_words.erase(it_plus, result.plus_words.end());

    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    return log(SearchServer::GetDocumentCount() * 1.0 / _word_to__id_freq_.at(std::string(word)).size());
}
// ----END OF CLASS-----


void AddDocument(SearchServer& search_server, int document_id, const std::string_view document,
                 DocumentStatus status, const std::vector<int>& ratings)
{
    try { search_server.AddDocument(document_id, document, status, ratings); }
    catch (const std::exception& e) {
        std::cout << "Error in adding document "s << document_id << ": "s << e.what() << std::endl;
    }
}

template <typename ExecutionPolicy>
void FindTopDocuments(ExecutionPolicy&& policy, const SearchServer& search_server, const std::string_view raw_query) {
    std::cout << "Results for request: "s << std::string(raw_query) << std::endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(policy, raw_query)) {
            PrintDocument(document);
        }
    }
    catch (const std::exception& e) {
        std::cout << "Error is seaching: "s << e.what() << std::endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const std::string_view query) {
    try {
        std::cout << "Matching for request: "s << query << std::endl;
        for (auto index = search_server.begin(); index != search_server.end(); ++index) {
            const int document_id = *index;
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    }
    catch (const std::exception& e) {
        std::cout << "Error in matchig request "s << query << ": "s << e.what() << std::endl;
    }
}
