#pragma once

#include <list>
#include <iterator>

using Pixels        = std::unique_ptr<unsigned char[]>;

class LockFreeQueue {
public:

    LockFreeQueue() {
        // Add one to validate the iterators
        m_list.push_back( Pixels() );
        m_headIt = m_list.begin();
        m_tailIt = m_list.end();
    }

    void produce( std::unique_ptr<unsigned char[]>&& t ) {
        m_list.push_back( std::move(t) );
        m_tailIt = m_list.end();
        m_list.erase( m_list.begin(), m_headIt );
    }

    bool consume( std::unique_ptr<unsigned char[]>&& t ) {
        typename PixelList::iterator nextIt = m_headIt;
        ++nextIt;
        if ( nextIt != m_tailIt ) {
            m_headIt = nextIt;
            t        = std::move( *m_headIt );
            return true;
        }

        return false;
    }

    int size() const { return std::distance( m_headIt, m_tailIt ) - 1; }
    typename std::list<Pixels>::iterator getHead() const { return m_headIt; }
    typename std::list<Pixels>::iterator getTail() const { return m_tailIt; }

private:
    using PixelList = std::list<Pixels>;
    PixelList                       m_list;
    typename PixelList::iterator    m_headIt, m_tailIt;
};
