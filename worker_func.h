#if !defined(WORKER_FUNC_H)
#define WORKER_FUNC_H

#include <list>
#include <pthread.h>

extern std::list<int> client_queue;

namespace initialized_instance {
    extern int epoll_fd;
}

namespace sync_util {
    extern bool thread_stop;
    extern ::pthread_mutex_t client_mutex;
    extern ::pthread_cond_t client_cond;
}

void* wroker_func(void * args);
void release_client(int client_fd);

#endif // WORKER_FUNC_H
