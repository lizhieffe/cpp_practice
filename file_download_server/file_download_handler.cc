#include "file_download_handler.hh"

#include <fstream>
#include <iostream>

#include "network/socket_sender.hh"

namespace ztl {

constexpr char kContextLengthStr[] = "$CONTENT_LENGTH";

void FileDownloadHandler::Handle(
    int socket_fd, const HttpLine& http_line, const HttpHeader& http_header) {
  auto search = http_line.GetParams().find("file");
  if (search == http_line.GetParams().end()) {
    std::cout << "Cannot find the file to download.";
    return;
  }

  // Sends response.
  auto socket_sender = NewSocketSender(socket_fd);
  // std::string response { "HTTP/1.1 200 OK\r\nContent-Length: 14\r\n\r\nHello World!\r\n" };
  std::ifstream file("/home/lizhieffe/Downloads/" + search->second, std::ifstream::binary);
  //std::ifstream file("/home/lizhieffe/Downloads/readme.txt", std::ifstream::binary);
  std::string s(std::istreambuf_iterator<char>(file), {});
  std::cout << s.size() << std::endl;
  std::cout << "Reading " << s.size() << " characters..." << std::endl;

  std::string header { "HTTP/1.1 200 OK\r\nContent-Length: $CONTENT_LENGTH\r\n\r\n" };
  header.replace(header.find(kContextLengthStr), sizeof(kContextLengthStr)-1, std::to_string(s.size()));
  socket_sender->Send(header);
  socket_sender->Send(s);
  file.close();
}

}  // namespace ztl
