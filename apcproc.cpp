#include "global.h"
#include "scheduling.h"
#include "cmbmgr.h"
#include "apcproc.h"
#include "comms.h"
#include "main.h"
#include "tinyxml.h"

#include <poll.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <dirent.h>

#define EVT_SIZE		(sizeof(struct inotify_event))
#define BUF_LEN			(1024*(EVT_SIZE + 16))

typedef struct
{
    long t;
    int num;
}DAT;

CApcProc::CApcProc()
{

}

CApcProc::~CApcProc()
{
	Terminate();
}

void CApcProc::Create(void *pGD)
{
	m_pGD = pGD;

	Start();
}

void CApcProc::Run()
{
    int msgid = 0;
	time_t ct, ot;
	char filename[256];
    strcpy(filename, (char*)"./sche.xml");

	GD *pg = (GD *)m_pGD;
	CCmbMgr *pMgr = (CCmbMgr *)pg->pMgr;

	//CApcRecv ar;
	//ar.Create();

	CApcParser ap;
    //CApcUpload au;
	
	//time(&ct);
	//ot = ct;

    //최초 ADMIXER 실행시 APC
    if (GetFileSize(filename) > 8192 )
    {
        ap.Parse(filename, pMgr);
    }

    msgid = msgget((key_t)33000, IPC_CREAT | 0666);
    _d("[APM] msgid is %d\n", msgid);

	_d("[APM] start loop\n");
	while(!m_bExit)
    {
        DAT d;
        //_d("[APM] Waiting for ... msgqueue DAT(%d)\n", d.num);
        
        if (msgrcv(msgid, &d, sizeof(DAT) - sizeof(long), 0, IPC_NOWAIT) >=0)
        {
            _d("[SCHE] new schedule has been received\n");
            if (GetFileSize(filename) > 8192 )
            {
                ap.Parse(filename, pMgr);
            }
        }

		usleep(500000);
	}
    //ap.Parse(filename, pMgr);
	_d("[APM] exit loop\n");
}

void CApcUpload::Upload(char *filename)
{
    sprintf(m_strFileName, "%s", filename);
    Start();
}

void CApcUpload::Run()
{
    ftpupload(m_strFileName);
}

CApcParser::CApcParser()
{
	m_bIsComplete = false;
}

CApcParser::~CApcParser()
{
	Terminate();
}

void CApcParser::Parse(char *filename, void *pMgr)
{
	m_bIsComplete = false;
	Terminate();

	//sprintf(m_strFileName, "/home/%s/%s", g_channel, filename);
    sprintf(m_strFileName, "%s", filename);

	m_pMgr = pMgr;

	Start();
}

