#ifndef __CACHE_HPP__
#define __CACHE_HPP__
#include <map>
#include "Util.hpp"
#include "ResponseMeta.hpp"
#include "RequestMeta.hpp"
#include "HttpParser.hpp"
#include "assert.h"

class Cache {
private:
    std::map<std::string, std::vector<char>> cash;

public:
    Cache() {}

    ~Cache() {}

    void put(std::string key, std::vector<char> resp);
    std::vector<char> &get(const std::string &key);
    void remove(const std::string &key);

    bool find(const std::string &key);
    std::string revalidate(ResponseMeta val, RequestMeta req_val);
    bool store_response(std::vector<char> resp);

};

#endif