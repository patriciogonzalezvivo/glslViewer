#pragma once

#include <string>
#include <vector>

typedef std::vector<std::string> List;

List merge(const List &_A,const List &_B);
void add(const std::string &_str, List &_list);
void del(const std::string &_str, List &_list);