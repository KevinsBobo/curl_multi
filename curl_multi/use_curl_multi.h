#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#endif

#include "libcurl/multi.h"

#define MAX_WAIT_MSECS 30*1000 /* Wait max. 30 seconds */

static const char *urls[] = {
  "http://www.microsoft.com",
  "http://www.yahoo.com",
  "http://www.baidu.com",
  "http://www.sogou.com",
  "http://www.soso.com"
};
#define CNT 5

static size_t cb(char *d , size_t n , size_t l , void *p);

static void init(CURLM *cm , int i);

int start_test(void);
