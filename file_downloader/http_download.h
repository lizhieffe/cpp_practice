#ifndef HTTP_DOWNLOAD_H
#define HTTP_DOWNLOAD_H

#include <cstdio>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>

namespace ztl {

class HttpDownload {
 public:
  HttpDownload(const char* host_addr, const int port,
               const char* get_path, const char* save_file_name);
  ~HttpDownload();

  bool Start();
  void Cancel();
  void GetPos(unsigned long long total_size,
              unsigned long long downloaded_size);

 protected:
  bool InitSocket();
  bool SendRequest();
  bool ReceiveData();
  bool CloseTransfer();

 private:
  std::string addr_;
  int port_;
  std::string get_path_;
  std::string save_file_name_;

  int socket_fd_ = 0;
  FILE* fp_ = nullptr;
  unsigned long long total_size_ = 0;
  unsigned long long downloaded_size_ = 0;

  bool header_received_ = false;
  bool cancelled_ = false;
};

}  // namespace ztl

#endif  // HTTP_DOWNLOAD_H
