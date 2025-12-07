#include "db.h"
string db_ip = "127.0.0.1";
unsigned int db_port = 3306;
string db_user = "root";    
string db_password = "20050610";
string db_name = "chatserver";
// 构造函数 
DB::DB()
{
    // 初始化连接对象
    conn = mysql_init(nullptr);
}
DB::~DB()
{
    // 关闭连接
    if (conn != nullptr)
    {
        mysql_close(conn);
    }
}
// 连接数据库
bool DB::connect()
{
    // 连接数据库
   MYSQL*p=mysql_real_connect(conn, db_ip.c_str(), db_user.c_str(), db_password.c_str(), db_name.c_str(), db_port, nullptr, 0);
    if(p==nullptr){
        LOG_INFO<<"mysql connect error!" << mysql_error(conn);
        return false;
    }
    LOG_INFO<<"mysql connect success!";
    return true;
}
// 更新数据
bool DB::update(string sql)
{
    if(mysql_query(conn, sql.c_str()))
    {
        LOG_INFO<<"mysql update error!" << mysql_error(conn);
        return false;
    }
    return true;
}
// 查询数据
MYSQL_RES* DB::query(string sql)
{
    if(mysql_query(conn, sql.c_str()))
    {
        LOG_INFO<<"mysql query error!" << mysql_error(conn);
        return nullptr;
    }
    return mysql_store_result(conn);
}
// 开启事务
bool DB::startTransaction()
{
    return mysql_autocommit(conn, 0) == 0;
}
// 提交事务
bool DB::commit()
{ 
      return mysql_commit(conn) == 0;  
}
// 回滚事务
bool DB::rollback()
{
    return mysql_rollback(conn) == 0;
}