#ifndef _DECUNIT_H_
#define _DECUNIT_H_

#define MAX_NUM_FRAMES		4
#define MAX_PCM_SAMPLES		2*48000 // 1sec

typedef struct tagJpg
{
	unsigned char *p;
	int len;
}JPG;

class CDecUnit : public PThread
{
public:
	CDecUnit();
	~CDecUnit();

	bool Open(char *filename, bool bIsIndex = false);

	bool IsInTime(TIME *pt);

	char *GetVideoFrame(int &nFrame, bool &bIsEOF);
	void RetVideoFrame();

	bool GetAudioSamples(short *pSamples, int nSamples);

	long long GetTotalFrames() { return m_i64TotalFrames; };

	JPG *GetJpeg(int nCount) { return &m_jpg[nCount]; };

    char *GetFileName() { return m_strFileName; };
protected:

	bool m_bExitDec;
	bool m_bEOF;
	bool m_bIsReady;
	bool m_bIsIndex;
	bool m_bIsStarted;

	TIME m_time;
	char m_strFileName[256];
	
	long long m_i64Duration;
	long long m_i64TotalFrames;

	JPG m_jpg[3];

	char *m_pFrames[MAX_NUM_FRAMES];
	int m_nFrames[MAX_NUM_FRAMES];

	short *m_pSampleBuff;

	int m_nReadPos;
	int m_nWritePos;

	int m_nSamplesInBuff;
	bool m_bIsEmptyFrame[MAX_NUM_FRAMES];

	pthread_mutex_t m_cs;
protected:

	void Run();
	void OnTerminate();

	int OpenCodec(AVFormatContext *pFmtCtx, AVMediaType nType, int nIndex);

};



#endif // _DECUNIT_H_

