#pragma once

#include <string>
#include <vector>
#include <map>

typedef std::vector<std::string> List;

List merge(const List &_A, const List &_B);
void add(const std::string &_str, List &_dst);
void del(const std::string &_str, List &_dst);

typedef std::map<std::string,std::string> DefinesList;
typedef std::map<std::string,std::string>::iterator DefinesList_it;
typedef std::map<std::string,std::string>::const_iterator DefinesList_cit;
bool merge(const DefinesList &_src, DefinesList &_dst);
