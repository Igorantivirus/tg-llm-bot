#pragma once

#include <string>

namespace config
{
struct Command
{
    std::string command;
    std::string description;
};

struct AllCommands
{
    Command clear;
    Command stop;
    Command stop_all;
    Command system;
    Command model;
    Command models;
    Command make_admin;
    Command remove_admin;
    Command ban_user;
    Command unban_user;
    Command add_group;
    Command remove_group;
    Command add_chat;
    Command remove_chat;
};

} // namespace config