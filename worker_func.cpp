#include "worker_func.h"
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread> // for sleep_for()

std::list<int> client_queue;

namespace sync_util {
    bool thread_stop = false;
    pthread_mutex_t client_mutex;
    pthread_cond_t client_cond;
}

void release_client(int client_fd) {
    auto ret = ::epoll_ctl(initialized_instance::epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
    if (ret == -1) {
        std::cerr << "release client socket failed as call epoll_ctl failed" << std::endl;
    }
    ::close(client_fd);
}

void* wroker_func(void* args) {
    using namespace sync_util;
    while (!thread_stop) {
        ::pthread_mutex_lock(&client_mutex);
        while (client_queue.empty()) {
            ::pthread_cond_wait(&client_cond, &client_mutex);
        }
        int client_fd = client_queue.front();
        client_queue.pop_front();
        pthread_mutex_unlock(&client_mutex);

        std::string client_msg;
        bool is_error = false;

        while (true) {
            char buff[256] {};
            int transfered_bytes = ::recv(client_fd, buff, 256 - 1, 0); // 防止缓冲区溢出
            if (transfered_bytes == -1) {
                if (errno == EWOULDBLOCK || errno == EAGAIN)
                    break;
                else {
                    std::cerr << "recv error, The client[" << client_fd << "] connection is to be disconnected" << std::endl;
                    release_client(client_fd);
                    is_error = true;
                    break;
                }
            } else if(transfered_bytes == 0) {
                std::cout << "peer closed, client disconnected, fd = " << client_fd << std::endl;
                release_client(client_fd);
                is_error = true;
                break;
            } else {
                client_msg += buff;
            }
        }

        if (is_error) // 该客户端出错或断开连接，则直接处理写一个客户端请求
            continue;
        
        // 获取客户端请求后开始业务逻辑
        // 将消息加上时间标签后发回

        // 获取当前时间
        auto now_timePoint = std::chrono::system_clock::now();
        auto now_C_time = std::chrono::system_clock::to_time_t(now_timePoint);
        auto now = ::localtime(&now_C_time);

        std::ostringstream os_timestr;
        os_timestr << "[" << now->tm_year + 1900 << "-"
            << std::setw(2) << std::setfill('0') << now->tm_mon + 1 << "-"
            << std::setw(2) << std::setfill('0') << now->tm_mday << " "
            << std::setw(2) << std::setfill('0') << now->tm_hour << ":"
            << std::setw(2) << std::setfill('0') << now->tm_min << ":"
            << std::setw(2) << std::setfill('0') << now->tm_sec << "]"
            << " ";
        client_msg.insert(0, os_timestr.str());  // 组装消息

        while (true) {
            int transfered_bytes = ::send(client_fd, client_msg.data(), client_msg.size(), 0);
            if (transfered_bytes == -1) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) { // 发送缓冲区已满(对端的接收窗口太小)，sleep one millisecond
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                } else {
                    std::cerr << "send error, The client[" << client_fd << "] connection is to be disconnected" << std::endl;
                    release_client(client_fd);
                    break;
                }
            } else if (transfered_bytes == 0) { // 对端关闭连接
                release_client(client_fd);
                break;
            } else {
                client_msg.erase(0, transfered_bytes); // 清除已经发送出去的数据
                if (client_msg.empty())
                    break;
            }
        }
    }
    return nullptr;
}