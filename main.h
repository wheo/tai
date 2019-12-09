#ifndef _MAIN_H_
#define _MAIN_H_

typedef struct
{
	CApcProc *pAP;
	CMySchedule *pMS;

	CCmbMgr *pMgr;
	CCommSvr *pSvr;

	pthread_mutex_t csRender;
	pthread_mutex_t csSche;
}GD;

#endif // _MAIN_H_

