#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <functional>
using namespace std;
using json = nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>

#include "group.hpp"
#include "User.hpp"
#include "public.hpp"
// 全局用户对象
User g_user;
// 全局好友对象
vector<User> g_friends;
// 全局群组对象
vector<Group> g_groups;

sem_t sem_rw;
atomic_bool g_isLoginSuccess{false};
// 控制主菜单页面程序
bool isMainMenuRunning = false;

void readTaskHandler(int sockfd);
// 主聊天页面程序
void mainMenu(int sockfd);
// 处理登录响应的业务逻辑
void doLoginResponse(json js);
// 处理注册响应的业务逻辑
void doRegResponse(json js);
// 显示帮助信息
void help(int sockfd, string args);
// 一对一聊天
void chat(int sockfd, string args);
// 添加好友
void addFriend(int sockfd, string args);
// 创建群组
void createGroup(int sockfd, string args);
// 加入群组
void addGroup(int sockfd, string args);
// 群聊
void groupChat(int sockfd, string args);
// 注销
void loginOut(int sockfd, string args);
void readTaskHandler(int sockfd)
{
    while (true)
    {
        char buff[1024] = {0};
        int n = recv(sockfd, buff, sizeof(buff), 0);
        if (n == -1)
        {
            cout << "recv failed" << endl;
            close(sockfd);
            break;
        }
        else if (n == 0)
        {
            cout << "server closed" << endl;
            close(sockfd);
            break;
        }
        //cout << "recv data:" << buff << endl;
        json js = json::parse(buff);
        int msgtype = js["msgid"].get<int>();
        if (msgtype == LOGIN_MSG_ACK)
        {
            doLoginResponse(js); // 处理登录响应的业务逻辑
            sem_post(&sem_rw);   // 通知主线程，登录结果处理完成
        }
        if (msgtype == REG_MSG_ACK)
        {
            doRegResponse(js); // 处理注册响应的业务逻辑
            sem_post(&sem_rw); // 通知主线程，登录结果处理完成
        }
        if(msgtype==ADD_FRIEND_MSG)
        {
            if(js["errno"].get<int>()==0)
            {
                cout<<"add friend success!"<<endl;
            }
            else
            {
                cout<<"add friend failed!"<<endl;
            }
        }
        if(msgtype==CREATE_GROUP_MSG)
        {
            if(js["errno"].get<int>()==0)
            {
                cout<<"create group success! groupid="<<js["groupid"].get<int>()<<endl;
            }
            else
            {
                cout<<"create group failed!"<<endl;
            }
        }
        if(msgtype==ADD_GROUP_MSG)
        {
            if(js["errno"].get<int>()==0)
            {
                cout<<"add group success!"<<endl;
            }
            else
            {
                cout<<"add group failed!"<<endl;
            }
        }
        if(msgtype==ONE_CHAT_MSG)
        {
            cout<<js["id"].get<int>()<<" said to you: "<<js["msg"].get<string>()<<endl;
        }
        if(msgtype==GROUP_CHAT_MSG)
        {
            cout<<js["userid"].get<int>()<<" said to group "<<js["groupid"].get<int>()<<": "<<js["msg"].get<string>()<<endl;
        }
    }
}
// 处理登录响应的业务逻辑
void doLoginResponse(json js)
{
    if (js["errno"].get<int>() != 0)
    {
        cout << "msgid error" << js["errmsg"].get<string>() << endl;
        g_isLoginSuccess = false;
        return;
    }
    g_isLoginSuccess = true;
    g_user.setId(js["id"].get<int>());
    g_user.setName(js["name"].get<string>());
    if (js.contains("friendlist"))
    {
        json friendlist = js["friendlist"];
        g_friends.clear();
        for (auto it = friendlist.begin(); it != friendlist.end(); ++it)
        {
            User frienduser;
            frienduser.setId((*it)["id"].get<int>());
            frienduser.setName((*it)["name"].get<string>());
            frienduser.setState((*it)["state"].get<string>());
            g_friends.push_back(frienduser);
        }
    }
    if (js.contains("grouplist"))
    {
        json group = js["grouplist"];
        g_groups.clear();
        for (auto it = group.begin(); it != group.end(); ++it)
        {
            Group groupuser;
            groupuser.setid((*it)["id"].get<int>());
            groupuser.setName((*it)["name"].get<string>());
            groupuser.setDesc((*it)["desc"].get<string>());
            g_groups.push_back(groupuser);
        }
    }

    cout << "======================login user======================" << endl;
    cout << "current login user => id:" << g_user.getId() << " name:" << g_user.getName() << endl;
    cout << "----------------------friend list---------------------" << endl;
    for (auto it = g_friends.begin(); it != g_friends.end(); ++it)
    {
        cout << "friend id:" << it->getId() << " name:" << it->getName() << " state:" << it->getState() << endl;
    }
    cout << "----------------------group list---------------------" << endl;
    for (auto it = g_groups.begin(); it != g_groups.end(); ++it)
    {
        cout << "group id:" << it->getId() << " name:" << it->getName() << " desc:" << it->getDesc() << endl;
    }
    // 显示当前用户的离线消息  个人聊天信息或者群组消息
    if (js.contains("offlinemsg"))
    {
        json offlinemsg = js["offlinemsg"];

        cout << "------------------offline message-------------------" << endl;
        for (auto it = offlinemsg.begin(); it != offlinemsg.end(); ++it)
        {
            string msg = *it;
            json js = json::parse(msg);
            if (js["msgid"].get<int>() == ONE_CHAT_MSG)
            {
                cout << js["toid"].get<int>() << " said to you: " << js["msg"].get<string>() << endl;
            }
            else if (js["msgid"].get<int>() == GROUP_CHAT_MSG)
            {
                cout << js["userid"].get<int>() << " said to group " << js["groupid"].get<int>() << ": " << js["msg"].get<string>() << endl;
            }
        }
    }
    cout << "----------------------------------------------------" << endl;
}

