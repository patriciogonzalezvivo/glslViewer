#include "list.h"

#include <algorithm>    // std::sort

List merge(const List &_A,const List &_B) {
    List rta;
    
    std::merge( _A.begin(), _A.end(),
                _B.begin(), _B.end(),
                std::back_inserter(rta) );

    std::sort( rta.begin(), rta.end() );

    rta.erase(std::unique(rta.begin(), rta.end()), rta.end());    

    return rta;
}

void add(const std::string &_str, List &_list) {
    _list.push_back(_str);
    std::sort( _list.begin(), _list.end() );
    _list.erase(std::unique(_list.begin(), _list.end()), _list.end());    
}

void del(const std::string &_str, List &_list) {
    for (unsigned int i = _list.size() - 1; i >= 0 ; i--) {
        if ( _list[i] == _str ) {
            _list.erase(_list.begin() + i);
        }
    }
}

bool merge(const DefinesList &_src, DefinesList &_dst) {
    bool change = false;
    for (DefinesList_cit it=_src.begin(); it!=_src.end(); ++it) {
        // If it doesn't exist 
        if ( _dst.find(it->first) == _dst.end() ) {
            // add it
            _dst[it->first] = it->second;
            change = true;
        }
        // If exist and is different
        else if ( _dst[it->first] != it->second) {
            // change it
            _dst[it->first] = it->second;
            change = true;
        }
    }

    return change;
}