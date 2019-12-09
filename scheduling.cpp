#include "global.h"
#include "scheduling.h"
#include "tinyxml.h"
#include "cmbmgr.h"
#include "apcproc.h"
#include "comms.h"
#include "main.h"

#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>

#define ADDITIONAL      0

typedef struct {
	long t;
	int num;
}DAT;

CMySchedule::CMySchedule()
{
	pthread_mutex_init(&m_mutex, NULL);
	for (int i=0; i<MAX_NUM_SCHEDULES; i++) {
		m_pSche[i] = NULL;
	}
    m_bIsUpdate = false;
    for (int i=0; i<MAX_NUM_INDEXED; i++) {
        m_pAdFile[i] = NULL;
    }
	memset(&m_pl, 0, sizeof(PL));
}

CMySchedule::~CMySchedule()
{
    Terminate();
	for (int i=0; i<MAX_NUM_SCHEDULES; i++) {
		if (m_pSche[i]) {
			delete m_pSche[i];
			m_pSche[i] = NULL;
		}
	}
    for (int i=0; i<MAX_NUM_INDEXED; i++) {
        if (m_pAdFile[i]) {
            delete m_pAdFile[i];
            m_pAdFile[i] = NULL;
        }
    }
	pthread_mutex_destroy(&m_mutex);
}

CAdFile *CMySchedule::GetAdFile(char *filename)
{
	CAdFile *paf = NULL;
    pthread_mutex_lock(&m_mutex);
    for (int i=0; i<MAX_NUM_INDEXED; i++) {
        paf = m_pAdFile[i];
        if (paf) {
            if (strncasecmp(paf->GetFileName(), filename, strlen(filename)) == 0) {
                pthread_mutex_unlock(&m_mutex);
                return paf;
            }
        }
	}
    pthread_mutex_unlock(&m_mutex);
	return NULL;
}

void CMySchedule::UpdateAdFiles()
{
    int nFiles = 0;

    pthread_mutex_lock(&m_mutex);
    for (int i=0; i<MAX_NUM_INDEXED; i++) {
        if (m_pAdFile[i]) {
            delete m_pAdFile[i];
            m_pAdFile[i] = NULL;
        }
    }

	FILE *fp = fopen("adfiles.idx", "rb");
	while (fp) {
		CAdFile *pa;
		int nAdFiles = 0;

		if (fread(&nAdFiles, 1, 4, fp) <= 0) {
			_d("[INDEXING] EOF\n");
			break;
		}
		if (nAdFiles == -1) {
			fclose(fp);
			break;
		}

		pa = new CAdFile;
		pa->ReadFromFile(fp);
	
        m_pAdFile[nFiles++] = pa;
	}
    pthread_mutex_unlock(&m_mutex);
}

