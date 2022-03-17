#ifndef __UTIL_HPP_
#define __UTIL_HPP_

#include <pthread.h>
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <ctime>
#include "RequestType.hpp"

/**
 * This file contains signatures of utility/helper functions
 */

// write std string payload into log file
void log_info(const std::string & payload);

// get string representation based on enum value
std::string req_type_repr(RequestType r_type);

// get enum value based on string representation
RequestType repr_to_req_type(const std::string& repr);

// left-strip null characters ('\0')a string 
std::string lstrip(const std::string& str);

// receives http messages from stream tied to fd and returns vector of char
// this function will parse content length or chunks to make sure it receives
//  full message
std::vector<char> Recv(int fd, bool is_resp);

// sends http messages in payload to stream tied to fd
// this function will check on return values from the send system call to make sure
//  full message is sent
void Send(int fd, const std::vector<char>& payload);

std::string currTime();
std::string allToLowercase(const std::string& str);

// get std string representation of error message based on the value of errno
std::string getErrorMsg();
int assignValueOrUnspecified(const std::pair<bool, std::string>& data);
time_t convertToTime(std::string ToConvert);

// send error http message to fd
void send_error_code(int fd, int error_code);

// get the ip address of client/server connected to fd
std::string getIpAddr(int fd);

// get UTC time representation specified by asctime in std string
std::string getTimeAsString();

// strip http newline "\r\n" from string
std::string stripNewLine(const std::string& line);
#endif