#ifndef __HTTP_PARSER_HPP_
#define __HTTP_PARSER_HPP_

#include <vector>
#include <utility>
#include <stdexcept>
#include "RequestMeta.hpp"
#include "ResponseMeta.hpp"
#include <bits/stdc++.h>

class HttpParser {
private:
    /* data */
public:
    HttpParser();
    ~HttpParser();

    const static int UNSPECIFIED = -1;

    static std::pair<bool, std::string> getCacheControlAttribute(const std::string& src, const std::string& restrict);

    static std::pair<bool, size_t> findEmptyLine(const std::vector<char>& req);
    static RequestMeta parseHeader(const std::vector<char>& req);
    // static std::vector<char> parseHttpBody(const std::vector<char>& req);
    static RequestType getReqType(const std::string& header);
    static std::string getUrl(const std::string& header);
    static std::pair<std::string, uint16_t> getHostAndPort(const std::string& header);
    static size_t getContentLength(const std::string& header, RequestType r_type);

    static std::string getFirstLine(const std::string& header);

/* response */
    static ResponseMeta parseRespHeader(const std::vector<char>& res);
    static std::string getDate(const std::string& header);
    static std::pair<bool, std::string> helper(const std::string& toFind, const std::string& header);
    static std::pair<bool, std::string> getLastModified(const std::string& header);
    static std::pair<bool, std::string> getExpires(const std::string& header);
    static std::pair<bool, std::string> getEtag(const std::string& header);
    static bool check(const std::string& toFind, const std::string& header);
    static std::pair<bool, std::string> getAge(const std::string& header);
    static std::pair<bool, std::string> getCacheControl(const std::string& header);
    static std::pair<bool, std::string> getTransferEncoding(const std::string& header);
    static bool isLastChunk(const std::vector<char>& chunk);



};

#endif
