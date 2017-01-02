#ifndef ZTL_REQUEST_HANDLER
#define ZTL_REQUEST_HANDLER

#include "network/http_line.hh"
#include "network/http_header.h"

namespace ztl {

class RequestHandler {
 public:
  ~RequestHandler() {}
  virtual void Handle(int socket_fd, const HttpLine& http_line,
                      const HttpHeader& http_header) = 0;
};

}  // namespace ztl

#endif  // ZTL_REQUEST_HANDLER
