#include "groupmodel.hpp"

bool GroupModel::createGroup(Group &group)
{
   DB db;
   if(db.connect()){
    db.startTransaction();
    string sql="insert into AllGroup(groupname,groupdesc) values('"+group.getName()+"','"+group.getDesc()+"')";
    if(db.update(sql)){
       MYSQL_RES *res=db.query("select last_insert_id()");
         if(res!=nullptr){
            MYSQL_ROW row=mysql_fetch_row(res);
            if(row!=nullptr){
                group.setid(atoi(row[0]));
                mysql_free_result(res);
                db.commit();
                return true;
            }
         }
    }
    db.rollback();
   }
   return false;
}

void GroupModel::addGroup(int userid, int groupid, string role)
{
    DB db;
    if(db.connect()){
        db.startTransaction();
        string sql="insert into GroupUser values("+to_string(groupid)+","+to_string(userid)+",'"+role+"')";
        if(db.update(sql)){
            db.commit();
        }
        else{
            db.rollback();
        }   
    }
}

vector<Group> GroupModel::queryGroups(int userid)
{
    vector<Group> groupVec;
    DB db;
    if(db.connect()){
        string sql="select a.id,a.groupname,a.groupdesc from AllGroup a inner join GroupUser b on a.id=b.groupid where b.userid="+to_string(userid);
        MYSQL_RES *res=db.query(sql);
        if(res!=nullptr){
            MYSQL_ROW row;
            while((row=mysql_fetch_row(res))!=nullptr){
                Group group;
                group.setid(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groupVec.push_back(group);
            }
            mysql_free_result(res);
        }
    }
    return groupVec;
}

vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    vector<int> useridVec;
    DB db;
    if(db.connect()){
        string sql="select userid from GroupUser where groupid="+to_string(groupid)+" and userid!="+to_string(userid);
        MYSQL_RES *res=db.query(sql);
        if(res!=nullptr){
            MYSQL_ROW row;
            while((row=mysql_fetch_row(res))!=nullptr){
                useridVec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return useridVec;
}

