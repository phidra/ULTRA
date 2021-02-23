#pragma once

#include <string>

struct Error400 : public std::exception {
    std::string msg;
    Error400(std::string const& msg_) : msg{std::string("ERROR 400> ") + msg_} {}
    inline const char* what() const throw() { return msg.c_str(); }
};
