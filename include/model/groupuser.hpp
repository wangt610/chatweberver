#ifndef GROUPUSER_HPP
#define GROUPUSER_HPP
#include"User.hpp"
#include"db.h"
class GroupUser : public User
{
    private:
        string role;
    public:
        void setRole(string role) { this->role = role; }
        string getRole() { return this->role; }
};
#endif