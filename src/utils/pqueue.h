#pragma once

#include <queue>
#include <vector>

using std::priority_queue;
using std::vector;
using std::greater;
using std::size_t;

template <class T>
class PQueue: public priority_queue<T, vector<T>, greater<T>>{
public:
    public:
    typedef typename
        std::priority_queue<T>::container_type::iterator iterator;
    typedef typename
        std::priority_queue<T>::container_type::const_iterator const_iterator;

    iterator begin() {
        return this->c.begin();
    }
    iterator end() {
        return this->c.end();
    }
    const_iterator cbegin() const {
        return this->c.cbegin();
    }
    const_iterator cend() const {
        return this->c.cend();
    }
    size_t size() const {
        return this->c.size();
    }
};
