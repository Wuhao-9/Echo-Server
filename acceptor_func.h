#if !defined(ACCEPTOR_H_)
#define ACCEPTOR_H_

#include <pthread.h>

namespace acceptor_util {
    extern ::pthread_cond_t acceptor_cond; // declaration
    extern ::pthread_mutex_t acceptor_mutex;
}

namespace initialized_instance {
    extern int listener_fd;
    extern int epoll_fd;
}

namespace sync_util {
    extern bool thread_stop;
}

void* accept_thread_func(void* arg);

#endif // ACCEPTOR_H_
