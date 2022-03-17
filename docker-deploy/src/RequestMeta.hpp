#ifndef __REQUEST_META_HPP_
#define __REQUEST_META_HPP_

#include <string>
#include <sstream>
#include "Util.hpp"
#include "RequestType.hpp"

class RequestMeta {
private:
    RequestType req_t;
    std::string url;
    size_t content_length;
    std::string host;
    uint16_t port;
    std::string FirstLine;


    int max_age;
    int max_stale;
    int min_fresh;

    bool is_no_cache;
    bool is_no_store;
    bool is_only_if_cached;

    std::string req_head;
    
public:
    const static int UNSPECIFIED = -1;

    RequestMeta(RequestType rt, std::string u, size_t l, std::string host, uint16_t port, std::string FirstLine, int max_age, int max_stale, int min_fresh, bool is_no_cache, bool is_no_store, bool is_only_if_cached, std::string req_head);
    std::string toString() const;
    const std::string& getUrl() const;
    const RequestType& getRequestType() const;
    const size_t& getContentLength() const;
    const std::string& getHost() const;
    const std::string& getHead() const;
    const uint16_t getPort() const;
    const std::string getFirstLine() const;

    int getMaxAge() const;
    int getMaxStale() const;
    int getMinFresh() const;
    bool isNoCache() const;
    bool isNoStore() const;
    bool isOnlyIfCached() const;



    ~RequestMeta();
};
#endif
