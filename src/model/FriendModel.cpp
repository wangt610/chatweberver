#include "FriendModel.hpp"

bool FriendModel::insert(int userid, int friendid)
{
    DB db;
    if(db.connect())
    {
        db.startTransaction();
        string sql = "INSERT INTO Friend(userid, friendid) VALUES(" + to_string(userid) + "," + to_string(friendid) + ");";
        if(db.update(sql)){
            db.commit();
        } else {
            db.rollback();
            return false;
        }
    }
    return true;
}

vector<User> FriendModel::query(int userid)
{
    vector<User> userVec;
    DB db;
    if(db.connect())
    {
        db.startTransaction();
        // 查询好友列表
        // 假设好友关系表名为Friend，包含userid和friendid字段
        // User表包含id和name字段
        // 这里使用INNER JOIN来获取好友的详细信息
        string sql = "SELECT a.id, a.name FROM User a INNER JOIN Friend b ON a.id = b.friendid WHERE b.userid = " + to_string(userid) + ";";
        MYSQL_RES *res = db.query(sql);
        if(res)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                User user(atoi(row[0]), row[1]);
                userVec.push_back(user);
            }
            mysql_free_result(res);
            db.commit();
        }
        else
        {
            db.rollback();
        }
    }
    return userVec;
}
