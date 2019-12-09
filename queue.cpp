#include "global.h"

CQueue::CQueue() 
{ 
	m_nMaxQueue = 0; 
	m_nSizeQueue = 0; 
	m_pBuffer = NULL; 

    m_bEnable = false;
    m_nFrames = 0;

	pthread_mutex_init(&m_mutex, NULL);
}
	
CQueue::~CQueue() 
{
	if (m_pBuffer) {
		free(m_pBuffer);
		m_pBuffer = NULL;
	}

	pthread_mutex_destroy(&m_mutex);
}
		
void CQueue::Create(int nNum, int nSize) 
{
	m_nMaxQueue = nNum;
	m_nSizeQueue = nSize;
	m_pBuffer = (char *)malloc(nNum*nSize);
	if (m_pBuffer == NULL) {
		_d("[QUEUE] failed to alloc queue buffer\n");
	} else {
		for (int i=0; i<m_nMaxQueue; i++) {
			m_e[i].len = 0;
			m_e[i].p = m_pBuffer + i*nSize;
		}
	}
}
	
void CQueue::Clear() 
{
	for (int i=0; i<m_nMaxQueue; i++) {
		m_e[i].len = 0;
	}

	m_nReadPos = 0;
	m_nWritePos = 0;

    m_bEnable = false;
    m_nFrames = 0;
}

void CQueue::Enable()
{
    m_nDelay = m_nFrames;
    m_bEnable = true;

    _d("[QUEUE] Enable outputing now...%d\n", m_nFrames);
}

int CQueue::Put(char *pData, int nSize)
{
	int nCount = 0;

	while(1) {
		ELEM *pe = &m_e[m_nWritePos];
		pthread_mutex_lock(&m_mutex);
		if (pe->len == 0) {
			memcpy(pe->p, pData, nSize);
			pe->len = nSize;

			m_nWritePos++;
			if (m_nWritePos >= m_nMaxQueue) {
				m_nWritePos = 0;
			}
            m_nFrames++;
			pthread_mutex_unlock(&m_mutex);
			return 0;
		}
		pthread_mutex_unlock(&m_mutex);
		usleep(10);

		nCount++;
		if (nCount >= 100000) {
			_d("TImeout\n");
			break;
		}
	}
}

void *CQueue::Get()
{
    if (m_bEnable == false) {
        return NULL;
    }

	while(1) {
		ELEM *pe = &m_e[m_nReadPos];
		pthread_mutex_lock(&m_mutex);
		if (pe->len) {
            if (m_nFrames < m_nDelay) {
                pthread_mutex_unlock(&m_mutex);
                return NULL;
            }
			pthread_mutex_unlock(&m_mutex);
			return pe;
		}
		pthread_mutex_unlock(&m_mutex);
		usleep(10);
	}
}

void CQueue::Ret(void *p)
{
	ELEM *pe = (ELEM *)p;
	pthread_mutex_lock(&m_mutex);

	pe->len = 0;
	m_nReadPos++;
	if (m_nReadPos >= m_nMaxQueue) {
		m_nReadPos = 0;
	}
    m_nFrames--;
	pthread_mutex_unlock(&m_mutex);
}

