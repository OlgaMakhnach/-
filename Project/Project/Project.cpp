#include <iostream>
#include <string>
#include <fstream>
#include <curl/curl.h>
#include <gumbo.h>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

namespace WebParser {

    // Функция для записи данных, полученных с помощью libcurl
    size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* s) {
        size_t newLength = size * nmemb;
        s->append((char*)contents, newLength);
        return newLength;
    }

    // Функция для выполнения HTTP-запроса и получения HTML-страницы
    string fetchPage(const string& url) {
        CURL* curl = curl_easy_init();
        string readBuffer;

        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            curl_easy_perform(curl);
            curl_easy_cleanup(curl);
        }
        return readBuffer;
    }

    // Рекурсивная функция для поиска текста в узлах HTML с помощью gumbo-parser
    string searchForText(GumboNode* node, const string& searchText) {
        if (node->type != GUMBO_NODE_ELEMENT) return "";

        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            string result = searchForText(static_cast<GumboNode*>(children->data[i]), searchText);
            if (!result.empty()) return result;
        }

        if (node->v.element.tag == GUMBO_TAG_TITLE) {
            GumboNode* title_text = static_cast<GumboNode*>(node->v.element.children.data[0]);
            if (title_text && title_text->type == GUMBO_NODE_TEXT) {
                string content(title_text->v.text.text);
                if (content.find(searchText) != string::npos) return content;
            }
        }
        return "";
    }

}

int main() {
    using namespace WebParser;

    string url, searchText;
    cout << "Введите URL: ";
    cin >> url;
    cout << "Введите текст для поиска: ";
    cin >> searchText;

    // Получение HTML-страницы
    string pageContent = fetchPage(url);

    // Парсинг HTML-страницы с помощью gumbo-parser
    GumboOutput* output = gumbo_parse(pageContent.c_str());
    string foundText = searchForText(output->root, searchText);
    gumbo_destroy_output(&kGumboDefaultOptions, output);

    // Формирование JSON-объекта
    json jsonObj;
    jsonObj["url"] = url;
    jsonObj["search_text"] = searchText;
    jsonObj["found_text"] = foundText;

    // Запись в файл JSON
    ofstream outFile("result.json");
    if (outFile.is_open()) {
        outFile << jsonObj.dump(4); // Красивый вывод с отступом в 4 пробела
        outFile.close();
        cout << "Результат записан в result.json" << endl;
    }
    else {
        cerr << "Не удалось открыть файл для записи." << endl;
    }

    return 0;
}
