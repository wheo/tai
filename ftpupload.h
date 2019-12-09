#ifdef _FTPUPLOAD_H_
#define _FTPUPLOAD_H_

#endif // _FTPUPLOAD_H_

#include "global.h"
#include <stdio.h>
#include <string.h>

#include <curl/curl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

int ftpupload(char *filename);

