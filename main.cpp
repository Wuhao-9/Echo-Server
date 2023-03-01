#include "Reactor.hpp"
#include "Acceptor.hpp"
#include "EchoHandler.hpp"
int main() {
    Reactor reactor_instance;
    Acceptor* acceptor = new Acceptor("127.0.0.1", 1993, reactor_instance);
    reactor_instance.register_handler(acceptor, EPOLLIN);
    reactor_instance.dispatch();
}