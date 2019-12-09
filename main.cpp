#include "global.h"
#include "scheduling.h"
#include "renderer.h"
#include "apcproc.h"
#include "cmbmgr.h"
#include "comms.h"
#include "decklink.h"
#include "main.h"

pthread_mutex_t	sleepMutex;
pthread_cond_t	sleepCond;

bool exit_flag_main = false;
bool ignore_flag_feed = false;

CQueue *g_pqv = NULL;
CQueue *g_pqa = NULL;
CLicMgr *g_pLicMgr = NULL;
//CTimeSync g_ts;

int g_out_delay = 1000;
int g_cmb_delay = 1000;
int g_cmm_delay = 1000;
int g_cma_delay = 1000;
int g_common_delay = 1000;

char g_channel[32];

void sigfunc(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) {
		exit_flag_main = true;
	}
	pthread_cond_signal(&sleepCond);
}

void LoadConfig();

int main(int argc, char *argv[])
{
	GD gd;

	CDeckLink *pDL = NULL;

	CRenderAudio *pRA = NULL;
	CRenderVideo *pRV = NULL;

	memset(&gd, 0, sizeof(GD));

	LoadConfig();

    //system("clear");
	_l("TNM AD Insert System KBS2 ver. (build at %s,%s)\n", __TIME__, __DATE__);
	_l("Channel : %s\n", g_channel);

	av_register_all();

	pthread_mutex_init(&gd.csSche, NULL);
	pthread_mutex_init(&gd.csRender, NULL);

	pthread_mutex_init(&sleepMutex, NULL);
	pthread_cond_init(&sleepCond, NULL);

	signal(SIGINT, sigfunc);
	signal(SIGTERM, sigfunc);
	signal(SIGHUP, sigfunc);

	//g_ts.Create(5432);

	g_pLicMgr = new CLicMgr();

	gd.pSvr = new CCommSvr;
	gd.pSvr->Open(48900);

	gd.pMgr = new CCmbMgr();
    gd.pMS = new CMySchedule();
	gd.pAP = new CApcProc();

	gd.pMgr->Create(&gd);
    gd.pMS->Create(&gd);
	gd.pAP->Create(&gd);

	ignore_flag_feed = false;

	g_pqv = new CQueue();
	g_pqv->Create(140, 2*1920*1080);
	
	g_pqa = new CQueue();
	g_pqa->Create(140, 8192);

	pDL = new CDeckLink();
	pDL->Init();

	pRA = new CRenderAudio();
	pRA->Create(&gd);

	pRV = new CRenderVideo();
	pRV->Create(&gd);

	_l("TNM AD Insert System started...\n");
	while(!exit_flag_main) {
		pthread_mutex_lock(&sleepMutex);
		pthread_cond_wait(&sleepCond, &sleepMutex);
		pthread_mutex_unlock(&sleepMutex);
	}
    _d("[MAIN] Get Signal\n");

	delete pRV;
	delete pRA;

	ignore_flag_feed = true;
	delete pDL;

	delete gd.pAP;
    delete gd.pMS;
	delete gd.pMgr;
	delete gd.pSvr;
	delete g_pLicMgr;

	printf("\nTNM AD Insert System closed...\n");
	printf("\n");

	delete g_pqa;
	delete g_pqv;

	g_pqa = NULL;
	g_pqv = NULL;

	pthread_mutex_destroy(&gd.csRender);
	pthread_mutex_destroy(&gd.csSche);
	_l("Exit\n");

	return 0;
}

void ReadConfigLine(FILE *fp, char *pCfg)
{
    int nCnt;
    bool bIsStart = false;

    while(1) {
        char c = fgetc(fp);
        if (c == '[') {
            nCnt = 0;
            bIsStart = true;
        } else if (c == ']') {
            pCfg[nCnt] = 0;
            fseek(fp, 1, SEEK_CUR);
            break;
        } else if (bIsStart) {
            pCfg[nCnt++] = c;
        }   
    }   
}

void LoadConfig()
{
    FILE *fp = fopen("config.ini", "r");
    if (fp) {
        char temp[256];

        ReadConfigLine(fp, temp);      	g_out_delay = atoi(temp); 
        ReadConfigLine(fp, temp);       g_cmb_delay = atoi(temp);
        ReadConfigLine(fp, temp);       g_cmm_delay = atoi(temp);
        ReadConfigLine(fp, temp);       g_cma_delay = atoi(temp);
        ReadConfigLine(fp, temp);       g_common_delay = atoi(temp);
        ReadConfigLine(fp, temp);       sprintf(g_channel, "%s", temp);
		// Main FTP
        ReadConfigLine(fp, temp);       sprintf(g_ftp_ip[0], "%s", temp);
        ReadConfigLine(fp, temp);       sprintf(g_ftp_id[0], "%s", temp);
        ReadConfigLine(fp, temp);       sprintf(g_ftp_pw[0], "%s", temp);
		// Backup FTP
        ReadConfigLine(fp, temp);       sprintf(g_ftp_ip[1], "%s", temp);
        ReadConfigLine(fp, temp);       sprintf(g_ftp_id[1], "%s", temp);
        ReadConfigLine(fp, temp);       sprintf(g_ftp_pw[1], "%s", temp);

        fclose(fp);
    }   
}

