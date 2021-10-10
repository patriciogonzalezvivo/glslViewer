#include "list.h"
#include <iterator>
#include <algorithm>

List merge(const List &_A,const List &_B) {

#ifdef PLATFORM_WINDOWS
    // std::merge on Windows would expect _A and _B to be already sorted to work
    List rta(_A);

    rta.insert(rta.begin(), _B.begin(), _B.end());      // rta = _A + _B
    rta.erase(std::unique(rta.begin(), rta.end()), rta.end());  // remove duplicates
    std::sort(rta.begin(), rta.end());  // sort the resulting List
#else
    List rta;
    
    std::merge( _A.begin(), _A.end(),
                _B.begin(), _B.end(),
                std::back_inserter(rta) );

    std::sort( rta.begin(), rta.end() );

    rta.erase(std::unique(rta.begin(), rta.end()), rta.end());    
#endif
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
