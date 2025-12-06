#include "chatservice.hpp"
// unique_ptr<chatservice> chatservice::service = make_unique<chatservice>();
mutex mutex1;

chatservice *chatservice::instance()
{
    lock_guard<mutex> lock(mutex1);
    static chatservice service;
    return &service;
}

void chatservice::recvmsg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // 获取消息id
    int msgid = js["msgid"].get<int>();
    // 根据消息id获取对应的处理器
    auto msgHandler = getMsgHandler(msgid);
    // 调用消息处理器，进行业务处理
    threadpool_.run(std::bind(msgHandler, conn, js, time));
}

MsgHandler chatservice::getMsgHandler(int msgid)
{
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        // 返回一个默认的处理器，空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time)
        {
            LOG_ERROR << "msgid:" << msgid << " no handler!";
        };
    }
    return _msgHandlerMap[msgid];
}

void chatservice::sendResponse(const TcpConnectionPtr &conn, const json &js)
{
    try
    {
        if (conn->connected())
        {
            string response = js.dump();
            conn->getLoop()->runInLoop([conn, response]()
                                       { conn->send(response); });
        }
    }
    catch (const exception &e)
    {
        LOG_ERROR << "Send response exception: " << e.what();
        // 异常处理：通知客户端错误、清理资源等
        if (conn->connected())
        {
            conn->getLoop()->runInLoop([conn]()
                                       { conn->send("Server internal error!"); });
        }
    }
}

void chatservice::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(m_mutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                // 找到了对应的用户连接
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }
    // 更新用户的状态信息
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

void chatservice::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["pwd"];
    int id = js["id"].get<int>();
    User user = _userModel.query(id);
    if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        {
            // 该用户已经登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this account is using, input another!";
            sendResponse(conn, response);
        }
        else
        {
            // 登录成功，记录用户连接信息
            {
                lock_guard<mutex> lock(m_mutex);
                _userConnMap.insert({id, conn});
            }
        }
        // 更新用户状态信息
        user.setState("online");
        _userModel.updateState(user);
        // 登录成功
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        response["name"] = user.getName();
        // 查询该用户是否有离线消息
        vector<string> offlinemsg = _offlinemessagemodel.query(id);
        json offline;
        while(!offlinemsg.empty())
        {
           offline.push_back(offlinemsg.back());
           offlinemsg.pop_back();
        }
        // 将离线消息添加到响应中
        if (!offline.empty())
        {
        response["offlinemsg"] = offline;
        }
        // 查询该用户的好友列表
        vector<User> friends = _friendModel.query(id);
        json friendList;
        for (auto &friendUser : friends)
        {
            json friendJson;
            friendJson["id"] = friendUser.getId();
            friendJson["name"] = friendUser.getName();
            friendJson["state"] = friendUser.getState();
            friendList.push_back(friendJson);
        }
        if (!friendList.empty())
        {
            response["friendlist"] = friendList;
        }
        // 查询该用户的群组列表
        vector<Group> groups = _groupModel.queryGroups(id);
        json groupList;
        for (auto &group : groups)
        {
            json groupJson;
            groupJson["id"] = group.getId();
            groupJson["name"] = group.getName();
            groupJson["desc"] = group.getDesc();
            groupList.push_back(groupJson);
        }
        if (!groupList.empty())
        {
            response["grouplist"] = groupList;
        }
        sendResponse(conn, response);
        _offlinemessagemodel.remove(id);
    }
    else
    {
        // 登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "id or password is invalid!";
        sendResponse(conn, response);
    }
}

void chatservice::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["pwd"];
    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = false;
    _userModel.insert(user);
    if (user.getId() != -1)
    {
        // 注册成功
        state = true;
    }
    // 回复注册结果
    json response;
    response["msgid"] = REG_MSG_ACK;
    if (state)
    {
        response["errno"] = 0;
        response["id"] = user.getId();
    }
    else
    {
        response["errno"] = 1;
        response["errmsg"] = "register error!";
    }
    sendResponse(conn, response);
}

void chatservice::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["toid"].get<int>();
    bool status = false;
    // 查询toid用户是否在线
    {
        lock_guard<mutex> lock(m_mutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            // toid在线，转发消息
            auto toConn = it->second;
            sendResponse(toConn, js);
            return;
        }
    }
    // toid不在线，存储离线消息
    // 省略
    _offlinemessagemodel.insert(toid, js.dump());

}

void chatservice::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid=-1;
    {
        lock_guard<mutex> lock(m_mutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                // 找到了对应的用户连接
                userid=it->first;
                break;
            }
        }
    }
    if(userid==-1){
        // 用户未登录，无法添加好友
        json response;
        response["msgid"] = ADD_FRIEND_MSG;
        response["errno"] = 2;
        response["errmsg"] = "user not logged in!";
        sendResponse(conn, response);
        return;
    }
    int friendid = js["friendid"].get<int>();
    // 存储好友信息
    bool status = _friendModel.insert(userid, friendid);
    if (status)
    {
        // 添加成功
        json response;
        response["msgid"] = ADD_FRIEND_MSG;
        response["errno"] = 0;
        sendResponse(conn, response);
    }
    else
    {
        // 添加失败
        json response;
        response["msgid"] = ADD_FRIEND_MSG;
        response["errno"] = 1;
        response["errmsg"] = "add friend error!";
        sendResponse(conn, response);
    }
}

void chatservice::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    Group group;
    group.setName(js["groupname"]);
    group.setDesc(js["groupdesc"]);
    bool state = _groupModel.createGroup(group);
    if(state)
    {
        // 创建群组成功
        json response;
        response["msgid"] = CREATE_GROUP_MSG;
        response["errno"] = 0;
        response["groupid"] = group.getId();
        sendResponse(conn, response);
    }
    else
    {
        // 创建群组失败
        json response;
        response["msgid"] = CREATE_GROUP_MSG;
        response["errno"] = 1;
        response["errmsg"] = "create group error!";
        sendResponse(conn, response);
    }
}

void chatservice::addGroup(const TcpConnectionPtr & conn, json & js, Timestamp time)
{
    int userid=-1;
    {
        lock_guard<mutex> lock(m_mutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                // 找到了对应的用户连接
                userid=it->first;
                break;
            }
        }
    }
    if(userid==-1){
        // 用户未登录，无法加入群组
        json response;
        response["msgid"] = ADD_GROUP_MSG;
        response["errno"] = 2;
        response["errmsg"] = "user not logged in!";
        sendResponse(conn, response);
        return;
    }
    int groupid = js["groupid"].get<int>();
    string role = js["role"];
    _groupModel.addGroup(userid, groupid, role);
}

void chatservice::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid=-1;
    {
        lock_guard<mutex> lock(m_mutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                // 找到了对应的用户连接
                userid=it->first;
                break;
            }
        }
    }
    if(userid==-1){
        // 用户未登录，无法进行群聊
        json response;
        response["msgid"] = GROUP_CHAT_MSG;
        response["errno"] = 2;
        response["errmsg"] = "user not logged in!";
        sendResponse(conn, response);
        return;
    }
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);
    lock_guard<mutex> lock(m_mutex);
    for (int id : useridVec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            // 群成员在线，转发消息
            auto toConn = it->second;
            sendResponse(toConn, js);
        }
        else
        {
            // 群成员不在线，存储离线消息
            _offlinemessagemodel.insert(id, js.dump());
        }
    }
}
