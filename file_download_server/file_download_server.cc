#include "file_download_server.h"

#include <cstring>
#include <errno.h>
#include <iostream>

#include "../../ztl/utils/socket_util.h"

namespace ztl {
namespace {
constexpr int kSocketListenQueue = 100;

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

FileDownloadServer::~FileDownloadServer() {}

void FileDownloadServer::Start() {
  cout << GetHostName() << ": starting file download server on port: "
       << port_ << endl; 
  RETURN_IF_FALSE(InitSocket());
}

bool FileDownloadServer::InitSocket() {
  struct addrinfo hints, *res;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;  // use machine's IP 

  getaddrinfo(nullptr, std::to_string(port_).c_str(), &hints, &res);
  AddrInfoDeleter res_deleter(res);

  socket_ = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (socket_ <= 0) {
    cerr << "Error: cannot create socket: " << std::strerror(errno)
         << std::endl;;
    return false;
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
  cout << "\nListening for incoming requests ...";

  return true;
}

}  // namespace ztl
