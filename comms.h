#ifndef _COMMS_H_
#define _COMMS_H_

#define MAX_NUM_SIM_CLIENT      4

typedef struct
{
	char filename[256];

	int elapsed_lic;
	int elapsed_ad;

	int duration;
}CPInfo;

class CCommBase // Class for comm. module base
{
public:
    CCommBase();
    ~CCommBase();

protected:

    int m_sd; // Socket descriptor

protected:

    int SR(char *pBuff, int nLen);
    int SS(char *pBuff, int nLen);
};

class CCommCt : public CCommBase, public PThread // Class for Comm server module for client
{
public:
    CCommCt();
    ~CCommCt();

    bool IsAlive();
	void Set(int sd, int nCt, char *pIpAddr);
	void SendSche(int nIndex, CMB *pCMB);
	void SendCPInfo(CPInfo *pCI);

protected:

    int m_nCt; // # of client
    char m_strIpAddr[32]; // IP address of client

    bool m_bIsAlive;
protected:

    void Run();
    void OnTerminate() {};
};

class CCommSvr : public PThread
{
public:
    CCommSvr();
    ~CCommSvr();

	void Open(int nPort) {
		m_nPort = nPort;
		Start();
	};
	void SendSche(int nIndex, CMB *pCMB);
	void SendCPInfo(CPInfo *pCI);

protected:
    void Run();
    void OnTerminate() {};

protected:

	int m_nPort;
    int m_sdListen;
	pthread_mutex_t m_cs;

	CCommCt *m_pCT[MAX_NUM_SIM_CLIENT];
};

#endif // _COMMS_H_

