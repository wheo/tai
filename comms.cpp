#include "global.h"
#include "cmbmgr.h"
#include "comms.h"

CCommSvr::CCommSvr()
{
    m_sdListen = -1;
    pthread_mutex_init(&m_cs, NULL);

	for (int i=0; i<MAX_NUM_SIM_CLIENT; i++) {
		m_pCT[i] = NULL;
	}
}

CCommSvr::~CCommSvr()
{
    if (m_sdListen >= 0) {
        shutdown(m_sdListen, 2);
        close(m_sdListen);
    }
    Terminate();
	for (int i=0; i<MAX_NUM_SIM_CLIENT; i++) {
		if (m_pCT[i]) {
			delete m_pCT[i];
		}
	}
    pthread_mutex_destroy(&m_cs);
}

void CCommSvr::Run()
{
    int t=1, sd;
    char buff[4];
    struct sockaddr_in sin;

    m_sdListen = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_sdListen < 0) {
        _d("[CMGR] Failed to open socket\n");
    }

    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(m_nPort);

    setsockopt(m_sdListen, SOL_SOCKET, SO_REUSEADDR, (const char*)&t, sizeof(t));

    if (bind(m_sdListen, (const sockaddr *)&sin, sizeof(sin)) < 0) {
        _d("[CMGR] Failed to bind to port %d\n", htons(46801));
    }

    if (listen(m_sdListen, 10) < 0) {
        _d("[CMGR] Failed to listen\n");
    }

    _d("[CMGR] start loop\n");
    while(!m_bExit) {
        if (m_sdListen < 0) {
            _d("[CMGR] Invalid listen socket\n");
            break;
        }

        //_d("[CMGR] waiting for connect\n");
        t = sizeof(sin);
        sd = accept(m_sdListen, (struct sockaddr *)&sin, (socklen_t *)&t);
        if (sd < 0) {
            _d("[CMGR] canceled...\n");
            continue;
        }
        _d("[CMGR] connected from %s\n", inet_ntoa(sin.sin_addr));

		for (int i=0; i<MAX_NUM_SIM_CLIENT; i++) {
			if (m_pCT[i]) {
				if (m_pCT[i]->IsAlive() == false) {
					pthread_mutex_lock(&m_cs);
					delete m_pCT[i];
					m_pCT[i] = NULL;
					pthread_mutex_unlock(&m_cs);
				}
			}
		}
		for (int i=0; i<MAX_NUM_SIM_CLIENT; i++) {
			if (m_pCT[i] == NULL) {
				CCommCt *pCT = new CCommCt();
				pCT->Set(sd, i, inet_ntoa(sin.sin_addr));

				pthread_mutex_lock(&m_cs);
				m_pCT[i] = pCT;
				pthread_mutex_unlock(&m_cs);
				break;
			}
		}

        memset(buff, 0, 4);
    }
    _d("[CMGR] exit loop\n");
}

void CCommSvr::SendSche(int nIndex, CMB *pCMB)
{
	for (int i=0; i<MAX_NUM_SIM_CLIENT; i++) {
		pthread_mutex_lock(&m_cs);
		if (m_pCT[i]) {
			if (m_pCT[i]->IsAlive() == false) {
				delete m_pCT[i];
				m_pCT[i] = NULL;
			} else {
				m_pCT[i]->SendSche(nIndex, pCMB);
			}
		}
		pthread_mutex_unlock(&m_cs);
	}
}

void CCommSvr::SendCPInfo(CPInfo *pCI) 
{
	if (pCI) {
		for (int i=0; i<MAX_NUM_SIM_CLIENT; i++) {
			pthread_mutex_lock(&m_cs);
			if (m_pCT[i]) {
				if (m_pCT[i]->IsAlive() == false) {
					delete m_pCT[i];
					m_pCT[i] = NULL;
				} else {
					m_pCT[i]->SendCPInfo(pCI);
				}
			}
			pthread_mutex_unlock(&m_cs);
		}
	}
}

CCommCt::CCommCt()
{
    m_bIsAlive = false;
}

CCommCt::~CCommCt()
{
	if (m_sd >= 0) {
        shutdown(m_sd, 2);
        close(m_sd);

        m_sd = -1;
    }

	Terminate();
	m_bIsAlive = false;
}

void CCommCt::Set(int sd, int nCt, char *pIpAddr)
{
    m_sd = sd;
    m_nCt = nCt;

    memcpy(m_strIpAddr, pIpAddr, 32);
    _d("[CCMT] set client IP address = %s\n", m_strIpAddr);
    m_bIsAlive = true;

    Start();
}

bool CCommCt::IsAlive()
{
    return m_bIsAlive;
}

void CCommCt::Run()
{
    char *pBuff = new char [1920*1080];

    _d("[CCT] start loop\n");
    while(!m_bExit) 
    {
        if (SR(pBuff, 4) < 0) {
            m_bIsAlive = false;
            break;
        }
        if (pBuff[0] == 'T' && pBuff[1] == 'C') {
            if (pBuff[2] == 'S') {
                if (pBuff[3] == 'L') {
					_l("[CCT] Licensed (off image)\n");
					//g_pLicMgr->ForceStop();
                }
				if (pBuff[3] == 'U') {
					_l("[CCT] Unlicensed (on image)\n");
					//g_pLicMgr->ForceStart();
				}
            } 
        }
        memset(pBuff, 0, 256);
    }
    _d("[CCT] exit loop\n");
    m_bIsAlive = false;
    delete [] pBuff;
}

void CCommCt::SendSche(int nIndex, CMB *pCMB)
{
	int nLen = sizeof(CMB);
	if (SS((char*)&nLen, 4) < 0) {
		m_bIsAlive = false;
		return;
	}
	if (SS((char*)pCMB, sizeof(CMB)) < 0) {
		m_bIsAlive = false;
		return;
	}
	if (SS((char*)&nIndex, 4) < 0) {
		m_bIsAlive = false;
		return;
	}
}
void CCommCt::SendCPInfo(CPInfo *pCI)
{
	int nLen = sizeof(CPInfo);
	if (SS((char*)&nLen, 4) < 0) {
		m_bIsAlive = false;
		return;
	}
	if (SS((char*)pCI, sizeof(CPInfo)) < 0) {
		m_bIsAlive = false;
		return;
	}
}

CCommBase::CCommBase()
{
    m_sd = -1;
}

CCommBase::~CCommBase()
{
    if (m_sd >= 0) {
        shutdown(m_sd, 2);
        close(m_sd);

        m_sd = -1;
    }
}

int CCommBase::SR(char *pBuff, int nLen)
{
	int w = 0, local = nLen;

	while(local) {
		int r = recv(m_sd, &pBuff[w], local, 0);
		if (r > 0) {
			w += r;
			local -= r;
		} else {
			return -1;
		}
	}

	return w;
}

int CCommBase::SS(char *pBuff, int nLen)
{
	int w = 0, local = nLen;

	while(local) {
		int s = send(m_sd, &pBuff[w], local, 0);
		if (s > 0) {
			w += s;
			local -= s;
		} else {
			return -1;
		}
	}

	return w;
}

