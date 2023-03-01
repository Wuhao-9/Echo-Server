#if !defined(ACCEPTOR_HPP_)
#define ACCEPTOR_HPP_

#include "Reactor.hpp"
#include "EchoHandler.hpp"
#include "eventHandler.hpp"
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>

class Acceptor : public EventHandler {
public:
    Acceptor(const char* ip, unsigned short port, Reactor& reactor)
        : base_reactor_(reactor) {
        listen_fd_ = ::socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0); 
        if (listen_fd_ == -1) {
            ::perror("create listen fd failure");
            exit(EXIT_FAILURE);
        }
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        ::inet_pton(AF_INET, ip, &addr.sin_addr);

        int opt = 1; // SO_REUSEADDR表示允许重用本地地址和端口，SO_REUSEPORT表示允许多个套接字绑定同一端口
        if (::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
            ::perror("setsockopt error");
            // ::exit(EXIT_FAILURE);
        }

        if (::bind(listen_fd_, (sockaddr*)&addr, sizeof(addr)) == -1) {
            ::perror("bind error");
            ::exit(EXIT_FAILURE);
        }

        if (::listen(listen_fd_, 10) == -1) {
            ::perror("listen error");
            ::exit(EXIT_FAILURE);
        }
    }   
    
    virtual int getFd() override {
        return listen_fd_;
    }

    virtual void handleRead() override {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        // 调用accept4函数接受客户端连接请求，并返回一个非阻塞套接字文件描述符
        int conn_fd = accept4(listen_fd_, (sockaddr*)&client_addr, &client_len, SOCK_NONBLOCK | SOCK_CLOEXEC);

        if (conn_fd == -1) {
            ::perror("accept error");
        }

        #ifdef PRINT_REMOTE_ADDR
        char client_ip[INET_ADDRSTRLEN + 1] = {};
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        std::cout << "Accepted connection from: " << client_ip << ":" << ::ntohs(client_addr.sin_port);
        #endif
        
        EventHandler* conn_handler = new EchoHandler(conn_fd, base_reactor_);
        // 将EchoHandler对象注册到epoll中，并设置感兴趣的事件类型为可读
        base_reactor_.register_handler(conn_handler, EPOLLIN);
    }

private:
    void handleWrite() {}
    Reactor& base_reactor_; // Acceptor所属的Reactor实例
    int listen_fd_;
};

#endif // ACCEPTOR_HPP_
