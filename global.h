#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <libgen.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <libwebsockets.h>

#define KGRN "\033[0;32;32m"
#define KCYN "\033[0;36m"
#define KRED "\033[0;32;31m"
#define KYEL "\033[1;33m"
#define KBLU "\033[0;32;34m"
#define KCYN_L "\033[1;36m"
#define KBRN "\033[0;33m"
#define RESET "\033[0m"

#ifdef DEBUG
#undef DEBUG
#define _d(fmt, args...) { printf(fmt, ## args); }
#else
#undef DEBUG
#define _d(fmt, args...)
#endif

#include "queue.h"
#include "DeckLinkAPI.h"
#include "ftpupload.h"

extern "C" {
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include <uuid/uuid.h>
}

#define MAX_NUM_CM		128

namespace
{
	class __GET_TICK_COUNT
	{
	public:
		__GET_TICK_COUNT()
		{
			if (gettimeofday(&tv_, NULL) != 0) {
				throw 0;
			}
		}
		timeval tv_;
	};
	__GET_TICK_COUNT timeStart;
}

unsigned long GetFileSize(const char *filename);
extern unsigned long GetTickCount();

extern IDeckLinkInput *g_pDLI;
extern IDeckLinkOutput *g_pDLO;

extern CQueue *g_pqv;
extern CQueue *g_pqa;

extern bool ignore_flag_feed;
extern void *next_playlist;

extern int g_out_delay;
extern int g_cmb_delay;
extern int g_cmm_delay;
extern int g_cma_delay;
extern int g_common_delay;
extern char g_channel[32];

extern char g_ftp_id[2][32];
extern char g_ftp_pw[2][32];
extern char g_ftp_ip[2][32];

//manual mode
extern int ad_play_command;
extern int ad_play_index;
extern int ad_play_elapsed;
extern int ad_play_mode;

extern void *next_apc;
extern void *next_schedule;
extern int g_common_index;
extern FILE *g_fpLog;

extern int g_update_apc;
extern int aying_list;
extern int aying_file;
extern int aying_elapsed;
extern int aying_dur;
extern int g_counter_apc;

enum THREAD_STATE {
        eREADY, eRUNNING, eTERMINATED, eZOMBIE, eABORTED,
};
enum THREAD_EXIT_STATE {
        eJOINABLE, eDETACHABLE,
};

void getUUID(char *out_uuid);

void _l(const char *format, ...);
void _ls(const char *format, ...);

#define MAX_AD_COUNT		16

typedef struct
{
	int yy;
	int mm;
	int dd;

	int h;
	int m;
	int s;
	int f;

	int d; // dur as frame
}TIME;

typedef struct
{
    char is_common;

    int ste_id;
    int sec_id;
    int pg_id;
    int loc_id;
    int cmp_id;
    int ad_id;
    int com_id;
    int ctv_id;
    int priority;
    char prepost;

    char filename[256];
}DInfo;

void MLT(time_t ct, struct tm &t);

void GetTime(int nSec, TIME *pt);
void GetTime(time_t ct, TIME *pt);
void GetTime(time_t ct, int &h, int &m, int &s);

time_t MakeTime(TIME *curt);

int GetHour(int nSec);
int GetMinute(int nSec);
int GetSecond(int nSec);
int GetSecond(TIME t);

int sr(int sd, char *p, int len);

class PThread {
public:
        PThread(char *a_szName = NULL, THREAD_EXIT_STATE a_eExitType = eJOINABLE);
        virtual ~PThread();

        int Start();
        void Terminate();

        THREAD_STATE GetState() const;
        THREAD_EXIT_STATE GetExitType() const;
        char* GetName() const;

        bool IsTerminated() const;
        bool IsRunning() const;

        bool m_bExit;
protected:
        static void* StartPoint(void *a_pParam);

        virtual void Run() = 0;
        virtual void OnTerminate() = 0;

        void Join(pthread_t a_nID);

        void SetState(THREAD_STATE a_eState);

private:
        /* Disable Copy */
        PThread(const PThread&);
        PThread& operator=(const PThread&);

        /* Attribute */
        pthread_t m_nID;
        char *m_szName;

        THREAD_STATE m_eState;
        THREAD_EXIT_STATE m_eExitType;
};

