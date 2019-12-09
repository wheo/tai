#ifndef _CMBMGR_H_
#define _CMBMGR_H_

#define MAX_NUM_CMB		128

typedef struct tagCMI
{
    int type; // 0-b, 1-a, 2-m, 3-sbcm
    int index; // cm index
    int status; // 1-not used, 2-using, 3-complete
    int dur;
    int ms; // millisecond start time

	time_t st;
	time_t ed;
}CMI;

typedef struct tagCMB
{
    int type; // 0-normal, 1-bypass
    int index; // cm index
    int num; // cm num

	time_t st;
	time_t ed;

    char title[256];

	//char clipid[16];

    CMI cm[MAX_NUM_CM];
}CMB;

class CCmbMgr : public PThread
{
public:
    CCmbMgr();
    ~CCmbMgr();

    void Clear();

    void Put(CMB *pc);
    bool Get(int nIndex, CMB *pc);

	bool IsNextRun(CMB *pp, CMB *pc);
	bool IsNextEpi(CMB *pc);

    void Set(int nIndex, int nCmb, int nStatus);
	void Create(void *pGD) {
		m_pGD = pGD;
		Start();
	};

protected:

    pthread_mutex_t m_mutex;

    CMB *m_pCMB[MAX_NUM_CMB];
	void *m_pGD;

    bool m_bIsUpdate;

protected:

    void Run();
    void OnTerminate() {};
};

class CLicMgr
{
public:
	CLicMgr();
	~CLicMgr();

	void Put(CMB *pc);
    bool Get(int nIndex, CMB *pc);

	void Print();

    CMB *IsStart(time_t ct);
    bool IsStop(time_t ct);

	//void ForceStart();
	//void ForceStop();

protected:

    pthread_mutex_t m_mutex;

    CMB *m_pCMB[MAX_NUM_CMB];
    CMB *m_pCur;

    bool m_bIsUpdate;

	int m_nForceCount;
};

extern CLicMgr *g_pLicMgr;
//extern void GetRunCode(char *pClipId, CMB *pc);

#endif // _CMBMGR_H_

