#include "ResponseMeta.hpp"
#include <time.h>

ResponseMeta::ResponseMeta(std::string rh, std::string fl, std::string d,
                           std::pair<bool, std::string> exp,
                           std::pair<bool, std::string> lm, std::pair<bool, std::string> etag,
                           std::pair<bool, std::string> age, std::pair<bool, std::string> cc,
                           std::pair<bool, std::string> te) : res_head(rh),
                                                               FirstLine(fl),
                                                               Date(d),
                                                               Expires(exp),
                                                               LastModified(lm),
                                                               Etag(etag),
                                                               Age(age),
                                                               CacheControl(cc),
                                                               TransferEncoding(
                                                                       te) {}

ResponseMeta::~ResponseMeta() {}
const std::string& ResponseMeta::getFirstLine(){
    return this->FirstLine;
}
const std::string& ResponseMeta::getDate(){
    return this->Date;
}
const std::pair<bool, std::string>& ResponseMeta::getExpires(){
    return this->Expires;
}
const std::pair<bool, std::string>& ResponseMeta::getLastModified(){
    return this->LastModified;
}
const std::pair<bool, std::string>& ResponseMeta::getEtag() {
    return this->Etag;
}
const std::pair<bool, std::string>& ResponseMeta::getTransferEncoding() {
    return this->TransferEncoding;
}
const std::pair<bool, std::string>& ResponseMeta::getAge() {
    return this->Age;
}
const std::pair<bool, std::string>& ResponseMeta::getCacheControl(){
    return this->CacheControl;
}

const bool ResponseMeta::checkCacheControl(std::string toCheck){
    std::pair<bool, std::string> cc = getCacheControl();
    if(cc.first){
        size_t f = cc.second.find(toCheck);
        if(f != std::string::npos){
          return true;
        }
    }
    return false;
}

const bool ResponseMeta::isNoCache(){
    return checkCacheControl("no-cache");
}
const bool ResponseMeta::isMustRevalidate(){
    return checkCacheControl("must-revalidate");
}
const bool ResponseMeta::isNoStore(){
    return checkCacheControl("no-store");
}
const bool ResponseMeta::isPrivate(){
    return checkCacheControl("private");
}

const std::pair<bool, std::string> ResponseMeta::getMaxAge(){
    std::pair<bool, std::string> cc = getCacheControl();
    if(cc.first){
        size_t f1 = cc.second.find("max-age=");
        if(f1 != std::string::npos){
            size_t f2 = cc.second.find(",", f1+1);
            if(f2 == std::string::npos){
                f2 = cc.second.find("\r\n",f1+1);
            }
            std::string ans = cc.second.substr(f1+8,f2-f1-8);
            return  std::pair<bool, std::string>(true, ans);
        }
    }
    return std::pair<bool, std::string>(false, "");
   }

const std::pair<bool, std::string> ResponseMeta::get_sMaxAge(){
    std::pair<bool, std::string> cc = getCacheControl();
    if(cc.first){
        size_t f1 = cc.second.find("s-maxage=");
        if(f1 != std::string::npos){
            size_t f2 = cc.second.find(",", f1+1);
            if(f2 == std::string::npos){
                f2 = cc.second.find("\r\n",f1+1);
            }
            std::string ans = cc.second.substr(f1+9,f2-f1-9);
            return  std::pair<bool, std::string>(true, ans);
        }
    }
    return std::pair<bool, std::string>(false, "");
}



//is fresh: return true
const bool ResponseMeta::if_fresh(){
    //age field 不存在？
    int age = std::stoi(getAge().second);
    double fresh_lifetime;
    //s-maxage
    if(get_sMaxAge().first){
      fresh_lifetime = std::stoi(get_sMaxAge().second);
     }
    //max-age
    else if(getMaxAge().first){
     fresh_lifetime = std::stoi(getMaxAge().second);
    }
    //expires
    else if(getExpires().first){
    fresh_lifetime = std::stoi(getExpires().second);
    }
    //last modified exist
    else if(getLastModified().first){
        time_t date = convertToTime(getDate());
        time_t lastModified = convertToTime(getLastModified().second);
        fresh_lifetime = difftime(date,lastModified)/10;
    }
    //none of it exist
    else{
        return false;
    }
    if(fresh_lifetime > age){
        return true;
    }
    else{
        // TODO
        log_info("in cache, but expired at ");
        return false;
    }


}











