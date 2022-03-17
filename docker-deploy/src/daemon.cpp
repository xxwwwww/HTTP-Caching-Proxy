#define BOOST_LOG_DYN_LINK 1
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <exception>

#include "HttpParser.hpp"
#include "RequestMeta.hpp"
#include "Util.hpp"
#include "Cache.hpp"

#define TCP_MAX_SIZE 65535
const char * SUCCESS_MSG = "HTTP/1.1 200 OK\r\n\r\n";

// used for passing arguments into threads
typedef struct {
  int fd;
  std::string addr;
  Cache *cache;
} parameter_t;

static void start_daemon() {
  pid_t pid;

  pid = fork();
  if (pid < 0) {
    exit(EXIT_FAILURE);
  }

  if (pid > 0) {
    exit(EXIT_SUCCESS);
  }

  if (setsid() < 0) {
    exit(EXIT_FAILURE);
  }

  // second fork
  pid = fork();
  if (pid < 0) {
    exit(EXIT_FAILURE);
  }

  // let parent exit
  if (pid > 0) {
    exit(EXIT_SUCCESS);
  }

  umask(0);
  chdir("/");

  for (int x = sysconf(_SC_OPEN_MAX); x >= 0; x--) {
    close(x);
  }
}

int connect_to_remote(const std::string & host, const std::string & url, uint16_t port) {
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo * host_info_list;
  char hostname[65535];
  strcpy(hostname, host.c_str());

  char port_num[65535];
  strcpy(port_num, std::to_string(port).c_str());
  memset(&host_info, 0, sizeof(host_info));

  // auto choose from ipv4 and ipv6
  host_info.ai_family = AF_UNSPEC;

  // TCP
  host_info.ai_socktype = SOCK_STREAM;

  status = getaddrinfo(hostname, port_num, &host_info, &host_info_list);
  if (status != 0) {
    return -1;
  }

  socket_fd = socket(host_info_list->ai_family,
                     host_info_list->ai_socktype,
                     host_info_list->ai_protocol);

  status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    return -1;
  }

  freeaddrinfo(host_info_list);

  return socket_fd;
}

