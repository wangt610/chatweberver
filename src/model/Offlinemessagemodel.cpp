#include "Offlinemessagemodel.hpp"
void Offlinemessagemodel::insert(int userid, string msg)
{
    DB db;
    if(db.connect())
    {
        db.startTransaction();
        string sql = "INSERT INTO OfflineMessage(userid, message) VALUES(" + to_string(userid) + ", '" + msg + "');";
       if(db.update(sql)) {
           db.commit();
       } else {
           db.rollback();
       }
    }
}

void Offlinemessagemodel::remove(int userid)
{
    DB db;
    if(db.connect())
    {
        db.startTransaction();
        string sql = "DELETE FROM OfflineMessage WHERE userid = " + to_string(userid) + ";";
       if(db.update(sql)) {
           db.commit();
       } else {
           db.rollback();
       }
    }
}

vector<string> Offlinemessagemodel::query(int userid)
{
    DB db;
    vector<string> messages;
    if(db.connect())
    {
        string sql = "SELECT message FROM OfflineMessage WHERE userid = " + to_string(userid) + ";";    
        MYSQL_RES *res = db.query(sql);
        if(res)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)  
            {
                messages.push_back(row[0]);
            }
            mysql_free_result(res);  
        }
    }
    return messages;
}
