#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <time.h>
#include <vector>
#include <fmt/core.h>
#include "httpStatus.h"
#include <fstream>

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
    vec_str.push_back(s.substr(start, end - start));
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

inline std::string get_html_header(std::string type, int statusCode=200) {
    /*
    HTTP/1.1 200 OK
    Date: Mon, 27 Jul 2009 12:28:53 GMT
    Server: Apache/2.2.14 (Win32)
    Last-Modified: Wed, 22 Jul 2009 19:15:56 GMT
    Content-Length: 88
    Content-Type: text/html; charset=iso-8859-1
    Connection: Closed*/
    std::string httpHeader = fmt::format(
    "HTTP/1.1 {} {}\r\n"
    // "Date: {}\r\n"
    // "Server: My Server/1.0\r\n"
    // "Last-Modified: {}\r\n"
    // "Content-Length: {}\r\n"
    "Content-Type: text/{}\r\n"
    "Connection: Closed\r\n\r\n"
    , 
    statusCode,      
    HttpStatus::reasonPhrase(statusCode),
    type
    // currentTime,
    // currentTime,
    // httpBody.length()
    );
    return httpHeader;
}

std::string read_file_content(std::string filepath) {
    std::ifstream ifs(filepath);
    std::string content( (std::istreambuf_iterator<char>(ifs) ),
                            (std::istreambuf_iterator<char>()) );
    // std::cout << "read from " << filepath << ": \n";
    // std::cout << content << std::endl;
    return content;
}

#endif