void doRegResponse(json js)
{
    if (js["errno"].get<int>() != 0)
    {
        cout << "msgid error" << js["errmsg"].get<string>() << endl;
        return;
    }
    cout << "register success!" << endl;
    cout << "======================register user======================" << endl;
    cout << "current register user => id:" << js["id"].get<int>() << " name:" << js["name"].get<string>() << endl;
    cout << "----------------------------------------------------" << endl;
}
unordered_map<string, string> commandMap = {
    {"help", "显示所有支持的命令"},
    {"chat", "一对一聊天"},
    {"addfriend", "添加好友"},
    {"creategroup", "创建群组"},
    {"addgroup", "加入群组"},
    {"groupchat", "群聊"},
    {"loginout", "注销"}};
unordered_map<string, function<void(int, string)>> commandFuncMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addFriend},
    {"creategroup", createGroup},
    {"addgroup", addGroup},
    {"groupchat", groupChat},
    {"loginout", loginOut}};
// 显示帮助信息
void help(int sockfd = -1, string args = "")
{
    cout << "show command list >>> " << endl;
    for (auto &p : commandMap)
    {
        cout << p.first << " : " << p.second << endl;
    }
    cout << endl;
}
void chat(int sockfd, string args) {
    cout << "chat command not implemented yet!" << endl;
    // 这里可以实现一对一聊天的逻辑
    cout<<"friendid :";
    int friendid = -1;
    cin >> friendid;
    cin.get(); // 读掉缓冲区残留的回车
    // 例如发送聊天消息给服务器，服务器再转发给对应的用户
    string msg;
    cout << "message :";
    getline(cin, msg);
    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["toid"] = friendid;
    js["msg"] = msg;
    string request = js.dump();
    int n = send(sockfd, request.c_str(), request.size(), 0);
    if (n == -1)
    {
        cerr << "send chat message failed" << endl;
        return;
    }
}
// 添加好友
void addFriend(int sockfd, string args) {
    cout<< "add friend command not implemented yet!" << endl;
    // 这里可以实现添加好友的逻辑
    cout<<"friendid :";
    int friendid = -1;
    cin >> friendid;
    cin.get(); // 读掉缓冲区残留的回车
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["friendid"] = friendid;
    string request = js.dump();
    int n = send(sockfd, request.c_str(), request.size(), 0);
    if (n == -1)
    {
        cerr << "send add friend message failed" << endl;
        return;
    }
}
// 创建群组
void createGroup(int sockfd, string args) {
    cout << "create group command not implemented yet!" << endl;
    // 这里可以实现创建群组的逻辑
    cout << "group name:";
    string groupName;
    getline(cin, groupName);
    cout << "group description:";
    string groupDesc;
    getline(cin, groupDesc);
    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["name"] = groupName;
    js["desc"] = groupDesc;
    string request = js.dump();
    int n = send(sockfd, request.c_str(), request.size(), 0);
    if (n == -1)
    {
        cerr << "send create group message failed" << endl;
        return;
    }
}
// 加入群组
void addGroup(int sockfd, string args) {
    cout << "add group command not implemented yet!" << endl;
    // 这里可以实现加入群组的逻辑
    cout << "group id:";
    int groupid = -1;
    cin >> groupid;
    cin.get(); // 读掉缓冲区残留的回车
    cout<<"role :1.creator 2.normal "<<endl;
    int role=-1;
    cin >> role;
    cin.get(); // 读掉缓冲区残留的回车
    string roleStr = (role == 1) ? "creator" : "normal";
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["groupid"] = groupid;
    js["role"] = roleStr;
    string request = js.dump();
    int n = send(sockfd, request.c_str(), request.size(), 0);
    if (n == -1)
    {
        cerr << "send add group message failed" << endl;
        return;
    }
}
// 群聊
void groupChat(int sockfd, string args) {
    cout << "group chat command not implemented yet!" << endl;
    // 这里可以实现群聊的逻辑
    cout << "group id:";
    int groupid = -1;
    cin >> groupid;
    cin.get(); // 读掉缓冲区残留的回车
    // 例如发送聊天消息给服务器，服务器再转发给对应的用户
    string msg;
    cout << "message :";
    getline(cin, msg);
    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["groupid"] = groupid;
    js["msg"] = msg;
    string request = js.dump();
    int n = send(sockfd, request.c_str(), request.size(), 0);
    if (n == -1)
    {
        cerr << "send group chat message failed" << endl;
        return;
    }
}
// 注销
void loginOut(int sockfd, string args) {
    cout << "logout command not implemented yet!" << endl;
    // 这里可以实现注销的逻辑
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_user.getId();
    string request = js.dump();
    int n = send(sockfd, request.c_str(), request.size(), 0);
    if (n == -1)
    {
        cerr << "send logout message failed" << endl;
        return;
    }else{
        isMainMenuRunning=false;
        cout<<"logout success!"<<endl;
    }
}
void mainMenu(int sockfd)
{
    help();
    while (isMainMenuRunning)
    {
        string command;
        getline(cin, command);
       auto it = commandFuncMap.find(command);
        if (it != commandFuncMap.end())
        {
            // 执行对应的命令
            it->second(sockfd, "");
        }
        else if (command == "quit")
        {
            cout << "quit main menu" << endl;
            isMainMenuRunning = false;
            break;
        }
        else
        {
            cout << "invalid command, type 'help' to see the command list." << endl;
        }

    }
}
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cout << "Usage: " << argv[0] << " <ip> <port>" << endl;
        return -1;
    }
    string ip = argv[1];
    int port = atoi(argv[2]);
    int cilsockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (cilsockfd == -1)
    {
        cout << "create socket failed" << endl;
        return -1;
    }
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &servaddr.sin_addr);
    if (connect(cilsockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
    {
        cout << "connect server failed" << endl;
        return -1;
    }
    sem_init(&sem_rw, 0, 0);
    thread readTask(readTaskHandler, cilsockfd);
    readTask.detach();
    while (true)
    {
        // 显示首页面菜单 登录、注册、退出
        cout << "========================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "========================" << endl;
        cout << "choice:";
        int choice = 0;
        cin >> choice;
        cin.get(); // 读掉缓冲区残留的回车
        /// cin与getline
        switch (choice)
        {
        case 1:
        {
            json loginJson;
            loginJson["msgid"] = LOGIN_MSG;
            cout << "id:";
            int id = -1;
            cin >> id;
            cin.get(); // 读掉缓冲区残留的回车
            loginJson["id"] = id;
            cout << "name:";
            string name;
            getline(cin, name);
            loginJson["name"] = name;
            cout << "pwd:";
            string pwd;
            getline(cin, pwd);
            loginJson["pwd"] = pwd;
            string loginJsonStr = loginJson.dump();
            int n = send(cilsockfd, loginJsonStr.c_str(), loginJsonStr.size(), 0);
            if (n == -1)
            {
                cout << "send loginJson failed" << endl;
                break;
            }
            sem_wait(&sem_rw);
            if (g_isLoginSuccess)
            {
                isMainMenuRunning = true;
                mainMenu(cilsockfd);
            }
        }
        break;
        case 2:
        {
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "username:";
            cin.getline(name, 50);
            cout << "userpassword:";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["pwd"] = pwd;
            string request = js.dump();
            int len = send(cilsockfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send reg msg error:" << request << endl;
            }

            sem_wait(&sem_rw); // 等待信号量，子线程处理完注册消息会通知
        }
        break;
        case 3:
            cout << "quit" << endl;
            close(cilsockfd);
            return 0;
        default:
            cout << "invalid choice" << endl;
            break;
        }
    }
}
