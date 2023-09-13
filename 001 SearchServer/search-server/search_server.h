#pragma once

#include <algorithm>
#include <cmath>
#include <deque>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <tuple>
#include <execution>
#include <numeric>
#include <list>
#include <iostream>
#include <iterator>
#include <functional>
#include <mutex>
#include <future>

#include "tbb/parallel_for.h"
#include "tbb/parallel_for_each.h"
#include "tbb/blocked_range.h"

#include "document.h"
#include "string_processing.h"
#include "log_duration.h"
#include "concurrent_map.h"

using namespace std::string_literals;
using DocumentsByStatus = std::tuple<std::vector<std::string_view>, DocumentStatus>;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double MIN_ACCURACY = 1e-6;

class SearchServer
{
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    explicit SearchServer(std::string_view stopwords_text);
    explicit SearchServer(const std::string& text);

    void AddDocument(int document_id, std::string_view document,
                     DocumentStatus status, const std::vector<int>& ratings);

    template<typename ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy&& policy, int document_id);

    void RemoveDocument(int document_id);

    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document>
    FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query,
                     DocumentPredicate document_predicate) const;

    template <typename ExecutionPolicy>
    std::vector<Document>
    FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query,
                     DocumentStatus status) const;

    template <typename ExecutionPolicy>
    std::vector<Document>
    FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query) const;

    // Без распараллеливания
    template <typename DocumentPredicate>
    std::vector<Document>
    FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const;

    std::vector<Document>
    FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document>
    FindTopDocuments(std::string_view raw_query) const;

    int GetDocumentCount() const;

    int GetWordCount(const std::string_view word) const;

    int GetDocRating(const int document_id) const;

    std::set<int>::const_iterator begin() const;
    std::set<int>::const_iterator end() const;

    template<typename ExecutionPolicy>
    DocumentsByStatus
    MatchDocument(ExecutionPolicy&& policy, std::string_view raw_query, int document_id) const;

    DocumentsByStatus
    MatchDocument(std::string_view raw_query, int document_id) const;

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

private:

    struct DocumentData {
        int rating;
        DocumentStatus status;
        std::vector<std::string> _words_doc_;
    };

    std::set<std::string, std::less<>> _stopwords_;
    std::map<std::string_view, std::map<int, double>> _word_to__id_freq_;
    std::map<int, std::map<std::string_view, double>> _id_to__word_freq_;
    std::map<int, DocumentData> _id_docdata_;
    std::set<int> _id_;
    // "_" перед именем и дополнительный между словами - признак контейнера (vector, set, map...)
    // "_" в конце имени - признак принадлежности к private области класса

    bool IsStopWord(std::string_view word) const;

    static bool IsValidWord(std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord
    {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string_view text) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(std::string_view text) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(std::string_view word) const;

    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document>
    FindAllDocuments(ExecutionPolicy&& policy, const Query& query,
                     DocumentPredicate document_predicate) const;

    template <typename ExecutionPolicy>
    std::vector<Document>
    FindAllDocuments(ExecutionPolicy&& policy, const Query& query) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
{
    if (any_of(
              stop_words.cbegin(), stop_words.cend(),
              [](const std::string_view word){ return !IsValidWord(word); })) {
        throw std::invalid_argument("Found a special symbol(s)"s);
    }

    for (const std::string_view word : stop_words) {       // Extract non-empty stop words
        if (!word.empty()) {
            _stopwords_.insert(std::string(word));
        }
    }
}

template<typename ExecutionPolicy>
DocumentsByStatus
SearchServer::MatchDocument(ExecutionPolicy&& policy, std::string_view raw_query, int document_id) const {

    if (_id_.count(document_id) == 0) {
        throw std::out_of_range("Id doesn't exist");
    }
    if (raw_query.empty()) {
        throw std::invalid_argument("The query is empty");
    }
    const std::map<std::string_view, double>* _word_freq = &GetWordFrequencies(document_id);
    if (_word_freq->empty()) {
        return {{}, _id_docdata_.at(document_id).status};
    }
    SearchServer::Query&& query = ParseQuery(raw_query);

    auto comparator = [&_word_freq](const std::string_view word) {
                          return _word_freq->count(word) > 0;
                      };
    if (std::any_of(query.minus_words.begin(), query.minus_words.end(), comparator)) {
        return {{}, _id_docdata_.at(document_id).status};
    }
    std::vector<std::string_view> matched_words(query.plus_words.size());
    std::atomic_uint index = 0;
    const auto it_end = _word_freq->end();

    std::for_each(policy,
                  query.plus_words.begin(), query.plus_words.end(),
                  [&_word_freq, &matched_words, &index, &it_end](const std::string_view word) {
                      if (_word_freq->find(word) != it_end) {
                          matched_words[index++] = word;
                      }
    });

    matched_words.resize(index);
    return {matched_words, _id_docdata_.at(document_id).status};
}

template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document>
SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query,
                               DocumentPredicate document_predicate) const {

    const SearchServer::Query query = ParseQuery(raw_query);
    std::vector<Document> matched_documents = FindAllDocuments(policy, query, document_predicate);
    sort(policy,
         matched_documents.begin(), matched_documents.end(),
         [](const Document& lhs, const Document& rhs) {
             return lhs.relevance > rhs.relevance
                 || (std::abs(lhs.relevance - rhs.relevance) < MIN_ACCURACY && lhs.rating > rhs.rating);
         });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <typename ExecutionPolicy>
std::vector<Document>
SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query,
                               DocumentStatus status) const {
    return FindTopDocuments(policy,
                            raw_query,
                            [status](int document_id, DocumentStatus document_status, int rating) {
                                return document_status == status;
                            });
}

