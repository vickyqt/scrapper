#include <iostream>
#include <libxml/xmlmemory.h>
#include <optional>
#include <string>
#include <curl/curl.h>
#include <vector>
#include "libxml/HTMLparser.h"
#include "libxml/parser.h"
#include "libxml/xmlstring.h"
#include "libxml/xpath.h"

struct Quotes {
    std::string text;
    std::string author;
};

std::string get_request(std::string url) {
    // initialize curl locally
    CURL *curl = curl_easy_init();
    std::string result;

    if (curl) {
        // perform the GET request
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(
            curl, CURLOPT_WRITEFUNCTION,
            +[](void *contents, size_t size, size_t nmemb, std::string *response) {
                response->append((char*) contents, size * nmemb);
                return size * nmemb;
            }
        );

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
        curl_easy_perform(curl);

        // free up the local curl resources
        curl_easy_cleanup(curl);
    }
    return result;
}

std::optional<std::string> get_text(const char *expression, xmlXPathContextPtr context) {
    xmlXPathObjectPtr text_nodes = xmlXPathEvalExpression((xmlChar *)expression, context);
    if (text_nodes == nullptr || text_nodes->nodesetval == nullptr || text_nodes->nodesetval->nodeNr <= 0) {
        xmlXPathFreeObject(text_nodes);
        return std::nullopt;
    }

    xmlNodePtr text_node = text_nodes->nodesetval->nodeTab[0];
    xmlChar *raw_text = xmlNodeGetContent(text_node);
    std::string string_text =
        std::string(reinterpret_cast<char *>(raw_text != nullptr ? raw_text : (xmlChar *)"Nothing"));

    xmlFree(raw_text);
    xmlXPathFreeObject(text_nodes);

    return string_text;
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    curl_global_init(CURL_GLOBAL_ALL);

    std::string html_document = get_request("https://quotes.toscrape.com");
    htmlDocPtr doc = htmlReadMemory(
        html_document.c_str(), html_document.length(),
        nullptr, nullptr, HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING
    );
    xmlXPathContextPtr context = xmlXPathNewContext(doc);

    xmlXPathObjectPtr div_quote_nodes = xmlXPathEvalExpression(
        (xmlChar *) "//div[contains(@class, 'quote')]", context
    );
    if (div_quote_nodes == nullptr || div_quote_nodes->nodesetval == nullptr) {
        std::cerr << "error, doing freeing job.." << "\n";

        xmlXPathFreeContext(context);
        xmlFreeDoc(doc);
        curl_global_cleanup();

        return 1;
    }

    std::vector<Quotes> quotes;
    for (int i = 0; i < div_quote_nodes->nodesetval->nodeNr; i++) {
        xmlNodePtr div_quote_node = div_quote_nodes->nodesetval->nodeTab[i];
        xmlXPathSetContextNode(div_quote_node, context);

        // for quotes
        std::optional<std::string> quote_t = get_text(".//span[contains(@class, 'text')]", context);
        if (!quote_t) { continue; }

        // for authors
        std::optional<std::string> author_t = get_text(".//small[@class='author']", context);
        if (!author_t) { continue; }

        Quotes quote = { quote_t.value(), author_t.value() };
        quotes.push_back(quote);
    }

    for (const auto &i : quotes) {
        std::cout << i.text << "\n--> \x20\x20" << i.author << "\n\n";
    }

    xmlXPathFreeContext(context);
    xmlXPathFreeObject(div_quote_nodes);
    xmlFreeDoc(doc);
    curl_global_cleanup();
}


// GOOD LOARD THIS IS MY LIMIT WITH C++
// this specific code is only currently valid for that on site by changing the expressions, you can use is for other
// if you know what you are doing.
//
// TODO:
// 1. wrap some pointer to std::unique_ptr with custom deleter and make freeing compiler's problem.
// 2. seperate some function and parser, improvement in parser is also required.
// 3. lastly implement a crawller that will use this scrapper.
