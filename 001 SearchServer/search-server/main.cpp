#include <algorithm>
#include <cmath>
#include <deque>
#include <iostream>
#include <iomanip>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <execution>
#include <random>

#include "test_example_functions.h"
#include "search_server.h"
#include "request_queue.h"
#include "document.h"
#include "paginator.h"
#include "read_input_functions.h"
#include "string_processing.h"
#include "log_duration.h"
#include "process_queries.h"

using namespace std;
//using namespace std::string_literals;

/*
ACTUAL by default:
{ document_id = 2, relevance = 0.866434, rating = 1 }
{ document_id = 4, relevance = 0.231049, rating = 1 }
{ document_id = 1, relevance = 0.173287, rating = 1 }
{ document_id = 3, relevance = 0.173287, rating = 1 }
BANNED:
Even ids:
{ document_id = 2, relevance = 0.866434, rating = 1 }
{ document_id = 4, relevance = 0.231049, rating = 1 }

Total relevance: 267.83

Match documents: 2164

5 documents total, 4 documents for query [curly and funny]
4 documents total, 3 documents for query [curly and funny]
3 documents total, 2 documents for query [curly and funny]
2 documents total, 1 documents for query [curly and funny]
10000->0
*/


int main() {

//-----FindTopDocument and ProcessQueries and MatchDocument

        SearchServer search_server("and with"s);
        int id = 0;
        for (
            const string& text : {
                "white cat and yellow hat"s,
                "curly cat curly tail"s,
                "nasty dog with big eyes"s,
                "nasty pigeon john"s,
            }
        ) { search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2}); }
    {
        cout << "ACTUAL by default:"s << endl;
       // последовательная версия
       for (const Document& document : search_server.FindTopDocuments(std::execution::seq, "curly nasty cat"s)) {
           PrintDocument(document);
       }
       cout << "BANNED:"s << endl;
       // последовательная версия
       for (const Document& document : search_server.FindTopDocuments(
                execution::seq, "curly nasty cat"s, DocumentStatus::BANNED)) {
           PrintDocument(document);
       }
       cout << "Even ids:"s << endl;
       // параллельная версия
       for (const Document& document : search_server.FindTopDocuments(
                execution::par, "curly nasty cat"s,
                [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
           PrintDocument(document);
       }
    }

    {
        mt19937 generator;
        const auto dictionary = GenerateDictionary(generator, 1000, 10);
        const auto documents = GenerateQueries(generator, dictionary, 10'000, 70);
        SearchServer search_server(dictionary[0]);
        for (size_t i = 0; i < documents.size(); ++i) {
            search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
        }
        const auto queries = GenerateQueries(generator, dictionary, 100, 70);
        const string query = GenerateQuery(generator, dictionary, 500, 0.1);

        TEST_FT(seq);
        TEST_FT(par);

        TEST_QP(ProcessQueries);
        TEST_QP(ProcessQueriesJoined);

        TEST_Mt(seq);
        TEST_Mt(par);

    }


//------FindTopDocument.End


//----RemoveDocument
    {
        SearchServer search_server("and with"s);
        int id = 0;
        for (
            const string& text : {
                "funny pet and nasty rat"s,
                "funny pet with curly hair"s,
                "funny pet and not very nasty rat"s,
                "pet with rat and rat and rat"s,
                "nasty rat with curly hair"s,
            }
        ) {
            search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
        }

        const string query = "curly and funny"s;

        auto report = [&search_server, &query] {
            cout << search_server.GetDocumentCount() << " documents total, "s
                << search_server.FindTopDocuments(query).size() << " documents for query ["s << query << "]"s << endl;
        };
        report();
        // однопоточная версия
        search_server.RemoveDocument(5);
        report();
        // однопоточная версия
        search_server.RemoveDocument(execution::seq, 1);
        report();
        // многопоточная версия
        search_server.RemoveDocument(execution::par, 2);
        report();

        {
            mt19937 generator;

            const auto dictionary = GenerateDictionary(generator, 10'000, 25);
            const auto documents = GenerateQueries(generator, dictionary, 10'000, 100);

            {
                SearchServer search_server(dictionary[0]);
                for (size_t i = 0; i < documents.size(); ++i) {
                    search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
                }
                TEST_RD(seq);
            }
            {
                SearchServer search_server(dictionary[0]);
                for (size_t i = 0; i < documents.size(); ++i) {
                    search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
                }
                TEST_RD(par);
            }
        }
    }
//----RemoveDocument.End


     return 0;
}
