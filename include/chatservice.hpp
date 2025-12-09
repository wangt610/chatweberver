#ifndef CHATSERVICE_H
#define CHATSERVICE_H
#include<string>
//#include"chatserver.hpp"
#include<unordered_map>
#include<muduo/net/TcpConnection.h>//muduo库中Tcp连接类的头文件
#include<functional>
#include<mutex>
#include<memory>
#include"json.hpp"
#include"public.hpp"
#include<muduo/base/Logging.h>
#include<muduo/base/ThreadPool.h>
#include"chatserver.hpp"

#include"UserModel.hpp"
#include"FriendModel.hpp"
#include"Offlinemessagemodel.hpp"
#include"groupmodel.hpp"

#include"redis.hpp"
using namespace muduo;
using namespace muduo::net;
using namespace std; 
using json=nlohmann::json;  
using MsgHandler=function<void(const TcpConnectionPtr& conn, json& js, Timestamp time)>;
class chatservice
{
    public:
    static chatservice* instance();
    void recvmsg(const TcpConnectionPtr& conn, json& js, Timestamp time);
    //获取消息对应的处理器
    MsgHandler getMsgHandler(int msgid);
    void  sendResponse(const TcpConnectionPtr& conn, const json& js); 
    //处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr& conn);
    //处理登录业务
    void login(const TcpConnectionPtr& conn, json& js, Timestamp time);
    //处理注册业务
    void reg(const TcpConnectionPtr& conn, json& js, Timestamp time);
    //一对一聊天业务
    void oneChat(const TcpConnectionPtr& conn, json& js, Timestamp time);
    //添加好友业务
    void addFriend(const TcpConnectionPtr& conn, json& js, Timestamp time);
    //创建群组业务
    void createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);
    //加入群组业务
    void addGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);
    //群聊天业务
    void groupChat(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // 注销业务
    void loginOut(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // 从redis消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int, string);

    private:
    // static unique_ptr<chatservice> service= make_unique<chatservice>();
    chatservice(){
        // 初始化消息id和对应的处理器
        threadpool_.start(4);
        _msgHandlerMap.insert({LOGIN_MSG, std::bind(&chatservice::login, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
        _msgHandlerMap.insert({REG_MSG, std::bind(&chatservice::reg, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
        _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&chatservice::oneChat, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
        _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&chatservice::addFriend, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
        _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&chatservice::createGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
        _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&chatservice::addGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
        _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&chatservice::groupChat, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
        _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&chatservice::loginOut, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
        // 连接redis服务器
    if (_redis.connect())
    {
        // 设置上报消息的回调
        _redis.init_notify_handler(std::bind(&chatservice::handleRedisSubscribeMessage, this, _1, _2));
    }
    }
    chatservice(const chatservice&)=delete;
    chatservice& operator=(const chatservice&)=delete;
    mutex m_mutex;
     // 存储消息id和其对应的业务处理方法
    unordered_map<int, MsgHandler> _msgHandlerMap;
    unordered_map<int, TcpConnectionPtr> _userConnMap;
    ThreadPool threadpool_;
    UserModel _userModel;
    FriendModel _friendModel;
    Offlinemessagemodel _offlinemessagemodel;
    GroupModel _groupModel;
     // redis操作对象
    Redis _redis;
};
#endif