template <typename ExecutionPolicy>
std::vector<Document>
SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query) const {
    const SearchServer::Query query = ParseQuery(raw_query);
    std::vector<Document> matched_documents = FindAllDocuments(policy, query);
    sort(policy,
         matched_documents.begin(), matched_documents.end(),
         [](const Document& lhs, const Document& rhs) {
             return lhs.relevance > rhs.relevance
                 || (std::abs(lhs.relevance - rhs.relevance) < MIN_ACCURACY && lhs.rating > rhs.rating);
         });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

// Без распараллеливания
template <typename DocumentPredicate>
std::vector<Document>
SearchServer::FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document>
SearchServer::FindAllDocuments(ExecutionPolicy&& policy, const Query& query,
                               DocumentPredicate document_predicate) const {

    ConcurrentMap<int, double> document_to_relevance(10007); //10007 - подобранное эффективное значение
    const auto it_end = _word_to__id_freq_.end();
    for_each(policy,
             query.plus_words.begin(), query.plus_words.end(),
             [this, &document_predicate, &document_to_relevance, &it_end](const auto word) {
                 if (_word_to__id_freq_.find(word) != it_end) {
                     //Заполнение ConcurrentMap данными по плюс-словам
                     const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                     for(const auto& id_freq : _word_to__id_freq_.at(word)) {
                         const int document_id = id_freq.first;
                         const double term_freq = id_freq.second;
                         const auto& doc_data = _id_docdata_.at(document_id);
                         if (document_predicate(document_id, doc_data.status, doc_data.rating)) {
                             document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                         }
                     }
                 }
    });
    // Удаление данных по минус-словам
    for_each(policy,
             query.minus_words.begin(), query.minus_words.end(),
             [this, &document_to_relevance, &it_end](const auto word) {
                 if (_word_to__id_freq_.find(word) != it_end) {
                     for (const auto& [document_id, freq] : _word_to__id_freq_.at(word)) {
                         document_to_relevance.Erase(document_id);
                     }

                  }
    });
    // Заполнение отмеченными документами (с использованием поля bucket_ класса ConcurrentMap)
    std::vector<Document> matched_documents(document_to_relevance.bucket_.size());
    std::atomic_uint index = 0;
    for (auto&& [mutex, map] : document_to_relevance.bucket_) {
        //std::lock_guard guard(mutex);
        if (map.empty()) {
            continue;
        }
        for (auto&& [document_id, relevance] : map) {
            matched_documents[index++] = { document_id, relevance, _id_docdata_.at(document_id).rating };
        }
    }
    matched_documents.resize(index);
    return matched_documents;
}

template <typename ExecutionPolicy>
std::vector<Document>
SearchServer::FindAllDocuments(ExecutionPolicy&& policy, const Query& query) const {

    ConcurrentMap<int, double> document_to_relevance(10007); //10007 - подобранное эффективное значение
    const auto it_end = _word_to__id_freq_.end();
    for_each(policy,
             query.plus_words.begin(), query.plus_words.end(),
             [this, &document_to_relevance, &it_end](const auto word) {
                 if (_word_to__id_freq_.find(word) != it_end) {
                     //Заполнение ConcurrentMap данными по плюс-словам
                     const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                     for(const auto& id_freq : _word_to__id_freq_.at(word)) {
                         const int document_id = id_freq.first;
                         const double term_freq = id_freq.second;
                         document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                     }
                 }
    });
    // Удаление данных по минус-словам
    for_each(policy,
             query.minus_words.begin(), query.minus_words.end(),
             [this, &document_to_relevance, &it_end](const auto word) {
                 if (_word_to__id_freq_.find(word) != it_end) {
                     for (const auto& [document_id, freq] : _word_to__id_freq_.at(word)) {
                         document_to_relevance.Erase(document_id);
                     }

                  }
    });
    // Заполнение отмеченными документами (с использованием поля bucket_ класса ConcurrentMap)
    std::vector<Document> matched_documents(document_to_relevance.bucket_.size());
    std::atomic_uint index = 0;
    for (auto&& [mutex, map] : document_to_relevance.bucket_) {
        //std::lock_guard guard(mutex);
        if (map.empty()) {
            continue;
        }
        for (auto&& [document_id, relevance] : map) {
            matched_documents[index++] = { document_id, relevance, _id_docdata_.at(document_id).rating };
        }
    }
    matched_documents.resize(index);
    return matched_documents;
}

template<typename ExecutionPolicy>
void SearchServer::RemoveDocument(ExecutionPolicy&& policy, int document_id) {

    const std::map<std::string_view, double>& _word_freq = _id_to__word_freq_.at(document_id);
    if (_word_freq.empty()) {
        throw std::invalid_argument("Id is wrong"s);
    }
    std::vector<std::string_view> words(_word_freq.size());
    transform(policy,
              _word_freq.begin(), _word_freq.end(),
              words.begin(),
              [](const std::pair<std::string_view, double> word_freq) {
                  return word_freq.first;
              }
    );
    for_each(policy,
             words.begin(), words.end(),
             [this, document_id](const std::string_view word) {
                 _word_to__id_freq_.at(word).erase(document_id);
             }
    );
    _id_docdata_.erase(document_id);
    _id_to__word_freq_.erase(document_id);
    _id_.erase(document_id);
}

//----------END OF CLASS--------------


void AddDocument(SearchServer& search_server, int document_id, const std::string_view document,
                 DocumentStatus status, const std::vector<int>& ratings);

void MatchDocuments(const SearchServer& search_server, const std::string_view query);

template <typename ExecutionPolicy>
void FindTopDocuments(ExecutionPolicy&& policy, const SearchServer& search_server, const std::string_view raw_query);
