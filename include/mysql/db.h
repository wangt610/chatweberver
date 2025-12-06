#ifndef DB_H
#define DB_H
#include <mysql/mysql.h>
#include <string>
#include <iostream>
#include<muduo/base/Logging.h>
using namespace std;
class DB
{
public:
    DB();
    ~DB();
    bool connect();
    bool update(string sql);
    MYSQL_RES *query(string sql);
    bool startTransaction();
    bool commit();
    bool rollback();
private:
    MYSQL *conn;
};
#endif