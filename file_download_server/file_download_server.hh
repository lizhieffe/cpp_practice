#ifndef FILE_DOWNLOAD_SERVER
#define FILE_DOWNLOAD_SERVER

#include <string>

namespace ztl {

class FileDownloadServer {
 public:
  FileDownloadServer(const char* address, int port);
  ~FileDownloadServer();

  // Starts the server. Returns only after the server stops.
  void Start();
 
 protected:
  // Initialize the socket and start listening.
  bool InitSocket();
  bool HandleRequest();

 private:
  const std::string address_; 
  const int port_;

  int socket_;
};

}  // namespace ztl

#endif  // FILE_DOWNLOAD_SERVER
