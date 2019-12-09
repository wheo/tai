#include "ftpupload.h"

char g_ftp_id[2][32];
char g_ftp_pw[2][32];
char g_ftp_ip[2][32];

/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2012, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at http://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

/*
 * This example shows an FTP upload, with a rename of the file just after
 * a successful upload.
 *
 * Example based on source code provided by Erick Nuwendam. Thanks!
 */

/* NOTE: if you want this example to work on Windows with libcurl as a
   DLL, you MUST also provide a read callback with CURLOPT_READFUNCTION.
   Failing to do so will give you a crash since a DLL may not use the
   variable's memory when passed in to it from an app like this. */
static size_t read_callback(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  curl_off_t nread;
  /* in real-world cases, this would probably get this data differently
     as this fread() stuff is exactly what the library already would do
     by default internally */
  size_t retcode = fread(ptr, size, nmemb, stream);

  nread = (curl_off_t)retcode;
#if 0
  fprintf(stderr, "*** We read %" CURL_FORMAT_CURL_OFF_T
          " bytes from file\n", nread);
#endif
  return retcode;
}

int ftpupload(char *filename)
{
	char localfile[256];
	sprintf(localfile, "/home/%s/%s", g_channel, filename);

	for (int i=0; i<2; i++) {
		CURL *curl;
		CURLcode res;

		FILE *hd_src = NULL;
		struct stat file_info;
		curl_off_t fsize;

		char remotefile[256];
		char url[256]; // = "ftp://smrftp:smrFtpTnm@54.250.203.47/";

		if (strlen(g_ftp_ip[i]) == 0) {
			continue;
		}

		sprintf(remotefile, "programs/sbs/%s", filename);
		sprintf(url, "ftp://%s:%s@%s/%s", g_ftp_id[i], g_ftp_pw[i], g_ftp_ip[i], remotefile);

		//_l("[FTP] URL : %s\n", url);

		// compare date
		//
		struct curl_slist *headerlist=NULL;
		//static const char buf_1 [] = "RNFR " UPLOAD_FILE_AS;
		//static const char buf_2 [] = "RNTO " RENAME_FILE_TO;

		char buf_1 [256] = "";
		char buf_2 [256] = "";

		sprintf(buf_1, "%s %s", "RNFR", localfile);
		sprintf(buf_2, "%s %s", "RNTO", localfile);

		//_l("[FTP] upload_file_as %s | reanme_file_to %s\n", buf_1, buf_2);
		if (stat(localfile, &file_info)) {
			_l("[FTP] Couldnt open '%s': %s\n", localfile, strerror(errno));
			// 
		} else {
			fsize = (curl_off_t)file_info.st_size;

			//_d("[FTP] Local file size: %" CURL_FORMAT_CURL_OFF_T " bytes.\n", fsize);

			/* get a FILE * of the same file */
			hd_src = fopen(localfile, "rb");

			/* In windows, this will init the winsock stuff */
			curl_global_init(CURL_GLOBAL_ALL);

			/* get a curl handle */
			curl = curl_easy_init();
			if(curl) {
				/* build a list of commands to pass to libcurl */
				headerlist = curl_slist_append(headerlist, buf_1);
				headerlist = curl_slist_append(headerlist, buf_2);

				/* we want to use our own read function */
				curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
                
                /* enable timeout */
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

				/* enable uploading */
				curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

				/* specify target */
				//curl_easy_setopt(curl,CURLOPT_URL, REMOTE_URL);
				curl_easy_setopt(curl,CURLOPT_URL, url);

				/* pass in that last of FTP commands to run after the transfer */
				curl_easy_setopt(curl, CURLOPT_POSTQUOTE, headerlist);

				/* now specify which file to upload */
				curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);

				/* Set the size of the file to upload (optional).  If you give a *_LARGE
				   option you MUST make sure that the type of the passed-in argument is a
				   curl_off_t. If you use CURLOPT_INFILESIZE (without _LARGE) you must
				   make sure that to pass in a type 'long' argument. */
				curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)fsize);

				/* Now run off and do what you've been told! */
				res = curl_easy_perform(curl);
				/* Check for errors */
				if(res != CURLE_OK) {
					//_l("[FTP] curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
				}
				/* clean up the FTP commands list */
				curl_slist_free_all (headerlist);

				/* always cleanup */
				curl_easy_cleanup(curl);
			}
			fclose(hd_src); /* close the local file */

			curl_global_cleanup();
		}
	}

	_l("[FTP] upload complete (%s)\n", filename);
	return 0;
}
