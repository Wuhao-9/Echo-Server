#include "acceptor_func.h"
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <iostream>
::pthread_cond_t acceptor_util::acceptor_cond; // define
::pthread_mutex_t acceptor_util::acceptor_mutex;

void* accept_thread_func(void* arg) {
    using namespace acceptor_util;
    while (!sync_util::thread_stop) {
        ::pthread_mutex_lock(&acceptor_mutex);
        ::pthread_cond_wait(&acceptor_cond, &acceptor_mutex);

        sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof client_addr;
        int new_client = ::accept(initialized_instance::listener_fd, (sockaddr*)&client_addr, &client_addr_len);
        if (new_client == -1) { // 此情况一般不会出现
            // std::cerr << "Accept fail!" << std::endl;
            ::pthread_mutex_unlock(&acceptor_mutex);
            continue;
        }

        std::cout << "new client connected: " << ::inet_ntoa(client_addr.sin_addr) << ":" << ::ntohs(client_addr.sin_port) << std::endl;

        // 将新的socket设为non-blocking
        int old_flag = ::fcntl(new_client, F_GETFL, 0);
        int new_flag = old_flag | O_NONBLOCK;
        ::fcntl(new_client, F_SETFL, new_flag);

        epoll_event add_event;
        add_event.data.fd = new_client;
        add_event.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
        if (::epoll_ctl(initialized_instance::epoll_fd, EPOLL_CTL_ADD, new_client, &add_event) == -1) {
            std::cout << "epoll_ctl error, fd =" << new_client << std::endl;
            continue;
        }
    }
    return nullptr;
}