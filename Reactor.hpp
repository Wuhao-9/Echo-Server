#if !defined(REACTOR_HPP_)
#define REACTOR_HPP_

#include "eventHandler.hpp"
#include <sys/epoll.h>
#include <iostream>
#include <unistd.h>
#include <string.h>
// class EventHandler;

// Reactor类，负责管理IO多路复用器和事件处理器之间的关系，并分发IO事件到对应的处理器中执行回调函数。
class Reactor {
public:
    Reactor() {
        epoll_instance_fd_ = ::epoll_create(1);
        if (epoll_instance_fd_ == -1) {
            ::perror("epoll instance create failure");
            ::exit(EXIT_FAILURE);            
        }
    }

    ~Reactor() {
        ::close(epoll_instance_fd_);
    }
    
    Reactor(const Reactor&) = delete;

    void register_handler(EventHandler* handler, unsigned events) {
        struct epoll_event event;
        event.data.ptr = handler; // 将事件处理器作为用户数据存储在event结构体中，方便后续获取和调用。
        event.events = events;

        int ret = epoll_ctl(epoll_instance_fd_, EPOLL_CTL_ADD, handler->getFd(), &event); 
        if (ret == -1) {
            std::cerr << "Register handler error: " << strerror(errno) << std::endl;
        }
    }

    void remove_handler(EventHandler* handler) {
        int ret = epoll_ctl(epoll_instance_fd_, EPOLL_CTL_DEL, handler->getFd(), nullptr);
        if (ret == -1) {
            std::cerr << "Remove handler error: " << strerror(errno) << std::endl;
        }
        delete handler;
    }

    void dispatch() { // 分发IO事件到对应的事件处理器中执行回调函数
        const int MAX_EVENTS = 50; // 定义每次最多监听50个就绪的IO事件
        ::epoll_event ready_events[MAX_EVENTS];
        while (true) {
            int n = ::epoll_wait(epoll_instance_fd_, ready_events, MAX_EVENTS, -1);
            if (n == -1) {
                ::perror("epoll_wait error");
                continue;
            }

            for (int i = 0; i < n; i++) {
                EventHandler* handler = static_cast<EventHandler*>(ready_events[i].data.ptr);
                auto event_type = ready_events->events;
                if (event_type & EPOLLIN) {
                    handler->handleRead(); // 调用事件处理器的handleRead函数
                } else if (event_type & EPOLLOUT) {
                    handler->handleWrite(); // 调用事件处理器的handleWrite函数
                } else if (event_type & (EPOLLERR | EPOLLHUP)) { // 如果是其他类型的事件（如错误或挂起），打印警告信息并关闭文件描述符
                    // remove_handler(handler); // 不再关注该链接相关信息，关闭连接
                    // logic of handle error.....
                } else {
                    remove_handler(handler);
                }
            }
        }
    }
private:
    int epoll_instance_fd_;    
};


#endif // REACTOR_HPP_
