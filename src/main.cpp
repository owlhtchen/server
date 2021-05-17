#include <unistd.h>

// Server side C/C++ program to demonstrate Socket programming
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <optional>
#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <fmt/core.h>
#include <string.h>
#include "httpStatus.h"
#include "utils.h"
#include "config.h"
#include <fstream>
#include "ThreadPool.h"
#include <functional>
#include <map>
#include <regex>
#include <optional>

class Socket {
    int fd;
public:
    Socket(int fd):fd(fd){}
    Socket(const Socket&)=delete;
    Socket&operator=(const Socket&)=delete;
    Socket(Socket&&rhs):fd(rhs.fd){rhs.fd = -1;}// auto a = std::move(b);
    Socket & operator=(Socket&&rhs){
        if(fd >= 0){
            close(fd);
        }
        fd = rhs.fd;
        rhs.fd = -1;
        return *this;
    }
    void write(const char * buf, size_t len) {
        send(fd, buf, len, 0);
    }
    size_t read(char * buf, size_t len) {
        size_t valread = ::read(fd , buf, len);
        return valread;
    }
    void sendResponse(std::string httpBody, std::string type="html", int statusCode=200) {
        std::string httpHeader = get_html_header(type);
        auto httpRespond = httpHeader + httpBody;
        write(httpRespond.c_str(), httpRespond.length());
    }
    void sendContent(std::string content, std::string title, std::string type="html", int statusCode=200) {
        std::string httpBody = html_template(content, title);
        sendResponse(httpBody, type);
    }
    void sendFile(std::string filepath, std::string title, int statusCode=200) {
        if(auto content = read_file_content(filepath)){
            // std::cout << "filepath: " << filepath << std::endl;
            auto splitted = split_str(filepath, ".");
            std::string type = "plain";
            if(splitted.size() > 0) {
                type = splitted.back();
            }
            // std::cout << "type: " << type << std::endl;
            sendResponse(*content, type);
        } else{
            sendContent("<p>404</p>", title, "html", 404); 
        }
    }
    ~Socket(){
        if(fd >= 0){
            close(fd);
        }
    }
    int getFd()const{
        return fd;
    }
};

class TcpListener {
    struct Data {
         int fd;
        struct sockaddr_in address;
        Data(int fd,struct sockaddr_in address):fd(fd),address(address){}
        ~Data(){
            if(fd >= 0){
                close(fd);
            }
        }
    };
    std::shared_ptr<Data> data;
public:
    TcpListener(int fd,struct sockaddr_in address ){
        data = std::make_shared<Data>(fd, address);
    }
    std::optional<Socket> accept(){
        auto& fd = data->fd;
        struct sockaddr_in client_address;
        auto&address = data->address;
        int new_socket;
        int addrlen = sizeof(address);
        if ((new_socket = ::accept(fd, (struct sockaddr *)&address, 
                           (socklen_t*)&address))<0)
        {
            perror("accept");
            return std::nullopt;
        }
        // printf("accepted!\n");
        return Socket(new_socket);
    }
    static std::optional<TcpListener> bind(int port, int max_pending) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if(fd == 0){
            return std::nullopt;
        }
        struct sockaddr_in address;
        int opt = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                      &opt, sizeof(opt)))
        {
            perror("setsockopt");
            return std::nullopt;
        }  
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons( port );       

        if (::bind(fd, (struct sockaddr *)&address, 
                                     sizeof(address))<0)
        {
            perror("bind failed");
            return std::nullopt;
        }
        if (listen(fd, max_pending) < 0)
        {
            perror("listen");
            return std::nullopt;
        }

        return TcpListener {
            fd,
            address
        };
    }
    ~TcpListener(){
        
    }
};
struct Request{
    std::map<std::string, std::string> params;
    void setParam(std::string key, std::string value) {
        params[key] = value;
    }
    std::string getParam(std::string key) {
        auto iter = params.find(key);
        if(iter != params.end()) {
            return iter->second;
        }
        return "undefined";
    }
};
struct Response {
    bool terminated;
    Socket& socket;
    int status; // TODO
    Response(Socket& _socket): terminated(false), socket(_socket) {};
    void sendContent(std::string content, std::string title, std::string type="html", int statusCode=200) {
        terminated = true;
        socket.sendContent(content, title, type, statusCode);
    }
    void sendFile(std::string filepath, std::string title, int statusCode=200) {
        terminated = true;
        socket.sendFile(filepath, title, statusCode);
    }    
};
struct PathCallback{
    std::string method;
    std::string path;
    std::string regex_path;
    std::vector<std::string> params;
    std::vector<std::function<void(Request&, Response&)>> callbacks;

