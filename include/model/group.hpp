#ifndef GROUP_HPP
#define GROUP_HPP
#include"groupuser.hpp"
#include <string>
#include <vector>
using namespace std;
class Group
{
public:
    Group(int groupId = -1, const string &groupName = "", const string &groupDesc = "")
        : id(groupId), name(groupName), desc(groupDesc)
    {
    }
    int getId() const { return id; }
    string getName() const { return name; }
    string getDesc() const { return desc; }
    vector<int>& getUsers() { return user; }
    void setName(const string &groupName) { name = groupName; }
    void setDesc(const string &groupDesc) { desc = groupDesc; }
    void setid(int groupId) { id = groupId; }
    
private:
    int id;
    string name;
    string desc;
    vector<int> user;
};
#endif