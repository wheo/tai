#ifndef _QUEUE_H_
#define _QUEUE_H_

#define MAX_NUM_QUEUE		512
#define QUE_INFINITE		-1

typedef struct 
{
	char *p;
	int len;
}ELEM;

class CQueue
{
public:
	CQueue();
	~CQueue();
		
	void Create(int nNum, int nSize);
	void Clear();

	int Put(char *pData, int nSize);

	void *Get();
	void Ret(void *p);

    void Enable();
    bool IsEnable() { return m_bEnable; };

private:

    bool m_bEnable;

    int m_nDelay;
    int m_nFrames;

	int m_nMaxQueue;
	int m_nSizeQueue;

	int m_nReadPos;
	int m_nWritePos;

	char *m_pBuffer;

	pthread_mutex_t m_mutex;

	ELEM m_e[MAX_NUM_QUEUE];
};

#endif //_QUEUE_H_
