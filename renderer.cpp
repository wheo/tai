#include "global.h"
#include "renderer.h"
#include "decunit.h"
#include "scheduling.h"
#include "tinyxml.h"
#include "apcproc.h"
#include "cmbmgr.h"
#include "comms.h"
#include "main.h"

#include <sys/ipc.h>
#include <sys/msg.h>

CMyList fl;
CDecUnit *puc = NULL;

bool is_mute = false;
bool is_disp_ad = false;

int dummy_state = 0;
int license_state = 0;

int run_more_frames = 25; //5; // 2014-12-05, add more additional frame at end of CM band

CRenderAudio::CRenderAudio()
{
}
CRenderAudio::~CRenderAudio()
{
	Terminate();
}

void CRenderAudio::Run()
{
	GD *pg = (GD *)m_pGD;

    short *pDummy = new short [10*1024*1024];
    int nDummy = 0, nPos = 0;

	short *pLic = new short [6*1024*1024];
	int nLic = 0, nLicPos = 0;

    FILE *fp = fopen("adinfo.pcm", "rb");
    if (fp) {
        _l("[AUDIO] ADINFO PCM file opened\n");
        fseek(fp, 0l, SEEK_END);

        nDummy = ftell(fp);
        _l("[AUDIO] ADINFO PCM size = %d\n", nDummy);

        fseek(fp, 0l, SEEK_SET);
        if (pDummy == NULL) {
            _l("[AUDIO] Out of memory\n");
        }

        fread(pDummy, 1, nDummy, fp);
        fclose(fp);
        _l("[AUDIO] ADINFO PCM read complete\n");
    }

	fp = fopen("copyright.pcm", "rb");
	if (fp) {
        _l("[AUDIO] Copyright PCM file opened\n");
        fseek(fp, 0l, SEEK_END);

        nLic = ftell(fp);
        _l("[AUDIO] Copyright PCM size = %d\n", nLic);

        fseek(fp, 0l, SEEK_SET);
        if (pLic == NULL) {
            _l("[AUDIO] Out of memory\n");
        }

        fread(pLic, 1, nLic, fp);
        fclose(fp);
        _l("[AUDIO] Copyright PCM read complete\n");
    }

	_l("[AUDIO] start render loop\n");
	while(!m_bExit) {
		ELEM *pe = (ELEM *)g_pqa->Get();
		if (pe) {
			uint32_t written;

			pthread_mutex_lock(&pg->csRender);
			if (puc && is_disp_ad) {
				if (is_mute == false) {
					if (!puc->GetAudioSamples((short*)pe->p, pe->len/4)) { // 2014-12-04, force to mute at EOF, CIEL
						memset(pe->p, 0, pe->len);
					}
				}
			}
			pthread_mutex_unlock(&pg->csRender);
			if (is_mute) {
				memset(pe->p, 0, pe->len);
			}

            if (dummy_state & 0x1) {
                dummy_state &= ~0x1;
                dummy_state |= 0x2;
                nPos = 0;
            }
            if (dummy_state & 0x4) {
                dummy_state = 0;
            }
            if (dummy_state & 0x2) {
                int nRem = nDummy - nPos;
                char *pPCM = (char*)pDummy;
                if (nRem < pe->len) {
                    char *pd = (char*)pe->p;
                    memcpy(pd, &pPCM[nPos], nRem);
                    memcpy(&pd[nRem], pPCM, (pe->len - nRem));
                    nPos = pe->len - nRem;
                } else {
                    memcpy(pe->p, &pPCM[nPos], pe->len);
                    nPos += pe->len;
                }
            }

            if (license_state & 0x1) {
				_l("[AUDIO] start playing licensed image\n");
                license_state |= 0x2;
                license_state &= ~0x1;
                nLicPos = 0;
            }
            if (license_state & 0x4) {
                license_state &= ~(0x4 | 0x2);
            }
            if (license_state & 0x2) {
                int nRem = nLic - nLicPos;
                char *pPCM = (char*)pLic;
                if (nRem < pe->len) {
                    char *pd = (char*)pe->p;
                    memcpy(pd, &pPCM[nLicPos], nRem);
                    memcpy(&pd[nRem], pPCM, (pe->len - nRem));
                    nLicPos = pe->len - nRem;
                } else {
                    memcpy(pe->p, &pPCM[nLicPos], pe->len);
                    nLicPos += pe->len;
                }
            }

			g_pDLO->WriteAudioSamplesSync(pe->p, pe->len/4, &written);
#if 0
			if (written != (uint32_t)pe->len/4) {
				_l("[AUDIO] %d / %d\n", written, pe->len/4);
			}
#endif
			g_pqa->Ret(pe);
		}
	}

	_l("[AUDIO] stop render loop\n");
}

