#include "HttpParser.hpp"
#include "RequestType.hpp"
#include <sstream>
#include <regex>
#include <assert.h>
#include <algorithm>
#include <cctype>
#include <string>
#include "Util.hpp"

std::pair<bool, size_t> HttpParser::findEmptyLine(const std::vector<char>& req) {
    for (size_t i = 0; i < req.size() - 3; ++i) {
        if (req[i] == '\r' && req[i+1] == '\n' && req[i+2] == '\r' && req[i+3] == '\n') {
            return std::pair<bool, size_t>(true, i);
        }
    }

    return std::pair<bool, size_t>(false, req.size());
}

RequestType HttpParser::getReqType(const std::string& header) {
    std::string method;

    std::regex req_method_rgx("^GET|POST|CONNECT");
    std::smatch req_method_matches;
    if (std::regex_search(header, req_method_matches, req_method_rgx)) {
        if (req_method_matches.size() != 1) {
            throw std::invalid_argument("Error: more than one http method specified");
        }
        method = req_method_matches[0].str();
    } else {
        throw std::invalid_argument("Error: none or not supported http method specified");
    }

    // get RequestType based on string representation
    return repr_to_req_type(method);
}

std::string HttpParser::getUrl(const std::string& header) {

    std::string url;

    // use regex to parse url path
    std::regex url_rgx("(?:GET|CONNECT|POST)\\s(\\S*)\\s");
    std::smatch url_matches;

    if (std::regex_search(header, url_matches, url_rgx)) {
        if (url_matches.size() != 2) {
            throw std::invalid_argument("Error: invalid url path in request");
        }
        url = url_matches[1].str();
    } else {
        throw std::invalid_argument("Error: no url path in request");
    }

    return url;
}

size_t HttpParser::getContentLength(const std::string& header, RequestType r_type) {
    size_t content_len;
    std::regex content_len_rgx("(?:Content-Length:) (\\d+)");
    std::smatch content_len_matches;
    if (std::regex_search(header, content_len_matches, content_len_rgx)) {
        if (content_len_matches.size() != 2 && r_type == POST) {
            throw std::invalid_argument("Error: invalid content length");
        }
        log_info("Content length is: " + content_len_matches[1].str());
        content_len = std::stol(content_len_matches[1].str());
        if (r_type != POST && content_len != 0) {
            throw std::invalid_argument("Error: invalid content length");
        }
    } else {
        if (r_type == POST) {
            throw std::invalid_argument("Error: invalid content length");
        }
        content_len = 0;
    }

    return content_len;
}

std::pair<std::string, uint16_t> HttpParser::getHostAndPort(const std::string& header) {
    std::string host;
    std::string port_str;
    uint16_t port;

    std::regex host_port_rgx("(?:Host: )([^:\\s]+)(?::*)(\\d*)");
    std::smatch host_port_matches;
    if (std::regex_search(header, host_port_matches, host_port_rgx)) {
        if (host_port_matches.size() != 3) {
            throw std::invalid_argument("Error: invalid host format");
        }

        host = host_port_matches[1].str();
        port_str = host_port_matches[2].str();
        if (port_str.length() == 0) {
            port = 80;
        } else {
            port = std::stol(port_str);
        }
    } else {
        throw std::invalid_argument("Error: Missing host information");
    }
    return std::pair<std::string, uint16_t>(host, port);
}

std::string HttpParser::getFirstLine(const std::string& header){
    size_t f = header.find("\r\n");
    return header.substr(0,f);
}

std::pair<bool, std::string> HttpParser::getCacheControlAttribute(const std::string& src, const std::string& restrict) {
    std::regex rgx(restrict);
    std::smatch matches;
    if (std::regex_search(src, matches, rgx)) {
        if (matches.size() != 2) {
            return std::pair<bool, std::string>(false, "");
        }
        return std::pair<bool, std::string>(true, matches[1].str());
    }
    return std::pair<bool, std::string>(false, "");
}

RequestMeta HttpParser::parseHeader(const std::vector<char>& req) {

    std::pair<bool, size_t> ans = HttpParser::findEmptyLine(req);
    
    size_t empty_line_idx = ans.first ? ans.second : req.size() - 1;

    std::string req_head;
    std::string method;
    RequestType req_t;
    std::string url;
    size_t content_len;

    // combine vector of char into string
    std::stringstream req_ss;
    for (size_t i = 0; i <= empty_line_idx; ++i) {
        req_ss << req[i];
    }
    req_head = req_ss.str();
    req_head = lstrip(req_head);

    // parse request type
    req_t = getReqType(req_head);
    url = getUrl(req_head);

    // parse content length
    content_len = getContentLength(req_head, req_t);
    std::pair<std::string, uint16_t> host_port = getHostAndPort(req_head);
    std::string host = host_port.first;
    uint16_t port = host_port.second;

    if (req_t == CONNECT) {
        port = 443;
    }

    //parse first line
    std::string first_line = getFirstLine(req_head);

    // parse cache control from request
    int max_age, max_stale, min_fresh = HttpParser::UNSPECIFIED;
    bool is_no_cache, is_no_store, is_only_if_cached = false;
    std::pair<bool, std::string> cache_ctrl_ans = getCacheControl(req_head);
    if (cache_ctrl_ans.first == true) {
        const std::string& cache_ctrl_str = allToLowercase(cache_ctrl_ans.second);

        max_age = assignValueOrUnspecified( getCacheControlAttribute(cache_ctrl_str, "(?:max-age=)(\\d+)") );
        max_stale = assignValueOrUnspecified( getCacheControlAttribute(cache_ctrl_str, "(?:max-stale=)(\\d+)") );
        min_fresh = assignValueOrUnspecified( getCacheControlAttribute(cache_ctrl_str, "(?:min-fresh=)(\\d+)") );
        is_no_cache = getCacheControlAttribute(cache_ctrl_str, "(no-cache)").first;
        is_no_store = getCacheControlAttribute(cache_ctrl_str, "(no-store)").first;
        is_only_if_cached = getCacheControlAttribute(cache_ctrl_str, "(only-if-cached)").first;
    }

    return RequestMeta(req_t, url, content_len, host, port, first_line, max_age, max_stale, min_fresh, is_no_cache, is_no_store, is_only_if_cached, req_head);
}




