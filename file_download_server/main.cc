#include "file_download_server.h"

int main() {
  ztl::FileDownloadServer server("abc", 9999);
  server.Start();
}