void CApcParser::Run()
{
    _d("[APC] Parse start...\n");
	TiXmlDocument doc(m_strFileName);
	CCmbMgr *pMgr = (CCmbMgr *)m_pMgr;

    pMgr->Clear();

	//char arrEle[7][64] = { "EventType", "EventName", "OnAirDate", "StartTime", "OnairDur", "ClipID", "CopyRight" };
    
    char running_date[16];

	if (doc.LoadFile()) {
		CMB cmb;
		CMB cmb_nl;

		CMI *pi = NULL;

        bool b_Selected = false;

		//TiXmlNode *pNodeEI = doc.FirstChild("EPGData")->FirstChild("EventInfo");
        TiXmlNode *pNodeEI = doc.FirstChild("APC_Message")->FirstChild("response");
        TiXmlElement *pE_response = pNodeEI->ToElement();
        sprintf(running_date, "%s", pE_response->Attribute("running_date"));

        pNodeEI = doc.FirstChild("APC_Message")->FirstChild("response")->FirstChild("schedule")->FirstChild("event_data");
        int tempcnt = 0; 
        _d("before while pNodeEI (%p)\n", pNodeEI);

		while(!m_bExit && pNodeEI)
        {
            tempcnt++;
            char pre_pType[32];
            char pre_Title[256];
            char eventType[32];

			char *pType, *pTitle, *pClipId;
			char arrResult[7][256];

            char starttime[16];
            char endtime[16];

            char title[256];

			TIME t;
			time_t st, ed;
            int dur;
			int sh, sm, ss, sms;
			int eh, em, es, ems;

            TiXmlElement *pe = pNodeEI->ToElement();

            sprintf(title, "%s", pe->Attribute("program_title"));
            sprintf(eventType, "%s", pe->Attribute("broadcast_event_kind"));
            sprintf(starttime, "%s", pe->Attribute("running_start_time"));
            sprintf(endtime, "%s", pe->Attribute("running_end_time"));

            //_d("[PARSE] (%d) Title : %s, eventType : %s, starttime : %s, endtime : %s\n", tempcnt, title, eventType, starttime, endtime);

			pType = arrResult[0];
			pTitle = arrResult[1];

            // 2 : OnAirDate, 3 : StartTime(HH:mm:ss:Frame), 4 : OnairDur(HH:mm:ss:Frame)
#if 0
			sscanf(arrResult[2], "%04d%02d%02d", &t.yy, &t.mm, &t.dd);
			sscanf(arrResult[3], "%02d:%02d:%02d:%02d", &t.h, &t.m, &t.s, &t.d);
			sscanf(arrResult[4], "%02d:%02d:%02d:%02d", &h, &m, &s, &ms);
#endif
            sscanf(running_date, "%4d%02d%02d", &t.yy, &t.mm, &t.dd);
            sscanf(starttime, "%02d%02d%02d%02d", &t.h, &t.m, &t.s, &t.d);
            sscanf(starttime, "%02d%02d%02d%02d", &sh, &sm, &ss, &sms);
			st = MakeTime(&t);
            sscanf(endtime, "%02d%02d%02d%02d", &t.h, &t.m, &t.s, &t.d);
            sscanf(endtime, "%02d%02d%02d%02d", &eh, &em, &es, &ems);
			ed = MakeTime(&t);
			//pClipId = arrResult[5];
#if 0
			_d("%s, %s, %02d:%02d:%02d\n", pType, pTitle, t.h, t.m, t.s);
#endif
			//st = MakeTime(&t);
			//dur = (h*3600 + m*60 + s)*30 + ms;
            dur = ( (eh*3600 + em*60 + es) * 30 + ems ) - ( (sh*3600 + sm*60 + ss) * 30 + sms);
            // dur is frame

            memset(&cmb, 0, sizeof(CMB));
            pi = &cmb.cm[0];
            memset(pi, 0, sizeof(CMI));
            sprintf(cmb.title, "%s", title);

			if (strcmp("광고", eventType) == 0)
            {
                _d("[APC] 공통CM\n");
                pi->type = 3;
                pi->index = 1;
               
                pi->st = st;
                pi->ed = ed;
                pi->ms = (int)(t.d*33.3333333);

                // increase duration
                pi->dur += dur;
                // status 1 - not used, 2 - using, 3 - complete
                pi->status = 1;
                cmb.index = 1;
                cmb.num = 1;
                cmb.st = st;
                cmb.type = 0;
            }
            if(strcmp("전 CM", eventType) == 0)
            {
                _d("전CM\n");
                pi->type = 0;
                pi->index = 0;

                pi->st = st;
                pi->ed = ed;
                pi->ms = (int)(t.d*33.3333333);

                pi->dur += dur;

                // status 1 - not used, 2 - using, 3 - complete
                pi->status = 1;
                cmb.index = 1;
                cmb.num = 1;
                cmb.st = st;
                cmb.type = 0;
            }
            if(strcmp("후 CM", eventType) == 0)
            {
                _d("후CM\n");
                pi->type = 1;
                pi->index = 0;

                pi->st = st;
                pi->ed = ed;
                pi->ms = (int)(t.d*33.3333333);
                pi->dur += dur;

                // status 1 - not used, 2 - using, 3 - complete
                pi->status = 1;
                cmb.index = 1;
                cmb.num = 1;
                cmb.st = st;
                cmb.type = 0;
            }

            _d("CM type : %d\n", pi->type);
            pMgr->Put(&cmb);

            _d("CM duration frame is %d \n", pi->dur);
            _d("------------------------------------------------------------\n");

			pNodeEI = pNodeEI->NextSibling();
		}
	}
    else
    {
        _l("%s is not loaded\n", m_strFileName);
    }

	m_bIsComplete = true;
}

