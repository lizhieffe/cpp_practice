#include "http_download.h"

int main() {
  ztl::HttpDownload http_download("www.tlu.ee", 80,
                                  "~rajaleid/manuals/Canon_EOS_5d_mkII_User_Manual.pdf",
                                  "manual.pdf");
  http_download.Start();
}
