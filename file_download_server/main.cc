#include "file_download_server.hh"

int main() {
  ztl::FileDownloadServer server("abc", 9999);
  server.Start();
}