typedef struct tagPos
{
	void *p;
	struct tagPos *prev;
	struct tagPos *next;

}Pos, *POSITION;

class CMyList
{
public:
	CMyList() {
		m_nCount = 0; 
		m_pHead = NULL;
		m_pTail = NULL;
	}

	~CMyList() {};

	int GetCount() { return m_nCount; };
	POSITION GetHeadPosition() { return m_pHead; };

	void AddHead(void *p) {
		POSITION pp = new Pos;

		pp->p = p;
		if (m_pHead) {
			pp->next = m_pHead;
			m_pHead->prev = pp;
		}
		m_pHead = pp;
		if (!m_pTail) {
			pp->prev = NULL;
			pp->next = NULL;
			m_pTail = m_pHead;
		}
		m_nCount++;
	}

	void AddTail(void *p) {
		POSITION pp = new Pos;
		pp->p = p;
		pp->next = NULL;
		if (m_pTail) {
			pp->prev = m_pTail;
			m_pTail->next = pp;
			m_pTail = pp;
		}
		if (!m_pHead) {
			pp->prev = NULL;
			pp->next = NULL;
			m_pHead = m_pTail = pp;
		}
		m_nCount++;
	};

	void RemoveAt(POSITION &pos) {
		POSITION p1 = pos->prev;
		POSITION p2 = pos->next;
	
		if (!p1) { // in case of head
            if (p2) {
                p2->prev = NULL;
                m_pHead = p2;
            } else {
                m_pHead = m_pTail = NULL;
            }
		} else {
			p1->next = p2;
			if (p2) {
				p2->prev = p1;
			} else {
				m_pTail = p1;
			}
		}
		m_nCount--;
		delete pos;
	}

	void RemoveAt(int nIndex) {
		int i;
		POSITION pos = m_pHead;
		for (i=0; i<nIndex; i++) {
			pos = pos->next;
		}

		RemoveAt(pos);
	}

	void RemoveAll() {
		POSITION pos = m_pHead;
		while(pos) {
			POSITION posPrev = pos;
			pos = posPrev->next;
			delete posPrev;
		}
        m_pHead = m_pTail = NULL;
	};

	void *GetNext(POSITION &pos) {
		void *pRet = pos->p;

		pos = pos->next;
		return pRet;
	}
	
	void *GetAt(int nIndex) {
		int i;
		POSITION pos = m_pHead;
		for (i=0; i<nIndex; i++) {
			pos = pos->next;
		}

		return pos->p;
	}

protected:

	POSITION m_pHead;
	POSITION m_pTail;

	int m_nCount;

};

class CTimeSync : public PThread
{
public:
	CTimeSync() {
		time(&m_time);
	};
	~CTimeSync() {
		Terminate();
	};

	void Create(int nPort) {
		m_nPort = nPort;
		Start();
	};

	time_t GetTime() { return m_time; };

protected:

	void Run() {
		int ret, len = sizeof(struct sockaddr_in);
		int sd = socket(PF_INET, SOCK_DGRAM, 0);
		int min = -1;
		char packet[256];

		struct sockaddr_in sin;

		memset(&sin, 0, sizeof(struct sockaddr_in));
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = INADDR_ANY;//INADDR_BROADCAST;
		sin.sin_port = htons(m_nPort);

		//ret = setsockopt(sd, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof(broadcast));
		ret = bind(sd, (struct sockaddr*)&sin, sizeof(sin));
		if (ret < 0) {
			_l("[TS] Failed to bind port\n");
		}

		memset(packet, 0, 32);
		_l("[TS] start\n");
		while(!m_bExit) {
			ret = recvfrom(sd, packet, 256, 0, (sockaddr *)&sin, (socklen_t *)&len);
			if (ret > 0) {
				unsigned int curTime;
				memcpy(&curTime, &packet[5], 4);

				int sec = curTime/30;
				int h, m, s;

				h = (sec/3600);
				m = (sec%3600)/60;
				s = (sec%3600)%60;

				if (min >= 0 && min != m && m != 0) {
					time_t ct;
					struct tm t;

					time(&ct);

					MLT(ct, t);
					if (t.tm_hour != h || t.tm_min != m || t.tm_sec != s) {
						char exec[256];

						_l("[ST] Time is different from APC(sec = %d)\n", sec);
						_l("[ST] %02d:%02d:%02d -> %02d:%02d:%02d\n", t.tm_hour, t.tm_min, t.tm_sec,
								h, m, s);
						sprintf(exec, "date -s %02d:%02d:%02d", h, m, s);
						system(exec);
					}
				}
				min = m;
				//_d("STX, %x, %x, %x, %02d:%02d:%02d\n", packet[0], packet[3], packet[4], h, m, s);
			}
		}
		_d("[TS} end\n");
	};

