#include "network/http_line.hh"
#include "network/http_header.h"
#include "request_handler.hh"

namespace ztl {

class FileDownloadHandler : public RequestHandler {
 public:
  void Handle(int socket_fd, const HttpLine& http_line,
              const HttpHeader& http_header) override;
};

}