CApcRecv::CApcRecv()
{
	pthread_mutex_init(&m_m, NULL);

	m_bIsNewFile = false;
	memset(m_strNewFile, 0, 256);
}

CApcRecv::~CApcRecv()
{
	Terminate();

	pthread_mutex_destroy(&m_m);
}

void CApcRecv::Create()
{
	sprintf(m_strPath, "/home/%s", g_channel);
	Start();
}

void CApcRecv::Run()
{
	int wd, fd;

	char filename[256];
	char buffer[BUF_LEN];

	fd = inotify_init1(IN_NONBLOCK);
	if (fd < 0) {
		_d("[AR] Failed to inotify_init\n");
	}

	memset(m_strNewFile, 0, 256);
	memset(m_strOldFile, 0, 256);

    FindLatestFile();

	_d("[AR] add watch %s\n", m_strPath);
	wd = inotify_add_watch(fd, m_strPath, IN_CLOSE | IN_CREATE | IN_DELETE);

	_d("[AR] start loop\n");
	while(!m_bExit) {
		int rd = 0;
		int len = read(fd, buffer, BUF_LEN);
		if (len < 0) {
			//_d("[AR] Failed to read inotify\n");
			usleep(100000);
			continue;
		} 

		while(rd < len)
        {
			struct inotify_event *pe = (struct inotify_event *)&buffer[rd];
			if (pe->len)
            {
				if (!(pe->mask & IN_ISDIR))
                {
					if (pe->mask & IN_CREATE)
                    {
						_ls("[AR] FILE %s was created\n", pe->name);
						sprintf(filename, "%s", pe->name);
					}
					if (pe->mask & IN_DELETE)
                    {
                        _l("[AR] FILE %s was deleted\n", pe->name);
					}
					if (pe->mask & IN_CLOSE)
                    {
						//_d("[AR] %s was closed\n", pe->name);
						if (strcmp(filename, pe->name) == 0)
                        {
							pthread_mutex_lock(&m_m);

							if (strlen(m_strOldFile))
                            {

							}
							sprintf(m_strOldFile, "%s", m_strNewFile);
							sprintf(m_strNewFile, "%s", pe->name);
							//_d("m_strOldFile : %s m_strNewFile : %s\n", m_strOldFile, m_strNewFile);
								
							memset(filename, 0, 256);

							_ls("[AR] new apc XML updated (%s)\n", pe->name);
							m_bIsNewFile = true;
							pthread_mutex_unlock(&m_m);
						}
					}
				}
			}
            rd += (EVT_SIZE + pe->len);
		}
	}
	_d("[AR] exit loop\n");
}

time_t FindDateAndTime(char *ps)
{
	int len = strlen(ps);
	if (strcmp(&ps[len-3], "xml") == 0) {
		int yy, mm, dd, h, m, s;
		sscanf(&ps[len-19], "%04d%02d%02d_%02d%02d%02d", &yy, &mm, &dd, &h, &m, &s);

		//_d("%04d-%02d-%02d, %02d:%02d:%02d\n", yy, mm, dd, h, m, s);
		struct tm st;
		memset(&st, 0, sizeof(struct tm));

		st.tm_year = yy - 1900;
		st.tm_mon = mm - 1;
		st.tm_mday = dd;
		st.tm_hour = h;
		st.tm_min = m;
		st.tm_sec = s;
		st.tm_isdst = 0;

		return mktime(&st);
	}

	return 0;
}

void CApcRecv::FindLatestFile()
{
	DIR *dir_info;
	struct dirent *dir_entry;

	time_t to = 0;

	char latest_file[256];

	dir_info = opendir(m_strPath);
	if (dir_info) {
		while(dir_entry = readdir(dir_info)) {
			time_t tc = FindDateAndTime(dir_entry->d_name);
			if (tc) {
				if (tc > to) {
					to = tc;
					sprintf(latest_file, "%s", dir_entry->d_name);
				}
			}
		}
		closedir(dir_info);
	}

	if (to) {
		sprintf(m_strNewFile, "%s", latest_file);
		m_bIsNewFile = true;

		_d("[AR] Find latest APC file : %s\n", m_strNewFile);
	}
}

