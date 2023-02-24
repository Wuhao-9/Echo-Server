/**
 * @author Wuhao9_ (1017174960@qq.com)
 * @brief 基于Reactor模型的echo服务器
 * @date 2023-02-24
 * @copyright Copyright (c) 2023
 *
 */
#include "acceptor_func.h"
#include "worker_func.h"
#include <unistd.h>
#include <iostream>
#include <arpa/inet.h>
#include <sys/epoll.h>

const int MAX_WORK_THREADS = 15;

namespace initialized_instance {
    int listener_fd = -1;
    int epoll_fd = -1;
    ::sockaddr_in server_addr {};
    ::pthread_t thread_pool[MAX_WORK_THREADS] {};
}

bool create_server_listener(unsigned host, unsigned short port);

int main(int argc, char* argv[]) {
    int option;
    unsigned short port = 1993;  // default port
    bool is_daemon = false;

    while ((option = ::getopt(argc, argv, "dp:")) != -1) {
        switch (option)
        {
        case 'p':
            port = std::stoi(::optarg);
            break;
        case 'd':
            is_daemon = true;
            break;
        case '?':
            std::cerr << "Usage: Echo [-p <num>] [-d]" << std::endl;
        }
    }

    if (!create_server_listener(INADDR_ANY, port)) {
        char host_str[INET_ADDRSTRLEN + 1] {};
        std::cerr << "Unable to start listen server: ip=" << ::inet_ntop(AF_INET, &initialized_instance::server_addr.sin_addr.s_addr, host_str, INET_ADDRSTRLEN) << ", port=" << port << std::endl;
        return -1;
    }

    ::pthread_t acceptor_thread;
    ::pthread_create(&acceptor_thread, nullptr, accept_thread_func, nullptr);
    ::pthread_detach(acceptor_thread);

    for (int i = 0; i < MAX_WORK_THREADS; i++) {
        ::pthread_create(&initialized_instance::thread_pool[i], nullptr, wroker_func, nullptr);
        ::pthread_detach(initialized_instance::thread_pool[i]); // detach子线程，线程结束时自动释放相关资源
    }

    epoll_event completed_events[1024];
    while (true) {
        auto comp_nums = ::epoll_wait(initialized_instance::epoll_fd, completed_events, 1024, -1);
        if (comp_nums == 0) { } // 此处epoll_wait()设置为非阻塞，故不会返回0
        else if (comp_nums > 0) {
            for (int i = 0; i < comp_nums; i++) {
                int cur_fd = completed_events[i].data.fd;
                if (cur_fd == initialized_instance::listener_fd) { // 有客户端连接，转交给Acceptor
                    // acceptor_func();
                } else { // 有客户端请求，转交给Work-thread
                    if (completed_events[i].events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) {
                        release_client(cur_fd);
                        continue;
                    }
                    pthread_mutex_lock(&sync_util::client_mutex);
                    client_queue.emplace_back(cur_fd);
                    pthread_mutex_unlock(&sync_util::client_mutex);
                    pthread_cond_signal(&sync_util::client_cond);
                }
            }
        }
    }
    return 0;
}




bool create_server_listener(unsigned host, unsigned short port) {
    using namespace initialized_instance;

    listener_fd = ::socket(PF_INET, SOCK_STREAM, 0);
    if (listener_fd == -1)
        return false;

    int is_set = 1;
    int ret = ::setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &is_set, sizeof is_set);

    // ::inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr.s_addr);
    server_addr.sin_addr.s_addr = ::htonl(host);
    server_addr.sin_port = ::htons(port);
    server_addr.sin_family = AF_INET;

    ret = ::bind(listener_fd, (sockaddr*)&server_addr, sizeof server_addr);
    if (ret == -1)
        return false;

    ret == ::listen(listener_fd, 128);

    epoll_fd = ::epoll_create(1);
    if (epoll_fd == -1)
        return false;

    ::epoll_event event_struct;
    event_struct.data.fd = listener_fd;
    event_struct.events = EPOLLIN;

    ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listener_fd, &event_struct);
    if (ret == -1)
        return false;

    return true;
}
