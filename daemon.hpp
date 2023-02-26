#if !defined(DAEMON_HPP_)
#define DAEMON_HPP_


#include <unistd.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h> // for ::umask

/**
 * 控制终端：
 *      用户登录到bash后，会启动一个shell进程，即建立了一个会话与进程组，并且会连接一个终端(不连接则tty为'?')，该终端就是这个会话的控制终端
 *      一个会话与一个控制终端相对应，控制制度可以向会话的前台进程组发送控制信号
 */

void start_daemon() {
    // 进程组leader不能建立新会话原因：
    // * 创建新会话也会创建新的进程组，创建出的新进程组id、新会话id都应与当前程序pid相同
    //   若进程组leader想要创建新的会话，创建会话就需要创建新的进程组，并且gip和sid要等于pid
    //   但因为是进程组leader，故肯定有一个进程组id与其pid相同，故不能创建出新的进程组(否则会出现两个相同的进程组id)，进而不能创建新的会话
    // * 禁止进程组领导调用setsid()可以防止进程组领导将自己置于新的会话中，而进程组中的其他进程仍然处于原始会话中;
    //   这样的场景将打破严格的会话和进程组的两层层次结构。
    auto pid = fork();
    if (pid == 0) {
        // 创建新会话————创建了新的进程组、会话，脱离了原来的会话和控制终端
        // 若不创建新的会话，虽然主进程直接退出了，子进程在后台运行，看起来满足了需求。
        // 但当控制终端被关闭时，由于当前进程没有脱离该控制终端的控制，故当前进程也会被关闭
        ::setsid();

        /**
            必要的话，可再次创建子进程结束当前进程，使进程不再是会话首进程来禁止进程重新打开控制终端
            pid = fork();
            if (pid > 0) {
                exit(EXIT_SUCCESS);
            }   
        */
        
        // 改变工作目录
        ::chdir("/");
        // 重设文件mask
        ::umask(0);
        // 由于子进程继承了父进程的pcb与打开文件表等信息，故需关闭继承自父进程的IO文件
        for (int i = 0; i < ::getdtablesize(); i++) {
            ::close(i);
            // 或dup2()指向/dev/null
        }

    } else if (pid > 0) {
        exit(EXIT_SUCCESS);
    } else {
        perror("set new session failure");
    }
}

#endif // DAEMON_HPP_