void CMySchedule::Run()
{

    bool bIsUpdateAdFiles = false;

    int first = 0;
	int msgid, count, target = 12*5; 
    time_t ot;
    time_t ct;

	GD *pg = (GD *)m_pGD;
	CCmbMgr *pMgr = (CCmbMgr *)pg->pMgr;

	CAL cal;
	memset(&cal, 0, sizeof(CAL));

    UpdateAdFiles();
    LoadFile((char*)"test.xml");
    //Print();

	msgid = msgget((key_t)1006, IPC_CREAT | 0666);
	count = target-1;

    time(&ct);
    ot = ct;

	_l("[SHCE] start loop...\n");
	while(!m_bExit) {
		DAT d;
		time(&ct);
		struct tm t;

        if (ct != ot)
        {
            ot = ct;
        }
        else
        {
            continue;
        }

		MLT(ct, t);

		count++;
        if (count == (target - 10)) {
            system("./ftpCommon &");
            system("./ftpPrePost &");
            //_d("[SCHE] downloaded XMLs from FTP server\n");
        }

        if (msgrcv(msgid, &d, sizeof(DAT) - sizeof(long), 0, IPC_NOWAIT) >= 0) {
            _l("[SCHE] ADfile update has been scheduled at %02d:%02d:%02d\n", t.tm_hour, t.tm_min, t.tm_sec);
            bIsUpdateAdFiles = true;
        }

        pthread_mutex_lock(&pg->csSche);
        if (count >= target && m_pl.is_used == 0) {
            _d("[SCHE] Load XML file\n");
            LoadFile((char*)"test.xml");
            _d("[SCHE] Update common list\n");
            UpdateCommonList(&cal);

            count = 0;
            _d("[SCHE] schedule changed at %02d:%02d:%02d\n", t.tm_hour, t.tm_min, t.tm_sec);

            //Print();
        }

        if (bIsUpdateAdFiles && m_pl.is_used == 0)
        {
            _l("[SCHE] ADfiles update at %02d:%02d:%02d\n", t.tm_hour, t.tm_min, t.tm_sec);
            UpdateAdFiles();
            UpdateCommonList(&cal);
            bIsUpdateAdFiles = false;
        }
        pthread_mutex_unlock(&pg->csSche);


        if (1)
        {
            CMB cmb;
            CMI *pi = NULL;
            bool bFound = false;

            for (int i=0; i<MAX_NUM_CMB; i++)
            {
                if (pMgr->Get(i, &cmb))
                {
                    for (int j=0; j<cmb.num; j++)
                    {
                        pi = &cmb.cm[j];

						int diff = abs(pi->st - ct);
                        if ((diff <= 3) &&  pi->status == 1)
                        {
                            // 이 조건에 맞으면 using으로 status 변경
							int h, m, s;
							GetTime(pi->st, h, m, s);
                            _d("[SCHE] proper CM band %s, %02d:%02d:%02d - %d\n", cmb.title, h, m, s, pi->type);
                            _l("[SCHE] proper CM band %s, %02d:%02d:%02d - %d\n", cmb.title, h, m, s, pi->type);
                            pMgr->Set(i, j, 2);
                            bFound = true;
                            break;
                        }
                    }
                }
                if (bFound)
                { 
                    break;
                }
            }
#if 1
            if (bFound)
            {
                SCHE *ps = NULL;
				for (int i=0; i<MAX_NUM_SCHEDULES; i++)
                {
                    _d("m_pSche (%p)\n", m_pSche[i]);
					if (m_pSche[i])
                    {
						SCHE *pst = m_pSche[i];

                        ps = pst;
                       
                        break;

#if 0
						if ((strcmp(pst->title, cmb.title) == 0) &&
							(strcmp(pst->clip, cmb.clipid) == 0))
                        {
							if ((pst->run == cmb.run))
                            {
								if (pi->type == 0)
                                {
									if (pst->epi <= 1) 
                                    { // CMB is in 1st. EPISODE's pre_ad list
										ps = pst;
										_l("[SCHE] 1. %s, %s, %d, %d\n", pst->title, pst->clip, pst->run, pst->epi);
										break;
									}
								}
                                else if (pi->type == 1) 
                                { // CMA is in last EPSISODE's post_ad list
									ps = pst;
									_l("[SCHE] 2. %s, %s, %d, %d\n", pst->title, pst->clip, pst->run, pst->epi);
								}
                                else
                                { // CMM is in each EPSIODE's mid_ad list
                                    ps = pst;
									if (pst->epi == pi->index)
                                    {
										_l("[SCHE] 3. %s, %s, %d, %d\n", pst->title, pst->clip, pst->run, pst->epi);
										break;
									}
                                    else
                                    {
										_l("[SCHE] 3. EPI is different (%d/%d)\n", pst->epi, pi->index);
                                    }
								}
							}
                            else
                            {
								_l("[SCHE] Run is different from schedule (%d/%d)\n", pst->run, cmb.run);
							}
						}
#endif
					}
                }

                if (ps) 
                {
                    int tot_dur, target_dur;
                    int files = 0;

                    time_t tt;
					TIME start;
					PL *pl = &m_pl;

					pthread_mutex_lock(&pg->csSche);

					if (pi->type == 0)
                    {
						tt = pi->st + g_cmb_delay/1000;
					}
                    else if (pi->type == 1)
                    {
						tt = pi->st + g_cma_delay/1000;
					}
                    else
                    {
						tt = pi->st + g_cmm_delay/1000;
					}

					GetTime(pi->st, &start);
                    GetTime(tt, &pl->start);

                    pl->start.d = pi->dur;
                    pl->start.f = pi->ms;
                    //sprintf(pl->title, "%s", ps->title);
                    sprintf(pl->title, "%s", "");
                    //sprintf(pl->program_id, "%s", ps->program_id);
                    sprintf(pl->program_id, "%s", "");
                    sprintf(pl->brd_ymd, "%04d%02d%02d", start.yy, start.mm, start.dd);
                    pl->index = pi->index;

                    //_l("[SCHE] Title : %s, (%d, %d, %d)\n", ps->title, ps->pre_count, ps->mid_count, ps->post_count);
                    _l("[SCHE] Title : %s, (pre_count : %d, post_count : %d)\n", ps->title, ps->pre_count, ps->post_count);
                    _l("[SCHE] Start : %02d:%02d:%02d.%04d\n", pl->start.h, pl->start.m, pl->start.s, pl->start.f);
                    _l("[SCHE] Duration = %d (%d => %d)\n", pi->dur, pl->start.d, pl->start.d + 25 + ADDITIONAL);

                    tot_dur = 0;
                    target_dur = pl->start.d + 25 + ADDITIONAL; // 2015-06-20, CIEL , add + 30 for MBC+

                    if (pi->type == 0)
                    { // before
                        for (int a=0; a<ps->pre_count; a++)
                        {
                            DInfo *pd = &ps->pre_file[a];
                            CAdFile *pa = GetAdFile(pd->filename);
                            if (pa)
                            {
                                int dur = pa->GetTotalFrames() + 15; // 2014-12-04, add additional_frame, CIEL
                                if ((dur + tot_dur) < target_dur)
                                {
                                    pd->prepost = 'f';
                                    memcpy(&pl->di[files], pd, sizeof(DInfo));
                                    pl->di[files].is_common = false;
                                    pl->pa[files] = pa;
                                    tot_dur += dur;
                                    files++;
                                }
                            }
                        }
                        pl->is_after = 0;
                        _l("[SCHE] CM -> Before\n");
                        _l("[SCHE] tot_dur = %d target_dur = %d\n", tot_dur, target_dur);
                    }
                    else if (pi->type == 1)
                    { // after
                        for (int a=0; a<ps->post_count; a++)
                        {
                            DInfo *pd = &ps->post_file[a];
                            CAdFile *pa = GetAdFile(pd->filename);
                            if (pa)
                            {
                                int dur = pa->GetTotalFrames() + 15; // 2014-12-04, add additional_frame, CIEL
                                if ((dur + tot_dur) < target_dur)
                                {
                                    pd->prepost = 'b';
                                    memcpy(&pl->di[files], pd, sizeof(DInfo));
                                    pl->di[files].is_common = false;
                                    pl->pa[files] = pa;
                                    pl->pa[files+1] = NULL;                        
                                    tot_dur += dur;
                                    files++;
                                }
                            }
                        }
                        pl->is_after = 1;
                        _l("[SCHE] CM -> After\n");
                        _l("[SCHE] tot_dur = %d target_dur = %d\n", tot_dur, target_dur);
                    }
#if 0
                    else if (pi->type == 2)
                    {
						for (int a=0; a<ps->mid_count; a++)
                        {
                            DInfo *pd = &ps->mid_file[a];
                            CAdFile *pa = GetAdFile(pd->filename);
                            if (pa)
                            {
                                int dur = pa->GetTotalFrames() + 15; // 2014-12-04, add additional_frame, CIEL
                                if ((dur + tot_dur) < target_dur)
                                {
                                    pd->prepost = 'm';
                                    memcpy(&pl->di[files], pd, sizeof(DInfo));
                                    _l("[SCHE] pl->di[%d].prepost : %c\n", files, pl->di[files].prepost);
                                    pl->di[files].is_common = false;
                                    pl->pa[files] = pa;
                                    pl->pa[files+1] = NULL;                        
                                    tot_dur += dur;
                                    files++;
                                }
                            }
                        }
                        pl->is_after = 1;
                        _l("[SCHE] CM -> Middle\n");
                        _l("[SCHE] tot_dur = %d target_dur = %d\n", tot_dur, target_dur);
					}
#endif

					if ((target_dur - tot_dur) >= 14*30)
                    {
						int added = 0;
                        while(tot_dur < target_dur)
                        {
                            CAdFile *pa = cal.pa[first];
                            if (pa)
                            {
                                int dur = pa->GetTotalFrames() + 15; // 2014-12-04, same as above
                                _l("[SCHE] dur %d, tot %d, target %d\n", dur, tot_dur, target_dur);
                                if ((dur + tot_dur) < target_dur)
                                {
                                    cal.di[first].prepost = 'b';
                                    memcpy(&pl->di[files], &cal.di[first], sizeof(DInfo));
                                    //_l("[SCHE] scheduled ADlist : %s, &pl->di[%d] : %p, &pl->di[%d] : %p\n", pl->di[files].filename, files, &pl->di[files], files+1, &pl->di[files+1]);
                                    _l("[SCHE] scheduled ADlist : %s\n", pl->di[files].filename);
                                    pl->di[files].is_common = true;
                                    pl->pa[files] = pa;
                                    pl->pa[files+1] = NULL;                        
                                    tot_dur += dur;
                                    files++;
                                    first++;
									added++;
                                    if (first >= cal.count)
                                    {
                                        first = 0;
                                    }
                                }
                                else
                                {
                                    break;
                                }
                            }
                            else
                            {
                                _l("[SCHE] not have proper AD file (in common.xml)\n");
                                break;
                            }
                        }
						_l("[SCHE] CMA or CMB, but insufficient ADFILE, so insert common CM (%d)\n", added);
                    }

					if (pi->type == 0)
                    {
						pl->dum_start = 0;
                        pl->dum_end = (target_dur - tot_dur);
					}
                    else
                    {
						pl->dum_start = tot_dur;
						pl->dum_end = target_dur;
					}

                    int s = pl->start.h*3600 + pl->start.m*60 + pl->start.s + pl->start.d/30;
                    int h = s/3600;
                    int m = (s%3600)/60;
                    s = ((s%3600)%60);
                    _l("[SCHE] %02d:%02d:%02d ~ %02d:%02d:%02d (%d sec)\n", pl->start.h, pl->start.m, pl->start.s, h, m, s, pl->start.d/30);
                    pl->is_used = 1;
					pthread_mutex_unlock(&pg->csSche);
                }
            }
#endif
        }
	}
	_l("[SCHE] stop thread...\n");
}

