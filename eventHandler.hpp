// 事件处理器接口
// 定义事件处理器接口
#if !defined(EVENTHANDLER_HPP_)
#define EVENTHANDLER_HPP_

class EventHandler {
public:
    virtual void handleRead() = 0; // 处理读事件
    virtual void handleWrite() = 0; // 处理写事件
    virtual int getFd() = 0; // 获取文件描述符
};

#endif // EVENTHANDLER_HPP_
