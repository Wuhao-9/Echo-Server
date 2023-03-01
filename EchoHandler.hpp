#if !defined(ECHOHANDLER_HPP_)
#define ECHOHANDLER_HPP_

#include "eventHandler.hpp"
#include "Reactor.hpp"
#include <unistd.h>
#include <iostream>
class EchoHandler : public EventHandler {
public:
    EchoHandler(const int file_descriptor, Reactor& reactor) 
        : fd_(file_descriptor)
        , base_reactor_(reactor) {}

    ~EchoHandler() { ::close(fd_); }

    virtual void handleRead() override {
        char buf[1024];
        int n = read(fd_, buf, sizeof(buf));
        if (n > 0) {
            std::cout << "Received: " << std::string(buf, n) << std::endl; // 打印收到的数据
            write(fd_, buf, n); // 将收到的数据写回文件描述符
        } else if (n == 0) {
            base_reactor_.remove_handler(this);
        } else { // 如果读取出错，打印错误信息并关闭文件描述符
            ::perror("An error has occurred in Read Callback");
            base_reactor_.remove_handler(this);
        }
    }

    virtual int getFd() override {
        return fd_;
    }

private:
    virtual void handleWrite() override {}
    Reactor& base_reactor_; // Handler所属的Reactor实例
    int fd_;  // 用于和客户端通信的文件描述符
};

#endif // ECHOHANDLER_HPP_
