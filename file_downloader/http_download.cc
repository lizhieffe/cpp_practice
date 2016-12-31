#include "http_download.h"

#include <cerrno>
#include <chrono>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <sys/select.h>
#include <thread>
#include <unistd.h>

namespace ztl {
namespace {

#define BUFFER_SIZE 1024

#define LOG_LINE_BREAK \
  std::cout << "-------------------------------------------------------------" \
            << std::endl

std::string GetHostName() {
  char host_name[1024];
  if (gethostname(host_name, sizeof host_name) == 0) {
    return host_name;
  } else {
    return "";
  }
}

std::string GetAiFamily(int ai_family) {
  std::string name;
  if (ai_family == AF_UNSPEC) {
    name = "AF_UNSPEC";
  } else if (ai_family == AF_INET) {
    name = "IPV4";
  } else if (ai_family == AF_INET6) {
    name = "IPV6";
  } else {
    name = std::to_string(ai_family);
  }
  return name;
}

void PrintAddrInfo(const addrinfo& addr_info) {
  char addr[1024];
  char service[20];

  getnameinfo(addr_info.ai_addr, sizeof *addr_info.ai_addr, addr, sizeof addr,
              service, sizeof service, 0);
  std::cout << "  address: " << addr << std::endl;
  std::cout << "  service: " << service << std::endl;
  std::cout << "ai_family: " << GetAiFamily(addr_info.ai_family) << std::endl;
}

}  // namespace

HttpDownload::HttpDownload(const char* addr, const int port,
                           const char* get_path, const char* save_file_name)
  : addr_(addr),
    port_(port),
    get_path_(get_path),
    save_file_name_(save_file_name) {
  std::cout << "Host name: " << GetHostName() << std::endl;
}

HttpDownload::~HttpDownload() {
  CloseTransfer();
}

bool HttpDownload::InitSocket() {
  int status;
  struct addrinfo hints;
  struct addrinfo* servinfo;

  std::memset(&hints, 0, sizeof hints);   // make sure the struct is empty
  hints.ai_family = AF_UNSPEC;            // don't care IPV4 or IPV6
  hints.ai_socktype = SOCK_STREAM;        // TCP stream socket
  hints.ai_flags = AI_PASSIVE;            // fill in my IP for me

  if ((status = getaddrinfo(addr_.c_str(), std::to_string(port_).c_str(),
                            &hints, &servinfo)) != 0) {
    std::cerr << "getaddrinfo error: " << gai_strerror(status) << std::endl;
    return false;
  }
  std::cout << "Peer info:" << std::endl;
  PrintAddrInfo(*servinfo);

  socket_fd_ = socket(servinfo->ai_family, servinfo->ai_socktype,
                      servinfo->ai_protocol);
  if (connect(socket_fd_, servinfo->ai_addr, servinfo->ai_addrlen) == 0) {
    std::cout << "Connected to the peer." << std::endl;
  } else {
    std::cerr << "Cannot connected to the peer." << std::endl;
    return false;
  }

  // char buf[BUFFER_SIZE];
  // int num_bytes;
  // if ((num_bytes = recv(socket_fd, buf, BUFFER_SIZE - 1, 0)) == -1) {
  //   std::cout << "client: recv error.";
  // } else {
  //   buf[BUFFER_SIZE - 1] = '\0';
  //   std::cout << "Receviced: " << buf << std::endl;
  // }


  freeaddrinfo(servinfo);   // free the linked-list
  return true;
}

boor HttpDownload::SendRequest() {
  if (get_path_.empty()) {
    return false;
  }

  char request_header[256];
  int len = sprintf(request_header, 
      "GET %s HTTP/1.1\r\n"
      "Host: %s\r\n"
      "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/55.0.2883.87 Safari/537.36\r\n"
      "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"
      "Range: bytes=%llu-6361568\r\n"    //从m_receivedDataSize位置开始
      "Connection: close\r\n"
      "\r\n",
      get_path_.c_str(), addr_.c_str(), downloaded_size_);    
  // int len = sprintf(request_header, 
  //     "GET %s HTTP/1.1\r\n"
  //     "Host: %s\r\n"
  //     "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/55.0.2883.87 Safari/537.36\r\n"
  //     "Range: bytes=%llu-6361568\r\n"    //从m_receivedDataSize位置开始
  //     "Connection: close\r\n"
  //     "\r\n",
  //     get_path_.c_str(), addr_.c_str(), downloaded_size_);    

  LOG_LINE_BREAK;
  std::cout << "Sending request header (length " << len << "):\n\n"
            << request_header << std::endl;
  LOG_LINE_BREAK;

  int bytes_sent = 0;
  while (true) {
    fd_set write_fd;
    FD_ZERO(&write_fd);
    FD_SET(socket_fd_, &write_fd);

    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;

    int err = select(socket_fd_ + 1, nullptr, &write_fd, nullptr, &timeout);
    if (err < 0) {
      std::cerr << "Socket write fd set is not ready. Error is: "
                << std::strerror(errno) << std::endl;;
      break;
    } else if (err == 0) { /* timeout */
      continue;
    }

    int n_bytes = send(socket_fd_, request_header + bytes_sent, len, 0);
    if (n_bytes < 0) {
      // TODO: add retry mechanism if the error is not fatal.
      //if (WSAGetLastError() != WSAWOULDBLOCK()) {
        break;
      //}
      //n_bytes = 0;
    } else {
      std::cout << "Sent " << n_bytes << " bytes." << std::endl;
    }
    bytes_sent += n_bytes;
    len -= n_bytes;
    if (len == 0) {
      std::cout << "Finished sending request header." << std::endl;
      std::cout << "===lizhi 000000";
      return true;
    }
  }
  return false;
}

bool HttpDownload::ReceiveData() {
  std::cout << "===lizhi 11111";
  char response_header[BUFFER_SIZE] = {0};

  struct timeval timeout;
  timeout.tv_sec = 3;
  timeout.tv_usec = 0;

  int retry_count = 0;
  int received_bytes = 0;
  while (true) {
    fd_set read_fd_set;
    FD_ZERO(&read_fd_set);
    FD_SET(socket_fd_, &read_fd_set);

    int err = select(socket_fd_ + 1, &read_fd_set, nullptr, nullptr, &timeout);
    if (err < 0) {
      std::cerr << "Socket read fd set is not ready. Error is: "
                << std::strerror(errno) << std::endl;;
      break;
    } else if (err == 0) { /* timeout */
      if (++retry_count >= 10) {
        return false;
      } else {
        continue;
      }
    }
    retry_count = 0;

    // TODO: here we read one byte each time in order to read header part only.
    // Use some more advanced technique to read more bytes each time to
    // improve efficiency.
    if (recv(socket_fd_, response_header + received_bytes, 1, 0) <= 0) {
      std::cerr << "Reading socket failed: " << std::strerror(errno)
                << std::endl;
      return false;
    }
    ++received_bytes;
    if (received_bytes >= BUFFER_SIZE) {
      std::cerr << "Received bytes >= BUFFER_SIZE (" << BUFFER_SIZE << ")"
                << std::endl;
      return false;
    }
    if (received_bytes >= 4 &&
        response_header[received_bytes - 4] == '\r' &&
        response_header[received_bytes - 3] == '\n' &&
        response_header[received_bytes - 2] == '\r' &&
        response_header[received_bytes - 1] == '\n') {
      break;
    }
  }
  response_header[received_bytes] = '\0';
  LOG_LINE_BREAK;
  std::cout << "Received response header:\n\n"
            << response_header << std::endl;

  if (strncmp(response_header, "HTTP/", 5))
    return false;
  int status = 0; 
  float version = 0.0;
  unsigned long long start_pos, end_pos, total_size_in_response_header;
  start_pos = end_pos = total_size_in_response_header = 0;
  if (sscanf(response_header, "HTTP/%f %d ", &version, &status) != 2)
    return false;
  char* findStr = strstr(response_header, "Content-Range: bytes ");
  if (findStr == nullptr) {
    std::cerr << "Error: reponse header doesn't contain the Content-Range info.\n" << std::endl;
    return false;
  }
  if (sscanf(findStr, "Content-Range: bytes %llu-%llu/%llu", 
             &start_pos, &end_pos, &total_size_in_response_header) != 3)
    return false;
  if ((status != 200 && status != 206) || total_size_in_response_header == 0)
    return false;
  if (!header_received_) {    //第一次获取HTTP响应头,保存目标文件总大小
    total_size_ = total_size_in_response_header;
    header_received_ = true;
  }
  if (downloaded_size_ != start_pos) {
    return false;
  }


  retry_count = 0;
  while (true) {
    char buf[BUFFER_SIZE] = {0};

    fd_set read_fd_set;
    FD_ZERO(&read_fd_set);
    FD_SET(socket_fd_, &read_fd_set);

    int err = select(socket_fd_ + 1, &read_fd_set, nullptr, nullptr, &timeout);
    if (err < 0) {
      if (++retry_count >= 10) {
        break;
      } else {
        continue;
      }
    }

    int length = recv(socket_fd_, buf, BUFFER_SIZE, 0);
    if (length < 0) {
      std::cerr << "Cannot read data from socket. Error is: "
                << std::strerror(errno) << std::endl;;
      return false;
    } else if (length == 0) {
      return true;
    }

    int written = fwrite(buf, sizeof(char), length, fp_);
    if (written < length) {
      return false;
    }

    downloaded_size_ += length;
    if (downloaded_size_ == total_size_) {
      std::cout << "Received all the data.";
      return false;
    }
  }

  return true;
}

bool HttpDownload::CloseTransfer() {
  if (socket_fd_ > 0) {
    close(socket_fd_);
    socket_fd_ = 0;
  }
  return true;
}

bool HttpDownload::Start() {
  fp_ = fopen(save_file_name_.c_str(), "wb");
  if (fp_ == nullptr) {
    std::cerr << "ERROR: cannot create file " << save_file_name_ << std::endl;
    return false;
  }

  bool err_flag = false;
  while (true) {
    if (!InitSocket() || !SendRequest() || !ReceiveData()) {
      if (cancelled_) {
        err_flag = true;
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      continue;
    } else {
      break;
    }
  }

  if (fp_ != nullptr) {
    fclose(fp_);
    fp_ = nullptr;
  }
  return err_flag ? false : true;
}

void HttpDownload::Cancel() {
  cancelled_ = true;
  CloseTransfer();
}

void HttpDownload::GetPos(unsigned long long total_size,
            unsigned long long downloaded_size) {}

}  // namespace ztl
