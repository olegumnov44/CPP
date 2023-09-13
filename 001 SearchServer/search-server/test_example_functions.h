#pragma once

#include <random>

#include "log_duration.h"
#include "search_server.h"
#include "process_queries.h"

using namespace std;

string GenerateWord(mt19937& generator, int max_length) {
    const int length = uniform_int_distribution(1, max_length)(generator);
    string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i) {
        word.push_back(uniform_int_distribution('a', 'z')(generator));
    }
    return word;
}

vector<string> GenerateDictionary(mt19937& generator, int word_count, int max_length) {
    vector<string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i) {
        words.push_back(GenerateWord(generator, max_length));
    }
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}

string GenerateQuery(mt19937& generator, const vector<string>& dictionary, int word_count, double minus_prob = 0) {
    string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
        }
        if (uniform_real_distribution<>(0, 1)(generator) < minus_prob) {
            query.push_back('-');
        }
        query += dictionary[uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
    }
    return query;
}

vector<string> GenerateQueries(mt19937& generator, const vector<string>& dictionary, int query_count, int max_word_count) {
    vector<string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i) {
        queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
    }
    return queries;
}

template <typename ExecutionPolicy>
void Test_FT(const string_view mark, const SearchServer& search_server, const vector<string>& queries, ExecutionPolicy&& policy) {
    LOG_DURATION(mark);
    //GLOB_Duration_test = {};
    double total_relevance = 0;
    for (const string_view query : queries) {
        for (const auto document : search_server.FindTopDocuments(policy, query)) {
            total_relevance += document.relevance;
        }
    }
    cout << "Total relevance: " << total_relevance << endl;
}

#define TEST_FT(policy) Test_FT("FT: " #policy, search_server, queries, execution::policy)

template <typename QueriesProcessor>
void Test_QP(const string_view mark, const QueriesProcessor processor, const SearchServer& search_server, const vector<string>& queries) {
    LOG_DURATION(mark);
    const auto documents_lists = processor(search_server, queries);
}

#define TEST_QP(processor) Test_QP("QP: " #processor, processor, search_server, queries)

template <typename ExecutionPolicy>
void Test_RD(string_view mark, SearchServer search_server, ExecutionPolicy&& policy) {
    LOG_DURATION(mark);
    cout << search_server.GetDocumentCount() << "->";
    const int document_count = search_server.GetDocumentCount();
    for (int id = 0; id < document_count; ++id) {
        search_server.RemoveDocument(policy, id);
    }
    cout << search_server.GetDocumentCount() << endl;
}

#define TEST_RD(policy) Test_RD("RD: " #policy, search_server, execution::policy)


template <typename ExecutionPolicy>
void Test_Mt(string_view mark, SearchServer search_server, const string& query, ExecutionPolicy&& policy) {
    LOG_DURATION(mark);
    //GLOB_Duration_Mt = {};
    //GLOB_Duration_PQ = {};

    const int document_count = search_server.GetDocumentCount();
    int word_count = 0;
    for (int id = 0; id < document_count; ++id) {
        const auto [words, status] = search_server.MatchDocument(policy, query, id);
        word_count += words.size();
    }
    cout << "Match documents: " << word_count << endl;
}

#define TEST_Mt(policy) Test_Mt("Mt: " #policy, search_server, query, execution::policy)

/*
    if (std::is_same_v<std::decay_t<ExecutionPolicy>, std::execution::sequenced_policy>)

    auto start_time1 = std::chrono::high_resolution_clock::now();
    auto  end_time1 = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<long, std::ratio<1, 1'000'000'000>> dur1 = end_time1 - start_time1;
    GLOB_Duration_Mt += dur1;
*/
