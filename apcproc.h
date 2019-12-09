#ifndef _APCPROC_H_
#define _APCPROC_H_

class CApcUpload : public PThread
{
public:
    CApcUpload() {};
    ~CApcUpload() {};

    void Upload(char *filename);

protected:

    void Run();
    void OnTerminate() {};

protected:

    char m_strFileName[256];
};

class CApcProc : public PThread
{
public:
	CApcProc();
	~CApcProc();

	void Create(void *pGD);

protected:

	void Run();
	void OnTerminate() {};

protected:

	void *m_pGD;
};

class CApcParser : public PThread
{
public:
	CApcParser();
	~CApcParser();

	void Parse(char *filename, void *pMgr);

protected:

	void Run();
	void OnTerminate() {};

protected:

	bool m_bIsComplete;
	char m_strFileName[256];

	void *m_pMgr;

};

class CApcRecv : public PThread
{
public:
	CApcRecv();
	~CApcRecv();

	void Create();

	bool GetNewFile(char *filename) {
		bool bNewFile = false;

		pthread_mutex_lock(&m_m);

		if (m_bIsNewFile) {
			sprintf(filename, "%s", m_strNewFile);

			bNewFile = true;
			m_bIsNewFile = false;
		}

		pthread_mutex_unlock(&m_m);
		return bNewFile;
	};


protected:

	void Run();
	void OnTerminate() {};

	void FindLatestFile();
protected:

	char m_strPath[256];

	char m_strNewFile[256];
	char m_strOldFile[256];

	bool m_bIsNewFile;

	pthread_mutex_t m_m;
};

#endif //_APCPROC_H_
