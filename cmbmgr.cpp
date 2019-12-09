#include "global.h"
#include "cmbmgr.h"
#include "comms.h"
#include "scheduling.h"
#include "apcproc.h"
#include "main.h"

#define THRESH_DIFF_PROG		3600

#if 0
void GetRunCode(char *pClipId, CMB *pc)
{
	char temp[32];

	char *ps = pClipId;
	char *pd = pc->clipid;

	int start = 0, run = 0, epi = 0;
	for (int i=0; i<(int)strlen(ps); i++) {
		if (ps[i] == '-') {
			start = i;
			break;
		}
		pd[i] = ps[i];
	}
	pd[start] = 0; // etc. H453

	int cnt = 0;
	for (int i=start+1; i<(int)strlen(ps); i++) {
		if (ps[i] == 'P') {
			temp[cnt] = 0;
			run = atoi(temp);
			epi = atoi(&ps[i+1]);
		}
		temp[cnt++] = ps[i];
	}

	pc->run = run;
	pc->epi = epi;
}
#endif

CCmbMgr::CCmbMgr()
{
    pthread_mutex_init(&m_mutex, NULL);
    for (int i=0; i<MAX_NUM_CMB; i++) {
        m_pCMB[i] = NULL;
    }
    m_bIsUpdate = false;
}

CCmbMgr::~CCmbMgr()
{
    Terminate();

    for (int i=0; i<MAX_NUM_CMB; i++) {
        if (m_pCMB[i]) {
            delete m_pCMB[i];
        }
        m_pCMB[i] = NULL;
    }
    pthread_mutex_destroy(&m_mutex);
}

void CCmbMgr::Clear()
{
    pthread_mutex_lock(&m_mutex);
    for (int i=0; i<MAX_NUM_CMB; i++) {
        if (m_pCMB[i]) {
            delete m_pCMB[i];
        }
        m_pCMB[i] = NULL;
    }

    pthread_mutex_unlock(&m_mutex);
}

#if 0

bool CCmbMgr::IsNextRun(CMB *pp, CMB *pc)
{
    int diff = (int)(pp->st - pc->st);
#if 0
    if ((strcmp(pp->title, pc->title) == 0) &&
            (strcmp(pp->clipid, pc->clipid) == 0) &&
            (abs(diff) < THRESH_DIFF_PROG)) {
        //_d("%s, %s, %d, %d / %s, %s, %d, %d(%d)\n", pp->title, pp->clipid, pp->run, pp->epi, pc->title, pc->clipid, pc->run, pc->epi, diff);
        if ((pp->run+1) == pc->run) {
            return true;
        }
    }
#else
    if ((strlen(pp->clipid) != 0) && 
            (strcmp(pp->clipid, pc->clipid) == 0) &&
            (abs(diff) < THRESH_DIFF_PROG)) {
        //_d("IsNextRun ??? %s, %s, %d, %d / %s, %s, %d, %d(%d)\n", pp->title, pp->clipid, pp->run, pp->epi, pc->title, pc->clipid, pc->run, pc->epi, diff);
        if ((pp->run+1) == pc->run) {
            _l("NextRun > %s, %s, %d, %d / %s, %s, %d, %d(%d)\n", pp->title, pp->clipid, pp->run, pp->epi, pc->title, pc->clipid, pc->run, pc->epi, diff);
            return true;
        }
    }
#endif
    return false;
}

#endif

