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
#include "httpStatus.h"
#include "utils.h"
#include "config.h"
#include <fstream>

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
    void sendResponse(std::string httpBody, std::string title, std::string type="html", int statusCode=200) {
        std::string httpHeader = get_html_header(type);
        auto httpRespond = httpHeader + httpBody;
        write(httpRespond.c_str(), httpRespond.length());
        std::cout << std::endl;
    }
    void sendContent(std::string content, std::string title, std::string type="html", int statusCode=200) {
        std::string httpBody = html_template(content, title);
        sendResponse(httpBody, title);
    }
    void sendFile(std::string filepath, std::string title, int statusCode=200) {
        auto content = read_file_content(filepath);
        std::cout << "filepath: " << filepath << std::endl;
        auto splitted = split_str(filepath, ".");
        std::string type = "plain";
        if(splitted.size() > 0) {
            type = splitted.back();
        }
        std::cout << "type: " << type << std::endl;
        sendResponse(content, title, type);
    }
    ~Socket(){
        if(fd >= 0){
            close(fd);
        }
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
        printf("accepted!\n");
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

int main() {
    char const*hello = "Hello from server";
    if(auto listener = TcpListener::bind(8000, 10)){
        while(true){
            if(auto socket_opt = listener->accept()) {
                auto& socket = *socket_opt;
                const size_t READ_SIZE = 100000;
                char request[READ_SIZE+1];
                socket.read(request, READ_SIZE);
                // printf("%s\n", request);
                auto path = get_path(request);
                auto title = get_title_from_path(path);
                std::cout << path << std::endl;
                auto filepath = static_file_folder + path;
                auto content = read_file_content(filepath);
                socket.sendFile(filepath, title);   // 2
                // socket.sendContent("<p>hi</p>", title);  // 1
            }
        }
    }else{
        printf("cannot create listener\n");
    }
    // int server_fd, new_socket, valread;
    // struct sockaddr_in address;
    // int opt = 1;
    // int addrlen = sizeof(address);
    // char buffer[1024] = {0};
    // char *hello = "Hello from server";
       
    // // Creating socket file descriptor
    // if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    // {
    //     perror("socket failed");
    //     exit(EXIT_FAILURE);
    // }
       
    // // Forcefully attaching socket to the port 8080
    // if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
    //                                               &opt, sizeof(opt)))
    // {
    //     perror("setsockopt");
    //     exit(EXIT_FAILURE);
    // }
    // address.sin_family = AF_INET;
    // address.sin_addr.s_addr = INADDR_ANY;
    // address.sin_port = htons( PORT );
       
    // // Forcefully attaching socket to the port 8080
    // if (bind(server_fd, (struct sockaddr *)&address, 
    //                              sizeof(address))<0)
    // {
    //     perror("bind failed");
    //     exit(EXIT_FAILURE);
    // }
    // if (listen(server_fd, 3) < 0)
    // {
    //     perror("listen");
    //     exit(EXIT_FAILURE);
    // }
    // if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
    //                    (socklen_t*)&addrlen))<0)
    // {
    //     perror("accept");
    //     exit(EXIT_FAILURE);
    // }
    // valread = read( new_socket , buffer, 1024);
    // printf("%s\n",buffer );
    // send(new_socket , hello , strlen(hello) , 0 );
    // printf("Hello message sent\n");
    // return 0;
}