PL *CMySchedule::GetPlayList() {
	GD *pg = (GD *)m_pGD;
	PL *pPL = NULL;

	pthread_mutex_lock(&pg->csSche);
	if (m_pl.is_used == 1) {
		pPL = &m_pl;
	}
	pthread_mutex_unlock(&pg->csSche);

	return pPL;
}

void CMySchedule::CompletePlayList()
{
	GD *pg = (GD *)m_pGD;

	pthread_mutex_lock(&pg->csSche);
	m_pl.is_used = 0;
	pthread_mutex_unlock(&pg->csSche);
}


void CMySchedule::Put(SCHE *pc)
{
	int nEmpty = -1;
    SCHE *pd = NULL;

    for (int i=0; i<MAX_NUM_SCHEDULES; i++)
    {
        if (m_pSche[i])
        {
			SCHE *ps = m_pSche[i];

            if ((strcmp(ps->title, pc->title) == 0))
            {
                pthread_mutex_lock(&m_mutex);
				memcpy(ps, pc, sizeof(SCHE));
                pthread_mutex_unlock(&m_mutex);
                return;
            }
        } else {
            if (nEmpty < 0) {
                nEmpty = i;
            }
        }
    }

    pd = new SCHE;
    memcpy(pd, pc, sizeof(SCHE));

    pthread_mutex_lock(&m_mutex);
    if (nEmpty < 0) {
        delete m_pSche[0];
        for (int i=0; i<MAX_NUM_SCHEDULES-1; i++) {
            m_pSche[i] = m_pSche[i+1];
        }
        m_pSche[MAX_NUM_SCHEDULES-1] = pd;
    } else {
        m_pSche[nEmpty] = pd;
    }

    pthread_mutex_unlock(&m_mutex);
    m_bIsUpdate = true;
}

