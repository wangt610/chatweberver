#include "chatserver.hpp"
int main(int argc,char*argv[]){
    if(argc<3){
        std::cerr<<"Usage: "<<argv[0]<<" <ip> <port>"<<std::endl;
        return 1;
    }
    EventLoop loop;
    InetAddress addr(argv[1], atoi(argv[2]));
    ChatServer server(&loop, addr, "ChatServer");
    server.start();
    loop.loop();
    return 0;
}