void CCmbMgr::Put(CMB *pc)
{
    int nEmpty = -1;
    CMB *pd = NULL;

    for (int i=0; i<MAX_NUM_CMB; i++)
    {
        if (m_pCMB[i])
        {
            //_d("m_pCMB[%d] %p\n", i, m_pCMB[i]);
            CMB *ps = m_pCMB[i];
            int diff = (int)(ps->st - pc->st);
            //_d("diff = %d\n", diff);
#if 0
            if ((strcmp(ps->clipid, pc->clipid) == 0) &&
                    (ps->run == pc->run) &&
                    (abs(diff) < THRESH_DIFF_PROG))
            {
#endif
            if(0)
                {

                CMB temp;
                if (strcmp(ps->title, pc->title))
                {
                    _l("Maybe same program (%s / %s)\n", ps->title, pc->title);
                }
#if 0
                if (ps->epi != pc->epi)
                {
                    _l("Same program, but different episode (%s, %s, %d, %d/%d)\n", pc->title, pc->clipid, pc->run, pc->epi, ps->epi);
                }
#endif
                pthread_mutex_lock(&m_mutex);
                memcpy(&temp, ps, sizeof(CMB));
                memcpy(ps, pc, sizeof(CMB));
                for (int k=0; k<temp.num; k++)
                {
                    ps->cm[k].status = temp.cm[k].status;
                }
                pthread_mutex_unlock(&m_mutex);
                return;
            }
        }
        else 
        {
            if (nEmpty < 0)
            {
                nEmpty = i;
                //_d("nEmpty %d\n", nEmpty);
            }
        }
    }

    //_d("새로운 CMB를 발견했어\n");

    pd = new CMB;
    memcpy(pd, pc, sizeof(CMB));

    pthread_mutex_lock(&m_mutex);
    if (nEmpty < 0) {
        delete m_pCMB[0];
        for (int i=0; i<MAX_NUM_CMB-1; i++) {
            m_pCMB[i] = m_pCMB[i+1];
        }
        m_pCMB[MAX_NUM_CMB-1] = pd;
    } else {
        m_pCMB[nEmpty] = pd;
        _d("put done (%d) title %s\n", nEmpty, m_pCMB[nEmpty]->title);
    }
    m_bIsUpdate = true;
    pthread_mutex_unlock(&m_mutex);
}


bool CCmbMgr::Get(int nIndex, CMB *pc)
{
    bool bRet = false;

    pthread_mutex_lock(&m_mutex);
    if (m_pCMB[nIndex]) {
        memcpy(pc, m_pCMB[nIndex], sizeof(CMB));
        bRet = true;
    }
    pthread_mutex_unlock(&m_mutex);

    return bRet;
}

void CCmbMgr::Set(int nIndex, int nCmb, int nStatus)
{
    pthread_mutex_lock(&m_mutex);
    if (m_pCMB[nIndex]) {
        m_pCMB[nIndex]->cm[nCmb].status = nStatus;
    }
    pthread_mutex_unlock(&m_mutex);
}

void CCmbMgr::Run()
{
	int nOld = -1;
	GD *pg = (GD *)m_pGD;
	CCommSvr *pSvr = (CCommSvr *)pg->pSvr;

    while(!m_bExit) {
		bool isFirst = true;
		time_t ct;
		time(&ct);
	
        if (m_bIsUpdate) {
			//system("clear");
			m_bIsUpdate = false;
		}
		//printf("\033[20;0H");
        pthread_mutex_lock(&m_mutex);
		for (int i=0, cnt=0; i<MAX_NUM_CMB; i++) {
			CMB *pc = m_pCMB[i];
			if (pc)
            {
                if (pc->st - ct < 0)
                {
                    continue;
                }

				//if ((pc->st - ct) >= -7200) {
                if(1)
                {
					if (cnt != nOld && isFirst)
                    {
						//system("clear");
						//printf("\033[20;0H");
						nOld = cnt;
					}
					isFirst = false;

					_d("[%d] CM Program : %s, (%d)\n", i, pc->title, (int)(pc->st - ct));
					for (int j=0; j<pc->num; j++)
                    {
						CMI *pi = &pc->cm[j];
						int h, m, s;
						GetTime(pi->st, h, m, s);

						_d("CM Band %d - Type : %d (%d), State : %d Start : %02d:%02d:%02d, dur : %d\n", j, pi->type, pi->index, pi->status, h, m, s, pi->dur);
					}
					cnt++;
					pSvr->SendSche(cnt, pc);
				}
			}
			if (cnt >= 20)
            {
				break;
			}
		}
        pthread_mutex_unlock(&m_mutex);

		g_pLicMgr->Print();
		//printf("\033[0;0H");

        sleep(5);
    }

	_l("[MGR] stop thread\n");
}

CLicMgr::CLicMgr()
{
    pthread_mutex_init(&m_mutex, NULL);
    for (int i=0; i<MAX_NUM_CMB; i++) {
        m_pCMB[i] = NULL;
    }
    m_bIsUpdate = false;
    m_pCur = NULL;
	m_nForceCount = 0;
}

CLicMgr::~CLicMgr()
{
    for (int i=0; i<MAX_NUM_CMB; i++) {
        if (m_pCMB[i]) {
            delete m_pCMB[i];
        }
        m_pCMB[i] = NULL;
    }
    pthread_mutex_destroy(&m_mutex);

	_l("[LIC] deleted\n");
}

CMB *CLicMgr::IsStart(time_t ct)
{
	if (m_pCur) {
		return NULL;
	}

    CMB *pnl = NULL;

    pthread_mutex_lock(&m_mutex);
    for (int i=0; i<MAX_NUM_CMB; i++) {
        CMB cmb;
        if (g_pLicMgr->Get(i, &cmb)) {
            if (cmb.st <= ct && cmb.ed > ct) {
                pnl = &cmb;
                break;
            }
        }
    }
    if (pnl) {
        m_pCur = pnl;
    }
    pthread_mutex_unlock(&m_mutex);

    return pnl;
}

bool CLicMgr::IsStop(time_t ct)
{
   if (m_pCur) {
       pthread_mutex_lock(&m_mutex);
       if (m_pCur->ed <= ct) {
           m_pCur = NULL;
       }
       pthread_mutex_unlock(&m_mutex);
       if (m_pCur == NULL) {
           return true;
       }
   }

   return false;
}
#if 0
void CLicMgr::ForceStart()
{
	if (m_pCur == NULL) {
		CMB cmb;
		time_t ct;

		time(&ct);
		memset(&cmb, 0, sizeof(CMB));

		sprintf(cmb.title, "unlicensedByForce");
		sprintf(cmb.clipid, "UBF");
		cmb.run = m_nForceCount++;
		cmb.type = 1;
		cmb.st = ct;
		cmb.ed = ct + 18000; // max 5 hour

		Put(&cmb);
	}
}

void CLicMgr::ForceStop()
{
	time_t ct;
	time(&ct);
	if (m_pCur) {
		pthread_mutex_lock(&m_mutex);
		for (int i=0; i<MAX_NUM_CMB; i++) {
			CMB cmb;
			if (g_pLicMgr->Get(i, &cmb)) {
				CMB *ps = m_pCur;
				CMB *pd = m_pCMB[i];
				if ((strcmp(pd->title, ps->title) == 0) &&
					(strcmp(pd->clipid, ps->clipid) == 0) &&
					(pd->run == ps->run)) {
					pd->ed = ct;
					break;
				}
			}
		}
		pthread_mutex_unlock(&m_mutex);
		m_pCur->ed = ct;
	}
}
#endif

void CLicMgr::Put(CMB *pc)
{
    int nEmpty = -1;

    int h1, m1, s1;
    int h2, m2, s2;

    CMB *pd = NULL;

    struct tm t;
	
	MLT(pc->st, t);

    h1 = t.tm_hour;
    m1 = t.tm_min;
    s1 = t.tm_sec;

    MLT(pc->ed, t);

    h2 = t.tm_hour;
    m2 = t.tm_min;
    s2 = t.tm_sec;

    for (int i=0; i<MAX_NUM_CMB; i++) {
        if (m_pCMB[i]) {
			pd = m_pCMB[i];
            if ((strcmp(pd->title, pc->title) == 0))
            {
                    
				//_l("[LICENSED] same as prev. cmb (%s, %s, %d / %s, %s, %d)\n", pc->title, pc->clipid, pc->run, pd->title, pd->clipid, pd->run);
                if (pd->ed != pc->ed) {
                    MLT(pd->ed, t);

                    h1 = t.tm_hour;
                    m1 = t.tm_min;
                    s1 = t.tm_sec;
                    _l("[LICENSED] end time is changed (%02d:%02d:%02d -> %02d:%02d:%02d)\n", h1, m1, s1, h2, m2, s2);
                }

                pthread_mutex_lock(&m_mutex);
				memcpy(pd, pc, sizeof(CMB));
				if (m_pCur) {
					if ((strcmp(m_pCur->title, pc->title) == 0))
                    {
						m_pCur->ed = pc->ed;
					}
				}
                pthread_mutex_unlock(&m_mutex);
                return;
            }
        } else {
            if (nEmpty < 0) {
                nEmpty = i;
            }
        }
    }

    pd = new CMB;
    memcpy(pd, pc, sizeof(CMB));

    pthread_mutex_lock(&m_mutex);
    if (nEmpty < 0) {
        delete m_pCMB[0];
        for (int i=0; i<MAX_NUM_CMB-1; i++) {
            m_pCMB[i] = m_pCMB[i+1];
        }
        m_pCMB[MAX_NUM_CMB-1] = pd;
    } else {
        m_pCMB[nEmpty] = pd;
    }
    m_bIsUpdate = true;
    pthread_mutex_unlock(&m_mutex);
}

bool CLicMgr::Get(int nIndex, CMB *pc)
{
    bool bRet = false;

    if (m_pCMB[nIndex]) {
        memcpy(pc, m_pCMB[nIndex], sizeof(CMB));
        bRet = true;
    }
    return bRet;
}

void CLicMgr::Print()
{
	time_t ct;
	bool isFirst = true;

	time(&ct);

	pthread_mutex_lock(&m_mutex);
	for (int i=0, cnt=0; i<MAX_NUM_CMB; i++) {
		CMB *pc = m_pCMB[i];
		if (pc && (pc->st - ct) >= -1800) {
			struct tm st;
			struct tm ed;

			if (isFirst) {
				_d("\r\nNot licensed contents\n");
			}
			isFirst = false;

			MLT(pc->st, st);
			MLT(pc->ed, ed);

			_d("[%d] Program : %s (%d)\n", i, pc->title, (int)(pc->ed - ct));
			_d(" > (%02d/%02d) %02d:%02d:%02d ~  %02d:%02d:%02d\n", st.tm_mon+1, st.tm_mday, 
					st.tm_hour, st.tm_min, st.tm_sec, ed.tm_hour, ed.tm_min, ed.tm_sec);
			cnt++;
		}
		if (cnt >= 3) {
			break;
		}
	}
	pthread_mutex_unlock(&m_mutex);
}

