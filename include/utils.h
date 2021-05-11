#ifndef UTILS
#define UTILS

#include <string>
#include <time.h>
#include <vector>
#include <fmt/core.h>
#include "httpStatus.h"

inline std::vector<std::string> split_str(std::string s, std::string delim) {
    std::vector<std::string> vec_str;
    auto start = 0U;
    auto end = s.find(delim);
    while (end != std::string::npos)
    {
        vec_str.push_back(s.substr(start, end - start));
        start = end + delim.length();
        end = s.find(delim, start);
    }
    return vec_str;
}

inline std::string get_now() {
    char buf[1000];
    time_t now = time(0);
    struct tm tm = *gmtime(&now);
    strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S %Z", &tm);
    return std::string(buf);
}

inline std::string get_path(char* _request) {
    std::string request(_request);
    std::vector<std::string> methods = {
        "GET", "HEAD", "POST", "PUT",  "DELETE", "CONNECT", "OPTIONS", "TRACE"};
    std::string first = request.substr(0, request.find("\n"));
    for(auto& method: methods) {
        auto pos = first.find(method);
        if(pos != std::string::npos) {
            auto path = split_str(first, " ")[1];
            return path;
        }
    }
    return "";
}

inline std::string html_template(std::string content, std::string title) {
    std::string http = fmt::format(
    "<!DOCTYPE html>"
    "<HMTL>"
        "<HEAD>"
            "<TITLE>{}</TITLE>"
        "</HEAD>"
        "<BODY>"
        "{}"
        "</BODY>"
    "</HTML>", title, content);
    return http;
}

inline std::string get_title_from_path(std::string path) {
    if(path == "/") {
        return "Home Page";
    } else {
        auto splitted = split_str(path, "/");
        if(splitted.size() == 0) {
            return "Home Page";
        } else {
            return splitted.back();
        }
    }
}

#endif