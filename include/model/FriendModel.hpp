#ifndef FRIENDMODEL_HPP
#define FRIENDMODEL_HPP
#include"db.h"
#include"User.hpp"
#include<vector>
using namespace std;
//好友关系表的数据操作类
class FriendModel
{
    public:
    //添加好友关系
    bool insert(int userid,int friendid);
    //返回用户的好友列表
    vector<User> query(int userid);
};
#endif