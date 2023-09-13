#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server, const std::vector<std::string>& queries) {

    std::vector<std::vector<Document>> documents(queries.size());

    transform(std::execution::par,
        queries.begin(),
        queries.end(),
        documents.begin(),
        [&search_server](const std::string_view query) {
           return search_server.SearchServer::FindTopDocuments(std::execution::par, query);
  });
    return documents;
}

std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server, const std::vector<std::string>& queries) {

    std::list<Document> documents;
    for (auto&& __doc : ProcessQueries(search_server, queries)) {
        tbb::blocked_range range(__doc.begin(), __doc.end());
        tbb::parallel_for(range,
                          [&documents](auto&& _doc) {
                              documents.insert(documents.end(), _doc.begin(), _doc.end());
        });
    }
    return documents;
}
