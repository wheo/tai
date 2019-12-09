#include "global.h"

FILE *g_fpLog = NULL;
FILE *g_fpls = NULL;

unsigned long GetFileSize(const char *filename)
{
    int fd; 
    fd = open(filename, O_RDONLY);

    if ( fd < 0) 
    {
        return 0;
    }
    struct stat statbuf;
    if(fstat(fd, &statbuf) < 0)
    {
        close(fd);
        return 0;
    }
    close(fd);
    return statbuf.st_size;
}


void getUUID(char *out_uuid)
{
    int i=0;
    uuid_t uuid;
    uuid_generate_random (uuid);
    char s[37];
    char c;

    uuid_unparse(uuid, s);

    while(s[i])
    {
        s[i] = toupper(s[i]);
        i++;
    }
    memcpy(out_uuid, s, 37);
}

void _l(const char *format, ...)
{
	time_t ct;
	struct tm t;
	if (!g_fpLog) {
		g_fpLog = fopen("log_tai.txt", "a+");
	}

	va_list argptr;
	char buf[1024];

	va_start(argptr, format);
	vsprintf(buf, format, argptr);

	time(&ct);
	MLT(ct, t);

	fprintf(g_fpLog, "[%04d-%02d-%02d, %02d:%02d:%02d] %s", t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, buf);
	fflush(g_fpLog);

	va_end(argptr);
}

void _ls(const char *format, ...)
{
	time_t ct;
	struct tm t;
	if (!g_fpls) {
		g_fpls = fopen("log_sche.txt", "a+");
	}

	va_list argptr;
	char buf[1024];

	va_start(argptr, format);
	vsprintf(buf, format, argptr);

	time(&ct);
	MLT(ct, t);

	fprintf(g_fpls, "[%04d-%02d-%02d, %02d:%02d:%02d] %s", t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, buf);
	fflush(g_fpls);

	va_end(argptr);
}

void MLT(time_t ct, struct tm &t)
{
	struct tm *pt = localtime(&ct);
	memcpy(&t, pt, sizeof(struct tm));
}

unsigned long GetTickCount()
{
	timeval tv;
	static time_t sec = timeStart.tv_.tv_sec;
	static time_t usec = timeStart.tv_.tv_usec;

	gettimeofday(&tv, NULL);
	return (tv.tv_sec - sec)*1000 + (tv.tv_usec - usec)/1000;
}


void GetTime(int nSec, TIME *pt)
{
	int nS = nSec;
	if (nSec >= 86400) {
		nS -= 86400;
		pt->d = 1;
	}

	pt->h = nS/3600;
	pt->m = (nS%3600)/60;
	pt->s = (nS%3600)%60;
}

void GetTime(time_t ct, TIME *pt)
{
   struct tm t;
   MLT(ct, t);

   pt->yy = t.tm_year+1900;
   pt->mm = t.tm_mon+1;
   pt->dd = t.tm_mday;
   pt->h = t.tm_hour;
   pt->m = t.tm_min;
   pt->s = t.tm_sec;
}

void GetTime(time_t ct, int &h, int &m, int &s)
{
   struct tm t;
   MLT(ct, t);

   h = t.tm_hour;
   m = t.tm_min;
   s = t.tm_sec;
}

time_t MakeTime(TIME *curt)
{
    struct tm st;

    memset(&st, 0, sizeof(struct tm));

    st.tm_year = curt->yy - 1900;
    st.tm_mon = curt->mm - 1;
    st.tm_mday = curt->dd;
    st.tm_hour = curt->h;
    st.tm_min = curt->m;
    st.tm_sec = curt->s;
	st.tm_isdst = 0;

    return mktime(&st); 
}

int sr(int sd, char *p, int len)
{
	int w = 0, local = len;

	while(local) {
		int r = recv(sd, &p[w], local, 0);
		if (r > 0) {
			w += r;
			local -= r;
		} else {
			return -1;
		}
	}

	return w;
}

int GetHour(int nSec) { return (nSec/3600); }
int GetMinute(int nSec) { return ((nSec%3600)/60); }
int GetSecond(int nSec) { return ((nSec%3600)%60); }
int GetSecond(TIME t) { return t.h*3600 + t.m*60 + t.s; };
#if 0
int UTF8toWCS(wchar_t *wcstr, const char *utf8str, size_t count)
{
	size_t wclen = 0;
	int utflen, i;
	wchar_t ch;

	while(*utf8str && (wcstr == NULL || wclen < count)) {
		utflen = 
int UTF8toMBS(char *mbstr, const char *utf8str, size_t count)
{
	wchar_t *wcs;
	size_t wcsize;

	int n;

	wcsize = strlen(utf8str) + 1;
	wcs = (wchar_t *)malloc(wcsize * sizeof(wchar_t));
};
#endif
PThread::PThread(char *a_szName , THREAD_EXIT_STATE a_eExitType)
{
        m_nID = 0;
        m_eState = eREADY;
        m_eExitType = a_eExitType;
        m_szName = NULL;

        if (a_szName) {
                int nLength =strlen(a_szName);
                m_szName = new char[nLength + 1];
                strcpy(m_szName, a_szName);
        }
}

PThread::~PThread()
{
        if (m_eState == eZOMBIE) {
                Join(m_nID);
        }

        if (m_szName)
                delete []m_szName;
}

int PThread::Start()
{
        int nResult = 0;
        m_bExit = false;

        if ((pthread_create(&m_nID, NULL, StartPoint, reinterpret_cast<void *>(this))) == 0) {
                if (m_eExitType == eDETACHABLE) {
                        pthread_detach(m_nID);
                }
        } else {
                SetState(eABORTED);
                nResult = -1;
        }

        return nResult;
}

void* PThread::StartPoint(void* a_pParam)
{
        PThread* pThis = reinterpret_cast<PThread*>(a_pParam);
        pThis->SetState(eRUNNING);

        pThis->Run();

        if (pThis->GetExitType() == eDETACHABLE) {
                pThis->SetState(eTERMINATED);
        } else {
                pThis->SetState(eZOMBIE);
        }

        pthread_exit((void*)NULL);
        return NULL;
}

void PThread::Terminate()
{
    m_bExit = true;
    if (m_eState == eRUNNING) {
        if (m_eExitType == eJOINABLE) {
            OnTerminate();
            Join(m_nID);
        } else if (m_eExitType == eDETACHABLE) {
            OnTerminate();
        }
    } else if (m_eState == eZOMBIE) {
        Join(m_nID);
    }
}

// this function will be blocked until the StartPoint terminates
void PThread::Join(pthread_t a_nID)
{
    if (!pthread_join(a_nID, NULL)) {
        SetState(eTERMINATED);
    } else {
        printf("Failed to join thread [%s: %d]\n", __FUNCTION__, __LINE__);
    }
}

void PThread::SetState(THREAD_STATE a_eState)
{
        m_eState = a_eState;
}

THREAD_STATE PThread::GetState() const
{
        return m_eState;
}

THREAD_EXIT_STATE PThread::GetExitType() const
{
        return m_eExitType;
}

char* PThread::GetName() const
{
        return m_szName ? m_szName  : NULL;
}

bool PThread::IsTerminated() const
{
        return m_eState == eTERMINATED  ?  true : false;
}

bool PThread::IsRunning() const
{
        return m_eState == eRUNNING  ?  true : false;
}

