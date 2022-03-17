

#ifndef INC_568_RESPONSE_HPP
#define INC_568_RESPONSE_HPP

#include <string>
#include "Util.hpp"


class ResponseMeta
{
private:
    std::string res_head;
    std::string FirstLine;
    std::string Date;
    std::pair<bool, std::string> Expires;
    std::pair<bool, std::string> LastModified;
    std::pair<bool, std::string> Etag;
    std::pair<bool, std::string> Age;

    std::pair<bool, std::string> CacheControl;
    std::pair<bool, std::string> TransferEncoding;



public:
    ResponseMeta(std::string rh, std::string fl, std::string d, std::pair<bool, std::string> exp,
                 std::pair<bool, std::string> lm, std::pair<bool, std::string> etag, std::pair<bool, std::string> age,
                 std::pair<bool, std::string> cc, std::pair<bool, std::string> te);

    const std::string& getFirstLine();
    const std::string& getDate();
    const std::pair<bool, std::string>& getExpires();
    const std::pair<bool, std::string>& getLastModified();
    const std::pair<bool, std::string>& getEtag();
    const std::pair<bool, std::string>& getAge();
    const std::pair<bool, std::string>& getTransferEncoding();
    const std::pair<bool, std::string>& getCacheControl();

    const bool checkCacheControl(std::string toCheck);
    const bool isNoCache();
    const bool isMustRevalidate();
    const bool isPrivate();
    const bool isNoStore();
    const std::pair<bool, std::string> getMaxAge();
    const std::pair<bool, std::string> get_sMaxAge();
    const bool if_fresh();

    





    ~ResponseMeta();

};

#endif //INC_568_RESPONSE_HPP
