#include "Cache.hpp"
#include <pthread.h>
pthread_mutex_t cache_lock;

void Cache::put(std::string key ,std::vector<char> val) {
    pthread_mutex_lock(&cache_lock);
    cash.insert(std::pair<std::string, std::vector<char>>(key, val));
    pthread_mutex_unlock(&cache_lock);
}

std::vector<char>& Cache::get(const std::string& key) {
    pthread_mutex_lock(&cache_lock);
    std::map<std::string, std::vector<char>>::iterator iter = cash.find(key);
    if (iter == cash.end()) {
        pthread_mutex_unlock(&cache_lock);
        throw std::out_of_range("no such key stored in map");
    }
    pthread_mutex_unlock(&cache_lock);
    return iter->second;
}

void Cache::remove(const std::string& key) {
    pthread_mutex_lock(&cache_lock);
    cash.erase(key);
    pthread_mutex_unlock(&cache_lock);
}

bool Cache::find(const std::string& key){
    pthread_mutex_lock(&cache_lock);
    std::map<std::string, std::vector<char>>::iterator iter = cash.find(key);
    if (iter == cash.end()) {
        pthread_mutex_unlock(&cache_lock);
       return false;
    }
    pthread_mutex_unlock(&cache_lock);
    return true;
}
/* response need to revalidate, if it has etag or last modified header field,
 * send the ask revalidation request to the server, else send the original
 * request to the server */


std::string Cache::revalidate(ResponseMeta val, RequestMeta req_val){
    std::pair<bool, std::string> etag = val.getEtag();
    std::pair<bool, std::string> lastModified = val.getLastModified();
    std::string newRequest;
    std::string head = req_val.getHead();
    if(etag.first){
        newRequest = head + "\r\n" + "If-None-Match: "+ etag.second + "\r\n\r\n";
    }
    else if(lastModified.first){
        newRequest = head + "\r\n" + "If-Modified-Since: "+ lastModified.second + "\r\n\r\n";
    }
    else{
        newRequest = head;
    }
    return newRequest;
}

/* check if the response can be stored in the cache */

bool Cache::store_response(std::vector<char> resp) {
    ResponseMeta response = HttpParser::parseRespHeader(resp);
    size_t f = response.getFirstLine().find("HTTP/1.1 200 OK\r\n\r\n");
    if (f != std::string::npos) {
        bool whether_have_cache_control = response.getCacheControl().first;
        //have cache control
        if (whether_have_cache_control) {

            if (response.isNoStore()) {
                log_info("not cacheable because Cache-Control : no-store");
                return false;
            } else if (response.isPrivate()) {
                log_info("not cacheable because Cache-Control : is-private");
                return false;
            } else {
                // TODO: check if no cache and print log
                if (response.isNoCache()) {
                    log_info("cached, but requires re-validation");
                }
                return true;
            }
        }

        else {
            return true;
        }
    }
    else{
        return false;
    }
}