/* response */

std::string HttpParser::getDate(const std::string& header){
    size_t f1 = header.find("Date: ");
    if(f1 == std::string::npos){
        throw std::invalid_argument("Error: invalid Date");
    }
    size_t f2 = header.find("\r\n",f1+1);
    std::string Date = header.substr(f1+6,f2-f1-6);
    return Date;
}

std::pair<bool, std::string> HttpParser::helper(const std::string& toFind, const std::string& header) {
    size_t f1 = header.find(toFind);
    if(f1 == std::string::npos){
        return std::pair<bool, std::string>(false, "");
    }
    size_t f2 = header.find("\r\n",f1+1);
    size_t len = toFind.size();
    std::string ans = header.substr(f1+len,f2-f1-len);

    return std::pair<bool, std::string>(true, ans);
}

bool HttpParser::check(const std::string& toFind, const std::string& header){
    size_t f1 = header.find(toFind);
    if(f1 == std::string::npos){
        return false;
    }
    else{
        return true;
    }
}

std::pair<bool, std::string> HttpParser::getLastModified(const std::string& header){
    return HttpParser::helper("Last-Modified: ", header);
}

std::pair<bool, std::string> HttpParser::getExpires(const std::string& header){
    return HttpParser::helper("Expires: ", header);
}

std::pair<bool, std::string> HttpParser::getEtag(const std::string& header){
    return HttpParser::helper("Etag: ", header);
}


std::pair<bool, std::string> HttpParser::getAge(const std::string& header){
    return HttpParser::helper("Age", header);
}

std::pair<bool, std::string> HttpParser::getCacheControl(const std::string& header){
    return HttpParser::helper("Cache-Control: ", header);
}

std::pair<bool, std::string> HttpParser::getTransferEncoding(const std::string& header){
    return HttpParser::helper("Transfer-Encoding: ", header);
}

bool HttpParser::isLastChunk(const std::vector<char>& chunk) {
    for (size_t i = 0; i < chunk.size() - 4; ++i) {
        if (chunk[i] == '0' && chunk[i+1] == '\r' && chunk[i+2] == '\n' && chunk[i+3] == '\r' && chunk[i+4] == '\n') {
            return true;
        }
    }

    return false;
}

//std::vector<std::string> HttpParser::parseCC(std::pair<bool, std::string> CacheControLine) {
//    std::vector<std::string> tokens;
//    if(CacheControLine.first) {
//        std::string CCLine = CacheControLine.second;
//
//        std::stringstream check(CCLine);
//        std::string intermediate;
//        // Tokenizing  ','
//        while (getline(check, intermediate, ',')) {
//            tokens.push_back(intermediate);
//        }
//    }
//    return tokens;
//}



ResponseMeta HttpParser::parseRespHeader(const std::vector<char>& res){

    std::pair<bool, size_t> ans = HttpParser::findEmptyLine(res);
    size_t empty_line_idx = ans.first ? ans.second : res.size() - 1;
    std::string res_head;

    // combine vector of char into string
    std::stringstream res_ss;
    for (size_t i = 0; i <= empty_line_idx; ++i) {
        res_ss << res[i];
    }
    res_head = res_ss.str();
    res_head = lstrip(res_head);

    //parse first line
    std::string FirstLine = getFirstLine(res_head);
    std::string Date = getDate(res_head);
    std::pair<bool, std::string> LastModified = getLastModified(res_head);
    std::pair<bool, std::string> Etag = getEtag(res_head);
    std::pair<bool, std::string> Expires = getExpires(res_head);
    std::pair<bool, std::string> Age = getAge(res_head);
    //parse cache-control
    std::pair<bool, std::string> CacheControl = getCacheControl(res_head);

    //parse transfer encoding
    std::pair<bool, std::string> TransferEncoding = getTransferEncoding(res_head);


    return ResponseMeta(res_head, FirstLine, Date, LastModified, Expires, Etag, Age, CacheControl, TransferEncoding);
}