CRenderVideo::CRenderVideo()
{

}
CRenderVideo::~CRenderVideo()
{
	Terminate();
}

void CRenderVideo::Run()
{
	char *pDummy;
	char *pLic;
	char filename[256];

	GD *pg = (GD *)m_pGD;

	PL *pPL = NULL, plist;

	CAdFile *pac = NULL;
	CAdFile *pan = NULL;

	CDecUnit *pun = NULL;
	CMB *pnl = NULL; // Not licensed period

	CCommSvr *pSvr = (CCommSvr *)pg->pSvr;
	CMySchedule *pMS = (CMySchedule *)pg->pMS;

	CPInfo ci;

	memset(&plist, 0, sizeof(PL));

	int playing = 0;

	int elapsed_ad = 0;
	int elapsed_lic = 0;

	int reload_count = 5;
	int additional_frames = 0; // 2014-12-04, add variables for additinal frames, CIEL

    bool is_ready_pl = false;
    bool is_write_log = false;

	IDeckLinkMutableVideoFrame*	pFrame = NULL;

	pDummy = new char [1920*1080*2];
	if (pDummy) {
		FILE *fp = fopen("dummy.yuv", "rb");
		fread(pDummy, 1, 1920*1080*2, fp);
		fclose(fp);
	}

	pLic = new char [1920*1080*2];
	if (pLic) {
		FILE *fp = fopen("copyright.yuv", "rb");
		fread(pLic, 1, 1920*1080*2, fp);
		fclose(fp);
	}

	memset(&ci, 0, sizeof(CPInfo));

    while (true)
    {
        if ( S_OK == g_pDLO->CreateVideoFrame(1920, 1080, 2*1920, bmdFormat8BitYUV, bmdFrameFlagDefault, &pFrame))
        {
            break;
        }
        else
        {
            _d("[RENDERER] Wait CreateVideoFrame...... \n");
            sleep(1);
        }
    }
	usleep(500);
	_l("[VIDEO] start render loop\n");

	long t0 = GetTickCount();

	while(!m_bExit)
    {
		time_t ct;
        timeval tp;
		struct tm t;

		ELEM *pe = NULL;

		//time(&ct);
        gettimeofday(&tp, 0);
        ct = tp.tv_sec;
		MLT(ct, t);

		pe = (ELEM*)g_pqv->Get();
		if (pe)
        {
			bool bEOF = false;
			char *pBuff, *ps = NULL;
			int nFrames = 0;

			pFrame->GetBytes((void**)&pBuff);

			if (license_state & 0x10)
            {
				license_state |= 0x20;
				license_state &= ~0x10;
			}
			if (license_state & 0x40)
            {
				license_state &= ~(0x60);
			}
			if (is_disp_ad)
            {
				bool bDummy = false;
				if (elapsed_ad >= pPL->dum_start && elapsed_ad < pPL->dum_end)
                {
                    if (dummy_state == 0)
                    {
                        dummy_state |= 0x1;
                    }
					sprintf(ci.filename, "Now advertising...");

					ps = pDummy;
					bDummy = true;
					is_mute = true;
				}
                if (bDummy == false && (dummy_state & 0x2))
                {
                    dummy_state |= 0x4;
                }
				if (bDummy == false && puc)
                {
					ps = puc->GetVideoFrame(nFrames, bEOF);
					is_mute = false;

					bEOF = false; // get more accurate EOF

					if ((nFrames+1) >= pac->GetTotalFrames())
                    {
						if (additional_frames == 0)
                        {
							additional_frames = 15;
						}
						bEOF = true;
					}
				}

				if (ps)
                {
					memcpy(pBuff, ps, pe->len);
					elapsed_ad++;
					ci.elapsed_ad = elapsed_ad;
				}

				if (pac)
                { // preload next decoding unit...
					if ((pac->GetTotalFrames() - nFrames) <= reload_count && pun == NULL)
                    {
						pan = pPL->pa[playing+1];
                        //_l("[VIDEO] pac->GetTotalFrame() : %d, nFrame : %d, A - B : %d, playing : %d pan = %p\n", pac->GetTotalFrames(), nFrames, pac->GetTotalFrames() - nFrames, playing, pan);
						if (pan)
                        {
							sprintf(filename, "%s", pan->GetFileName());
							if (strlen(filename) < 5)
                            {
								_l("[VIDEO] no proper file...so discard this (%d)\n", playing);
								playing++;
							}
                            else
                            {
								sprintf(filename, "/home/adfile_ftp/%s", pan->GetFileName());
								if (access(filename, F_OK) == 0)
                                { // 2014-12-05, one more exception handler, CIEL
									pun = new CDecUnit();
									pun->Open(filename);
									_l("[VIDEO] preload %s (%d)\n", pan->GetFileName(), elapsed_ad);
								}
                                else
                                {
									_l("[VIDED] failed to preload...there is no file (%s)\n", filename);
                                    playing++;
								}
							}
						}
					}
				}

				if (bDummy == false && puc)
                {
					if (!bEOF)
                    {
						puc->RetVideoFrame();
					}
					if (additional_frames)
                    {
						additional_frames--;
					}
				}

				if (bEOF && additional_frames == 0)
                {
                    //_l("[VIDEO] bEOF : %s is_write_log : %s\n", bEOF ? "true" : "false" , is_write_log ? "true" : "false");
                    
                    if (!is_write_log)
                    {
                        FILE *fpLog = NULL;
                        DInfo *pdi = &pPL->di[playing];

                        //_l("[Video] pdi->prepost : %c\n", pdi->prepost);
                        sprintf(filename, "logs/%s_%04d%02d%02d%02d.log", g_channel, t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour);
                        fpLog = fopen(filename, "a");
                        if (fpLog && pdi) {
                            fprintf(fpLog, "%04d-%02d-%02d %02d:%02d:%02d,%s,%s,%02d%02d,%d,%d,%d,%d,%d,%d,%d,%d,%c,%d,\n", 
                                    t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,
                                    pPL->program_id, pPL->brd_ymd, pPL->start.h, pPL->start.m, 
                                    pdi->ste_id, pdi->sec_id, pdi->pg_id, pdi->loc_id, pdi->cmp_id, pdi->ad_id, pdi->com_id, pdi->ctv_id, pdi->prepost, pdi->priority);
                            fclose(fpLog);
                        }
                        is_write_log = true;
                    }

					if (pun)
                    {
						playing++;
						pthread_mutex_lock(&pg->csRender);
						if (puc)
                        {
							delete puc;
							puc = NULL;
						}
						puc = pun;
						pun = NULL;
						pthread_mutex_unlock(&pg->csRender);

						pac = pan;
						pan = NULL;
                        is_write_log = false;
						sprintf(ci.filename, "%s", pac->GetFileName());
					}
                    else
                    {
                        if (elapsed_ad < (pPL->start.d + run_more_frames))
                        {
                            _l("[VIDEO] no more AD files to decode but run more frames(%d)\n", elapsed_ad);
                            is_mute = true;
                        }
					}
				}

				if (elapsed_ad >= (pPL->start.d + run_more_frames))
                {
					_l("[VIDEO] all completed (actual dur = %d)\n", elapsed_ad);
					pthread_mutex_lock(&pg->csRender);
					if (puc) {
						delete puc;
						puc = NULL;
					}
					pthread_mutex_unlock(&pg->csRender);
					pPL = NULL;
					pMS->CompletePlayList();
					is_disp_ad = 0;
					is_mute = false;
                    is_ready_pl = false;
                    dummy_state |= 0x4;
					ci.elapsed_ad = -1;
				}
			}
            else
            {
				memcpy(pBuff, pe->p, pe->len);
			}
			if (license_state & 0x20)
            {
				sprintf(ci.filename, "Unlicensed program...");
				memcpy(pBuff, pLic, pe->len);
				elapsed_lic++;
				ci.elapsed_lic = elapsed_lic;
			}

			g_pqv->Ret(pe);
			g_pDLO->DisplayVideoFrameSync(pFrame);

			if (pnl)
            {
				if ((license_state & 0x60) == 0x20)
                {
                    if (g_pLicMgr->IsStop(ct))
                    {
						license_state |= 0x44;
						_l("[VIDEO] stop playing licensed image\n");
						ci.elapsed_lic = -1;
						pnl = NULL;
					}
				}
			}
            else
            {
                if ((license_state & 0x77) == 0)
                {
                    pnl = g_pLicMgr->IsStart(ct);
                    if (pnl) {
                        license_state |= 0x11;
                        _l("[VIDEO] start playing licensed image\n");
						elapsed_lic = 0;
                    }
                }
            }
		
			if (pPL == NULL)
            {
				PL *pl = pMS->GetPlayList();
				if (pl && pl->is_used)
                {
					long long sta = (pl->start.h*3600 + pl->start.m*60 + pl->start.s)*1000 + pl->start.f;
					long long cur = (t.tm_hour*3600 + t.tm_min*60 + t.tm_sec)*1000 + tp.tv_usec/1000;
					int diff = abs(sta - cur);
					if (diff < 2000)
                    {
						_l("[VIDEO] Select proper schedule > %02d:%02d:%02d.%04d (%02d:%02d:%02d.%04d)\n", t.tm_hour, t.tm_min, t.tm_sec, tp.tv_usec/1000, pl->start.h, pl->start.m, pl->start.s, pl->start.f);

                        memset(&plist, 0, sizeof(PL));
						memcpy(&plist, pl, sizeof(PL));
						pPL = &plist;

						playing = 0;
						elapsed_ad = 0;
						is_disp_ad = 0;
						is_ready_pl = false;
					}
				}
			}

			if (pPL && puc == NULL && is_disp_ad == 0)
            {
				pac = pPL->pa[playing];
				if (pac)
                {
                    puc = new CDecUnit();
					sprintf(filename, "/home/adfile_ftp/%s", pac->GetFileName());
					puc->Open(filename);
					is_ready_pl = true;
                    is_write_log = false;
                    _l("[VIDEO] Buffering %s\n", filename);
				}
                if (pac == NULL && is_ready_pl == false)
                {
                    _l("[VIDEO] Error on playlist (bad filename or not have file in playlist)\n");
					pPL = NULL;
                }
			}

			if (pPL && is_disp_ad == 0)
            {
#if 0
                int sta = pl->start.h*3600 + pl->start.m*60 + pl->start.s;
                int cur = t->tm_hour*3600 + t->tm_min*60 + t->tm_sec;
#else
                long long sta = (pPL->start.h*3600 + pPL->start.m*60 + pPL->start.s)*1000 + pPL->start.f;
                long long cur = (t.tm_hour*3600 + t.tm_min*60 + t.tm_sec)*1000 + tp.tv_usec/1000;
#endif
				if (cur >= sta)
                {
					sprintf(ci.filename, "%s", pac->GetFileName());
                    _l("[VIDEO] Start playing > %02d:%02d:%02d.%04d (%02d:%02d:%02d.%04d)\n", t.tm_hour, t.tm_min, t.tm_sec, tp.tv_usec/1000, pPL->start.h, pPL->start.m, pPL->start.s, pPL->start.f);
                    _l("[VIDEO] dummy %d ~ %d\n", pPL->dum_start, pPL->dum_end);
					is_disp_ad = 1;
					ci.elapsed_ad = 0;
					ci.duration = pPL->start.d + 25;
				}
			}
		}

		long t1 = GetTickCount();
		if ((t1 - t0) >= 500)
        {
			t0 = t1;

			if (ci.elapsed_ad || ci.elapsed_lic)
            {
				pSvr->SendCPInfo(&ci);

				if (ci.elapsed_ad < 0)
                {
					ci.elapsed_ad = 0;
				}
				if (ci.elapsed_lic < 0)
                {
					ci.elapsed_lic = 0;
				}
			}
		}
	}

	_l("[VIDEO] stop render loop...\n");
}