	void OnTerminate() {}; 

protected:

	int m_nPort;
	time_t m_time;
};

extern CTimeSync g_ts;

typedef struct tagThumb
{
	uint8_t *p;
	int len;
}Thumb;

class CAdFile
{
public:
	CAdFile() {
		int i;
		m_bIsIndexed = false;
		m_bIsReady = false;

		m_llTotalFrames = 0;
		m_llTotalSize = 0;

		for (i=0; i<3; i++) {
			m_th[i].len = 0;
			m_th[i].p = NULL;
		}
	};

	~CAdFile() {
		int i;
		for (i=0; i<3; i++) {
			if (m_th[i].p) {
				delete [] m_th[i].p;
			}
		}
	};

	void ReadFromFile(FILE *fp) {
		int i;
		fread(&m_bIsIndexed, 1, 1, fp);
		fread(&m_bIsReady, 1, 1, fp);
		fread(&m_llTotalFrames, 1, 8, fp);
		fread(&m_llTotalSize, 1, 8, fp);
		fread(m_strFileName, 1, 256, fp);
        //_d("ADFILE %lld, %lld, %s\n", m_llTotalFrames, m_llTotalSize, m_strFileName);
		for (i=0; i<3; i++) {
			Thumb *pt = &m_th[i];
			fread(&pt->len, 1, 4, fp);
			if (pt->len) {
				pt->p = new uint8_t [pt->len];
				fread(pt->p, 1, pt->len, fp);
			}
		}
	};

	void WriteToFile(FILE *fp) {
		int i;
		fwrite(&m_bIsIndexed, 1, 1, fp);
		fwrite(&m_bIsReady, 1, 1, fp);
		fwrite(&m_llTotalFrames, 1, 8, fp);
		fwrite(&m_llTotalSize, 1, 8, fp);
		fwrite(m_strFileName, 1, 256, fp);

		for (i=0; i<3; i++) {
			Thumb *pt = &m_th[i];
			fwrite(&pt->len, 1, 4, fp);
			if (pt->len) {
				fwrite(pt->p, 1, pt->len, fp);
			}
		}
	};

	void SetThumbnail(int nIndex, uint8_t *pJpg, int nSize) {
		if (m_th[nIndex].p) {
			delete [] m_th[nIndex].p;
			m_th[nIndex].p = NULL;
		}

		m_th[nIndex].p = new uint8_t [nSize];
		m_th[nIndex].len = nSize;

		memcpy(m_th[nIndex].p, pJpg, nSize);
		if (nIndex == 2) {
			m_bIsIndexed = true;
		}
	};

	char *GetFileName() { return m_strFileName; };
	long long GetTotalFrames() { return m_llTotalFrames; };
	long long GetTotalSize() { return m_llTotalSize; };

	void SetFileName(char *filename) { sprintf(m_strFileName, "%s", filename); };
	void SetTotalFrames(long long llTotalFrames) {
		m_llTotalFrames = llTotalFrames; };
	void SetTotalSize(long long llTotalSize) {
		m_llTotalSize = llTotalSize; };

	bool IsReady() {
		if (m_bIsReady) {
			return true;
		} else {
			char fullpath[256];
			struct stat sb;

			sprintf(fullpath, "../adfiles/%s", m_strFileName);
			stat(fullpath, &sb);

			if (sb.st_size == m_llTotalSize) {
				m_bIsReady = true;
				return true;
			} else {
				m_llTotalSize = sb.st_size; 
			};
		}

		return false;
	};
	bool IsIndexed() { return m_bIsIndexed; };

protected:
	char m_strFileName[256];

	bool m_bIsIndexed;
	bool m_bIsReady;

	long long m_llTotalFrames;
	long long m_llTotalSize;

	Thumb m_th[3];
};

extern CMyList fl; //file list

#endif // _GLOBAL_H_