    PathCallback(std::string method, std::string path, 
        std::vector<std::function<void(Request&, Response&)>> callbacks):
        method(method), path(path), callbacks(std::move(callbacks)){
        // "/user/:userid/book/:bookid"
        std::string log = path;
        std::string pattern = "(:[^/#?]+)";
        std::regex r(pattern);
        std::smatch sm;
        while(std::regex_search(log, sm, r))
        {
            params.push_back(sm.str().substr(1)); // add key
            log = sm.suffix();
        }    
        regex_path = std::regex_replace(path, r, "([^/#?]+)");
    }

    std::optional<Request> matches(std::string route_path) {
        // "/user/user123/book/456book"
        std::smatch pieces_match;
        if (std::regex_match(route_path, pieces_match, std::regex(regex_path))) {
            Request req;
            if(pieces_match.size() - 1 != params.size()) {
                printf("path match but params after colon(:) doesn't match\n");
                return std::nullopt;
            } 
            for (size_t i = 1; i < pieces_match.size(); ++i) {
                std::ssub_match sub_match = pieces_match[i];
                std::string piece = sub_match.str();
                req.setParam(params[i - 1], piece);
            }
            return req;   
        } else {
            return std::nullopt;
        }        
    }
};
class Application {
    std::vector<PathCallback> path_callbacks;
public:
    void handle_path(Socket& socket, std::string route_path) {
        for(auto& path_callback: path_callbacks) {
            auto req_opt = path_callback.matches(route_path);
            if(req_opt) {
                Request req(std::move(*req_opt));
                Response res(socket);
                for(auto& callback: path_callback.callbacks) {
                    if(!res.terminated) {
                        callback(req, res);
                    }
                }
                return;
            }
        }
        auto title = get_title_from_path(route_path);
        auto filepath = static_file_folder + route_path;
        socket.sendFile(filepath, title);   // 2
        // socket.sendContent("<p>hi</p>", title);  // 1        
    }

    void listen(int port) {
        ThreadPool pool(100);
        if(auto listener = TcpListener::bind(port, 50)){
            while(true){
                if(auto socket_opt = listener->accept()) {
                    std::shared_ptr<Socket> p_socket(new Socket(std::move(*socket_opt)));
                    pool.add([=]()mutable {
                        auto&socket = *p_socket;
                        // read: () -> std::vector<char>
                        const size_t READ_SIZE = 100000;
                        char request[READ_SIZE + 1];
                        int nbytes = socket.read(request, READ_SIZE);
                        if(nbytes == 0)
                            return;
                        // fmt::print("request {}: {}\n", nbytes, std::string_view(request, nbytes));
                        
                        auto path = get_path(std::string_view(request, nbytes));
                        handle_path(socket, path);
                    });
                }
            }
        }else{
            printf("cannot create listener\n");
        }
    }

    template<class... Fs>
    // const std::string& (copy constructor), std::string&& (move constructor)
    void get(const std::string& path, Fs&&... cbs) {
        std::vector<std::function<void(Request&, Response&)>> callbacks = {cbs...};
        path_callbacks.emplace_back("GET", path, std::move(callbacks));
    }

};

int main() {
    char const*hello = "Hello from server";
    Application app;

    app.get("/user", [](Request req, Response res) {
        res.sendContent("<p>hi user</p>", "User Greeting");
    });
    app.get("/book/:bookid", [](Request req, Response res) {
        auto content = fmt::format("<p>book number: {}</p>.", req.getParam("bookid"));
        res.sendContent(content, "Book");
    });    
    app.listen(8000);
}