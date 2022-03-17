#include "RequestMeta.hpp"

RequestMeta::RequestMeta(RequestType rt, std::string u, size_t l,
    std::string host, uint16_t port,std::string FirstLine, int max_age,
    int max_stale, int min_fresh, bool is_no_cache, bool is_no_store, bool is_only_if_cached, std::string rh):
    req_t(rt), url(u), content_length(l), host(host), port(port), FirstLine(FirstLine), max_age(max_age),
    min_fresh(min_fresh), is_no_cache(is_no_cache), is_no_store(is_no_store), is_only_if_cached(is_only_if_cached), req_head(rh) {}

RequestMeta::~RequestMeta() {}
std::string RequestMeta::toString() const {
    std::stringstream ss;
    ss << "Method: ";
    ss << req_type_repr(this->req_t) << ", ";
    ss << "URL: " << this->url << ", ";
    ss << "Content-Length: " << this->content_length << ", ";
    ss << "Host: " << this->host << ", ";
    ss << "Port: " << this->port << std::endl;

    return ss.str();
}

const std::string& RequestMeta::getUrl() const {
    return this->url;
}

const RequestType& RequestMeta::getRequestType() const {
    return this->req_t;
}

const size_t& RequestMeta::getContentLength() const {
    return this->content_length;
}

const std::string& RequestMeta::getHost() const {
    return this->host;
}

const uint16_t RequestMeta::getPort() const {
    return this->port;
}

const std::string RequestMeta::getFirstLine() const {
    return this->FirstLine;
}

int RequestMeta::getMaxAge() const {
    return this->max_age;
}

int RequestMeta::getMaxStale() const {
    return this->max_stale;
}

int RequestMeta::getMinFresh() const {
    return this->min_fresh;
}

bool RequestMeta::isNoCache() const {
    return this->is_no_cache;
}

bool RequestMeta::isNoStore() const {
    return this->is_no_store;
}

bool RequestMeta::isOnlyIfCached() const {
    return this->is_only_if_cached;
}

const std::string& RequestMeta::getHead() const {
    return this->req_head;
}