void * handler(void * ptr) {

  // parse argument from struct
  int client_connection_fd;
  std::string ip_addr;
  parameter_t * param = (parameter_t *)ptr;
  client_connection_fd = param->fd;
  ip_addr = param->addr;
  Cache cache = *(param->cache);

  // free memory
  delete (parameter_t*)ptr;

  // buffer is the payload of tcp packet
  char buffer[TCP_MAX_SIZE + 1] = {'\0'};
  ssize_t rcvd = recv(client_connection_fd, buffer, TCP_MAX_SIZE, 0);

  // check if successfully received client request
  if (rcvd <= 0) {
    log_info("ERROR failed to receive client request");
    try {
      send_error_code(client_connection_fd, 400);
    } catch (std::exception& e) {
      log_info("WARNING: " + std::string(e.what()));
    }
    return NULL;
  }

  try {
    std::vector<char> payload_vec;
    for (ssize_t i = 0; i < rcvd; ++i) {
      payload_vec.push_back(buffer[i]);
    }

    // parse client request
    RequestMeta meta = HttpParser::parseHeader(payload_vec);

    // try to connect to remote server specified in client request
    int server_socket_fd =
        connect_to_remote(meta.getHost(), meta.getUrl(), meta.getPort());
    if (server_socket_fd == -1) {
      send_error_code(client_connection_fd, 502);
      close(client_connection_fd);
      log_info("ERROR failed to establish connection with remote server");
      return NULL;
    }

    log_info("\"" + meta.getFirstLine() + "\" from " + getIpAddr(client_connection_fd) + " @ " + getTimeAsString());
    if (meta.getRequestType() == POST) {
      // forward request to server
      Send(server_socket_fd, payload_vec);

      std::vector<char> server_resp = Recv(server_socket_fd, true);
      std::stringstream ss;
      for (size_t i = 0; i < server_resp.size(); i++) {
        ss << server_resp[i];
      }

      std::string server_resp_str = ss.str();

      // forward response to client
      Send(client_connection_fd, server_resp);
      close(server_socket_fd);
    }
    else if (meta.getRequestType() == CONNECT) {

      // at this time we have already established connection with remote server
      //  send msg back to client to indicate success

      // send success message back to client
      ssize_t sent;
      sent = send(client_connection_fd, SUCCESS_MSG, strlen(SUCCESS_MSG), 0);
      sent = 0;
      ssize_t received;

      // forward packet from client to server and from server to client
      fd_set fds;

      int max_fd = client_connection_fd > server_socket_fd ? client_connection_fd
                                                           : server_socket_fd;
      struct timeval tv;
      tv.tv_sec = 10;
      tv.tv_usec = 0;
      while (true) {
        FD_ZERO(&fds);
        FD_SET(client_connection_fd, &fds);
        FD_SET(server_socket_fd, &fds);

        int res = select(max_fd + 1, &fds, NULL, NULL, &tv);
        

        if (res <= 0) {
          throw std::runtime_error(getErrorMsg() + " select failed");
        }
        else {
          try {
            int descriptors[2] = {client_connection_fd, server_socket_fd};
            for (int i = 0; i < 2; ++i) {
              if (FD_ISSET(descriptors[i], &fds)) {
                std::vector<char> buffer(TCP_MAX_SIZE + 1, '\0');
                received = recv(descriptors[i], &(buffer.data()[0]), TCP_MAX_SIZE, 0);
                if (received <= 0) {
                  throw std::runtime_error(getErrorMsg() + " with " +
                                           std::to_string(received) +
                                           " from connect recv");
                }

                sent =
                    send(descriptors[i == 0 ? 1 : 0], &(buffer.data()[0]), received, 0);
                if (sent <= 0) {
                  throw std::runtime_error(getErrorMsg() + " with " +
                                           std::to_string(received) +
                                           " from connect send");
                }
              }
            }
          }
          catch (const std::runtime_error & e) {
            ;
          }
        }
      }
      log_info("Tunnel closed");
      close(server_socket_fd);
    }
      /* receive GET request from server, check if it is in the cache. If in the cache check its revalidation and freshness. If not, forward the request to the server directly */
    
    else if (meta.getRequestType() == GET) {
      if(cache.find(meta.getFirstLine())){
          std::vector<char> response = cache.get(meta.getFirstLine());
          ResponseMeta resp = HttpParser::parseRespHeader(response);
          if(resp.isNoCache()){
              log_info("cached, but requires re-validation");
              std::string new_req = cache.revalidate(resp,meta);
              //send revalidate request to the server
              send(server_socket_fd, new_req.c_str(), new_req.length(), 0);
              std::vector<char> r1 = Recv(server_socket_fd, true);
              try{
                  ResponseMeta sec_response= HttpParser::parseRespHeader(r1);
                  std::string firstLine = sec_response.getFirstLine();
                  //if return 304, use the response in the cache, else store the response send by server.
                  size_t f = firstLine.find("304");
                  if(f == std::string::npos){
                      cache.put(sec_response.getFirstLine(),r1);
                      log_info("Responding \"HTTP/1.1 200 OK\"");
                      Send(client_connection_fd, r1);
                      close(server_socket_fd);
                  }
                  else{
                      log_info("in cache, valid");
                      Send(client_connection_fd,cache.get(meta.getFirstLine()));
                      close(server_socket_fd);
                  }
              }
              catch (std::invalid_argument &e) {
                  send_error_code(client_connection_fd, 502);
              }

          }
          else{
	    //check freshness
              if(!(resp.if_fresh())){
                  std::string new_req = cache.revalidate(resp,meta);
                  send(server_socket_fd, new_req.c_str(), new_req.length(), 0);
                  std::vector<char> r1 = Recv(server_socket_fd, true);
                  try {
                      ResponseMeta sec_response = HttpParser::parseRespHeader(r1);
                      std::string firstLine = sec_response.getFirstLine();
                      size_t f = firstLine.find("304");
                      if(f == std::string::npos){
                          cache.put(sec_response.getFirstLine(), r1);
                          Send(client_connection_fd, r1);
                          close(server_socket_fd);
                      }
                      else{
                          //send the response in the cache back to the client
                          log_info("in cache, valid");
                          Send(client_connection_fd,cache.get(meta.getFirstLine()));
                          close(server_socket_fd);
                      }
                  }
                  catch (std::invalid_argument &e) {
                      send_error_code(client_connection_fd, 502);
                  }
              }
          }
      }
      //not in the cache
      else {
          log_info("not in cache");
          Send(server_socket_fd, payload_vec);
          std::vector<char> resp = Recv(server_socket_fd, true);

          try {
              ResponseMeta response = HttpParser::parseRespHeader(resp);
              // log_info("Responding" + response.getFirstLine());
              log_info("Received \"" + stripNewLine(response.getFirstLine()) + "\" from " + meta.getHost());
              if(cache.store_response(resp)){
                  cache.put(response.getFirstLine(), resp);
              }
              Send(client_connection_fd, resp);
              close(server_socket_fd);
          }
          catch (std::invalid_argument &e) {
              send_error_code(client_connection_fd, 502);
              log_info("ERROR " + std::string(e.what()));
          }
      }
    }
    else {
      throw std::invalid_argument("Unsupported http request type");
    }
  }
  catch (const std::exception & e) {
    log_info("WARNING " + std::string(e.what()));
  }

  close(client_connection_fd);
  return NULL;
}

int main(int argc, const char ** argv) {
  start_daemon();

  Cache cash;

  // setup listening tcp server
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo * host_info_list;
  const char * hostname = NULL;
  const char * port = "12345";
  memset(&host_info, 0, sizeof(host_info));

  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags = AI_PASSIVE;

  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  if (status != 0) {
    log_info("failed to get addr info");
    return EXIT_FAILURE;
  }

  socket_fd = socket(host_info_list->ai_family,
                     host_info_list->ai_socktype,
                     host_info_list->ai_protocol);

  if (socket_fd == -1) {
    log_info("failed to create socket");
    return EXIT_FAILURE;
  }

  int yes = 1;
  status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  if (status == -1) {
    log_info("failed to set reuse addr");
    return EXIT_FAILURE;
  }

  status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    log_info("failed to bind to port");
    return EXIT_FAILURE;
  }

  status = listen(socket_fd, 100);
  if (status == -1) {
    log_info("failed to listen on port");
    return EXIT_FAILURE;
  }

  freeaddrinfo(host_info_list);

  while (true) {
    try {
      struct sockaddr_storage socket_addr;
      socklen_t socket_addr_len = sizeof(socket_addr);
      int client_connection_fd;
      client_connection_fd =
          accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);

      if (client_connection_fd < 0) {
        continue;
      }

      std::string ip_addr = inet_ntoa(((struct sockaddr_in *)&socket_addr)->sin_addr);

      if (client_connection_fd == -1) {
        continue;
      }

      // spawn a thread for handling the request
      parameter_t * parameter = new parameter_t;
      parameter->addr = ip_addr;
      parameter->fd = client_connection_fd;
      parameter->cache = &cash;


      pthread_t handler_thread;
      pthread_create(&handler_thread, NULL, handler, (void*)parameter);
    } catch (const std::exception& e) {
      continue;
    }
  }

  close(socket_fd);
  return EXIT_SUCCESS;
}