void CMySchedule::Print()
{
    if (m_bIsUpdate) {
        for (int i=0; i<MAX_NUM_SCHEDULES; i++) {
            SCHE *pd = m_pSche[i];
            if (pd) {
                //_ls("[%d] Title = %s, clip = %s, run = %d, epi = %d (%d, %d, %d)\n", pd->title, pd->clip, pd->run, pd->epi, pd->pre_count, pd->mid_count, pd->post_count);
            }
        }
        m_bIsUpdate = false;
    }
}

void CMySchedule::LoadFile(char *filename)
{
    TiXmlDocument doc(filename);
    if (doc.LoadFile()) {
        TiXmlNode *pNodeSche = doc.FirstChild("data")->FirstChild("scheduleinfo");
        while(pNodeSche) {
            char *pt, buf[8];
            int nPre = 0, nPost = 0;

            SCHE temp_sche;
            SCHE *ps = &temp_sche;//(SCHE*)&m_sche[m_nSchedules];

            TiXmlNode *pNodePre, *pNodePost;
            TiXmlElement *pes = pNodeSche->ToElement();

            if (strcmp("A", pes->Attribute("License_Info")) == 0) {
                ps->lic_info = 'A';
            } else {
                ps->lic_info = 'N';
            }
            sprintf(ps->title, "%s", pes->Attribute("Title"));
            sprintf(ps->program_id, "%s", pes->Attribute("ProgramID"));
            sprintf(ps->brd_ymd, "%s", pes->Attribute("Brd_ymd"));

            pt = (char*)pes->Attribute("StartTime");
            memcpy(buf, &pt[0], 2);
            buf[2] = 0;
            ps->start.h = atoi(buf);
            memcpy(buf, &pt[2], 2);
            buf[2] = 0;
            ps->start.m = atoi(buf);
            ps->start.s = 0;
            ps->start.d = 0;
            pt = (char*)pes->Attribute("EndTime");
            memcpy(buf, &pt[0], 2);
            buf[2] = 0;
            ps->end.h = atoi(buf);
            memcpy(buf, &pt[2], 2);
            buf[2] = 0;
            ps->end.m = atoi(buf);
            ps->end.s = 0;
            ps->end.d = 0;

            pt = (char*)pes->Attribute("Real_ymd");
            memcpy(buf, &pt[0], 4);
            buf[4] = 0;
            ps->start.yy = atoi(buf);
            memcpy(buf, &pt[4], 2);
            buf[2] = 0;
            ps->start.mm = atoi(buf);
            memcpy(buf, &pt[6], 2);
            buf[2] = 0;
            ps->start.dd = atoi(buf);

            pNodePre = pNodeSche->FirstChild("pre_ad")->FirstChild("adinfo");
            while(pNodePre) {
                DInfo *pdi = &ps->pre_file[nPre];
                TiXmlElement *pe = pNodePre->ToElement();
                sprintf(pdi->filename, "%s", pe->Attribute("file_name"));

                pt = (char*)pe->Attribute("ste_id");
                pdi->ste_id = atoi(pt);

                pt = (char*)pe->Attribute("sec_id");
                pdi->sec_id = atoi(pt);

                pt = (char*)pe->Attribute("pg_id");
                pdi->pg_id = atoi(pt);

                pt = (char*)pe->Attribute("loc_id");
                pdi->loc_id = atoi(pt);

                pt = (char*)pe->Attribute("cmp_id");
                pdi->cmp_id = atoi(pt);

                pt = (char*)pe->Attribute("ad_id");
                pdi->ad_id = atoi(pt);

                pt = (char*)pe->Attribute("com_id");
                pdi->com_id = atoi(pt);

                pt = (char*)pe->Attribute("ctv_id");
                pdi->ctv_id = atoi(pt);

                pt = (char*)pe->Attribute("priority");
                pdi->priority = atoi(pt);

                //pdi->is_enable = true;

                nPre++;
                pNodePre = pNodePre->NextSibling();
            }

            pNodePost = pNodeSche->FirstChild("post_ad")->FirstChild("adinfo");
            while(pNodePost) {
                DInfo *pdi = &ps->post_file[nPost];

                TiXmlElement *pe = pNodePost->ToElement();
                sprintf(pdi->filename, "%s", pe->Attribute("file_name"));

                pt = (char*)pe->Attribute("ste_id");
                pdi->ste_id = atoi(pt);

                pt = (char*)pe->Attribute("sec_id");
                pdi->sec_id = atoi(pt);

                pt = (char*)pe->Attribute("pg_id");
                pdi->pg_id = atoi(pt);

                pt = (char*)pe->Attribute("loc_id");
                pdi->loc_id = atoi(pt);

                pt = (char*)pe->Attribute("cmp_id");
                pdi->cmp_id = atoi(pt);

                pt = (char*)pe->Attribute("ad_id");
                pdi->ad_id = atoi(pt);

                pt = (char*)pe->Attribute("com_id");
                pdi->com_id = atoi(pt);

                pt = (char*)pe->Attribute("ctv_id");
                pdi->ctv_id = atoi(pt);

                pt = (char*)pe->Attribute("priority");
                pdi->priority = atoi(pt);

                //pdi->is_enable = true;

                nPost++;

                pNodePost = pNodePost->NextSibling();
            }

            ps->pre_count = nPre;
            ps->post_count = nPost;
            if (1) {
                _d("[SCHE] Title : %s, Lic : %c, Start : %04d-%02d-%02d %02d:%02d\n", ps->title, ps->lic_info, ps->start.yy, ps->start.mm, ps->start.dd, ps->start.h, ps->start.m);
                _d("[SCHE] PreAD : %d, PostAD : %d\n", nPre, nPost);
                for (int i=0; i<nPre; i++) {
                    _d("[SCHE] Pre > %s\n", ps->pre_file[i]);
                }
                for (int i=0; i<nPost; i++) {
                    _d("[SCHE] Post > %s\n", ps->post_file[i]);
                }
            }
            Put(ps);
            pNodeSche = pNodeSche->NextSibling();
        }
    }
}

