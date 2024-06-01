#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <ctime>
#include <curl/curl.h>
#include <sstream>
#include <map>
#include "nlohmann/json.hpp"


using json = nlohmann::json;

#define SMTP_SERVER "smtp://smtp.gmail.com:587"
#define USERNAME "uselessmailsforever@gmail.com"
#define FROM_EMAIL "uselessmailsforever@gmail.com"

std::map<std::string, std::string> loadConfig(const std::string& filename) {
    std::map<std::string, std::string> config;
    std::ifstream file(filename);
    std::string line;
    
    if (file.is_open()) {
        while (getline(file, line)) {
            std::istringstream is_line(line);
            std::string key;
            if (getline(is_line, key, '=')) {
                std::string value;
                if (getline(is_line, value)) {
                    config[key] = value;
                }
            }
        }
        file.close();
    } else {
        std::cerr << "Unable to open the config file." << std::endl;
    }

    return config;
}

size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp) {
    std::string *payload = (std::string *)userp;
    size_t len = size * nmemb;

    if (payload->empty())
        return 0;

    size_t copy_size = std::min(len, payload->size());
    memcpy(ptr, payload->c_str(), copy_size);
    payload->erase(0, copy_size);

    return copy_size;
}

bool isTimeToSend(const std::string& sendtime) {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    struct tm *parts = std::localtime(&now_c);

    char buffer[6];
    strftime(buffer, sizeof(buffer), "%H:%M", parts);
    return sendtime == std::string(buffer);
}

int main() {
    std::string filename = "secrets.txt";
    auto config = loadConfig(filename);
    std::string password = config["password"];
    std::cout << password << std::endl;

    std::string file_path = "subscribe.json";
    json jsonData;
    std::ifstream inputFile(file_path);

    if (!inputFile.is_open()) {
        std::cerr << "Failed to open the file." << std::endl;
        return -1;
    }

    inputFile >> jsonData;
    inputFile.close();

    CURLcode res = CURLE_OK;
    while (true) {
        for (json &j : jsonData) {
            if (!j.contains("email") || !j["email"].is_string() ||
                !j.contains("subject") || !j["subject"].is_string() ||
                !j.contains("sendtime") || !j["sendtime"].is_string() ||
                !j.contains("content") || !j["content"].is_string() ||
                !isTimeToSend(j["sendtime"].get<std::string>())) {
                continue;
            }
            std::string email = j["email"];
            std::string subject = j["subject"];
            std::string content = j["content"];
            std::string payload_text = "To: " + email + "\r\n" +
                                       "From: " + FROM_EMAIL + "\r\n" +
                                       "Subject: " + subject + "\r\n" + "\r\n" +
                                       content + "\r\n";

            CURL *curl = curl_easy_init();
            struct curl_slist *recipients = NULL;
            if (curl) {
                curl_easy_setopt(curl, CURLOPT_USERNAME, USERNAME);
                curl_easy_setopt(curl, CURLOPT_PASSWORD, password);
                curl_easy_setopt(curl, CURLOPT_URL, SMTP_SERVER);
                curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
                curl_easy_setopt(curl, CURLOPT_MAIL_FROM, FROM_EMAIL);
                recipients = curl_slist_append(recipients, email.c_str());
                curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
                curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
                curl_easy_setopt(curl, CURLOPT_READDATA, &payload_text);
                curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

                res = curl_easy_perform(curl);

                if (res != CURLE_OK)
                    std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;

                curl_slist_free_all(recipients);
                curl_easy_cleanup(curl);
            }
        }

        std::this_thread::sleep_for(std::chrono::minutes(1));  // Wait for 1 minute
    }

    return (int)res;
}
