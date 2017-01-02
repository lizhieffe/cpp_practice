#include "file_download_server.hh"

#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <netdb.h>
#include <regex>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#include "network/socket_sender.hh"
#include "utils/socket_util.h"

namespace ztl {
namespace {

constexpr int kSocketListenQueue = 100;
constexpr int kBufferSize=1000;

constexpr char kContextLengthStr[] = "$CONTENT_LENGTH";

#define RETURN_IF_FALSE(boolean)  \
  if (boolean == false) {         \
    return;                       \
  }
}

using std::cerr;
using std::cout;
using std::endl;
using std::string;

FileDownloadServer::FileDownloadServer(const char* address, int port)
  : address_(address),
    port_(port) {}

FileDownloadServer::~FileDownloadServer() {
  if (socket_ > 0) {
    close(socket_);
  }
}

void FileDownloadServer::Start() {
  cout << GetHostName() << ": starting file download server on port: "
       << port_ << endl; 
  RETURN_IF_FALSE(InitSocket());
  while (true) {
    RETURN_IF_FALSE(HandleRequest());
  }
}

bool FileDownloadServer::InitSocket() {
  struct addrinfo hints, *res;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;  // use machine's IP 

  //getaddrinfo("192.168.1.36", std::to_string(port_).c_str(), &hints, &res);
  getaddrinfo(nullptr, std::to_string(port_).c_str(), &hints, &res);
  AddrInfoDeleter res_deleter(res);

  socket_ = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (socket_ <= 0) {
    cerr << "Error: cannot create socket: " << std::strerror(errno)
         << std::endl;;
    return false;
  }

  //// Makes sure the port will be binded successfully even if the process exited
  //// unexpectedly previously. 
  //int i_set_option = 1;
  //setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, (char*)&i_set_option,
  //           sizeof(i_set_option));

  // lose the pesky "Address already in use" error message
  int yes=1;
  if (setsockopt(socket_,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes) == -1) {
    perror("setsockopt");
    exit(1);
  } 
          
          
  int bind_err = bind(socket_, res->ai_addr, res->ai_addrlen);
  if (bind_err == -1) {
    cerr << "Error: cannot bind socket: " << std::strerror(errno)
         << std::endl;;
    return false;
  }
  cout << "\nBinded to socket: " << std::endl;
  PrintAddrInfo(*res);

  int listen_err = listen(socket_, kSocketListenQueue);
  if (listen_err == -1) {
    cerr << "Error: cannot listen on socket: " << std::strerror(errno)
         << std::endl;;
    return false;
  }
  cout << "\nListening for incoming requests ...\n\n";

  return true;
}

bool FileDownloadServer::HandleRequest() {
  struct sockaddr_storage their_addr;
  socklen_t addr_size;
  int new_socket = accept(socket_, (struct sockaddr*)&their_addr, &addr_size);
  if (new_socket == -1) {
    cerr << "Error: cannot accept new request: " << std::strerror(errno)
         << std::endl;;
    return false;
  } else {
    cout << "Accepted new request." << std::endl;
  }

  int retry_count = 0;
  struct timeval timeout;
  timeout.tv_sec = 3;
  timeout.tv_usec = 0;

  std::string request;

  while (true) {
    char buf[kBufferSize] = {0};

    fd_set read_fd_set;
    FD_ZERO(&read_fd_set);
    FD_SET(new_socket, &read_fd_set);

    int err = select(new_socket + 1, &read_fd_set, nullptr, nullptr, &timeout);
    if (err < 0) {
      if (++retry_count >= 10) {
        break;
      } else {
        continue;
      }
    }

    int length = recv(new_socket, buf, kBufferSize, 0);
    if (length < 0) {
      std::cerr << "Cannot read data from socket. Error is: "
                << std::strerror(errno) << std::endl;;
      return false;
    } else if (length == 0) {
      return true;
    }

    request.append(buf);
    if (request.find("\r\n\r\n", 0) != std::string::npos) {
      std::cout << "Request: " << std::endl;
      std::cout << request << std::endl;
      break;
    }
  }

  // Sends response.
  bool status = true;
  auto socket_sender = NewSocketSender(new_socket);
  // std::string response { "HTTP/1.1 200 OK\r\nContent-Length: 14\r\n\r\nHello World!\r\n" };
  std::ifstream file("/home/lizhieffe/Downloads/output.ts", std::ifstream::binary);
  //std::ifstream file("/home/lizhieffe/Downloads/readme.txt", std::ifstream::binary);
  std::string s(std::istreambuf_iterator<char>(file), {});
  std::cout << s.size() << std::endl;
  std::cout << "Reading " << s.size() << " characters..." << std::endl;

  std::string header { "HTTP/1.1 200 OK\r\nContent-Length: $CONTENT_LENGTH\r\n\r\n" };
  header.replace(header.find(kContextLengthStr), sizeof(kContextLengthStr)-1, std::to_string(s.size()));
  status = socket_sender->Send(header);
  status = socket_sender->Send(s);
  file.close();

  close(new_socket);
  return status;
}

}  // namespace ztl