void CMySchedule::UpdateCommonList(CAL *pcal)
{
	TiXmlDocument doc("common.xml");
	if (doc.LoadFile()) {
		TiXmlNode *pRoot = doc.RootElement();
		if (pRoot) {
			int fcount = 0;

			TiXmlNode *pns;
			pns = pRoot->FirstChild();
			while(pns) {
				char *pt, filename[256];
				CAdFile *paf = NULL;

				TiXmlElement *pes = pns->ToElement();
                DInfo *pdi = &pcal->di[fcount];

				sprintf(filename, "%s", pes->Attribute("file_name"));
                paf = GetAdFile(filename);
                if (paf) {
                    pcal->pa[fcount++] = paf;
                }

                sprintf(pdi->filename, "%s", filename);
                pt = (char*)pes->Attribute("ste_id");
                pdi->ste_id = atoi(pt);

                pt = (char*)pes->Attribute("sec_id");
                pdi->sec_id = atoi(pt);

                pt = (char*)pes->Attribute("pg_id");
                pdi->pg_id = atoi(pt);

                pt = (char*)pes->Attribute("loc_id");
                pdi->loc_id = atoi(pt);

                pt = (char*)pes->Attribute("cmp_id");
                pdi->cmp_id = atoi(pt);

                pt = (char*)pes->Attribute("ad_id");
                pdi->ad_id = atoi(pt);

                pt = (char*)pes->Attribute("com_id");
                pdi->com_id = atoi(pt);

                pt = (char*)pes->Attribute("ctv_id");
                pdi->ctv_id = atoi(pt);

                pt = (char*)pes->Attribute("priority");
                pdi->priority = atoi(pt);

				pns = pns->NextSibling();
			}
			pcal->count = fcount;
			pcal->pos = 0;
		}
	}

	//_d("[COMMON] total %d files\n", pcal->count);
}

