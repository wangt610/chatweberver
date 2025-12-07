#include "chatserver.hpp"
int main(){
    EventLoop loop;
    InetAddress addr("192.168.1.242", 6000);
    ChatServer server(&loop, addr, "ChatServer");
    server.start();
    loop.loop();
    return 0;
}
