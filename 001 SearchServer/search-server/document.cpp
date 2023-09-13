#include <iostream>
#include <string>
#include <vector>

#include "document.h"


using namespace std;

std::ostream& operator<<(std::ostream& out, const Document& document) {
    out << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s;
    return out;
}

void PrintDocument(const Document& document) {
    std::cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << std::endl;
}

void PrintMatchDocumentResult(int document_id, const std::vector<std::string_view>& words,
                              DocumentStatus status) {
    std::cout << "{ "s
              << "document_id = "s << document_id << ", "s
              << "status = "s << static_cast<int>(status) << ", "s
              << "words ="s;
    for (std::string_view word_sv : words) {
        const string& word = std::string(word_sv);
        std::cout << ' ' << word;
    }
    std::cout << "}"s << std::endl;
}
