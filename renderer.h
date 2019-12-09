#ifndef _RENDERER_H_
#define _RENDERER_H_

class CRenderAudio : public PThread
{
public:
	CRenderAudio();
	~CRenderAudio();

	void Create(void *pGD) {
		m_pGD = pGD;
		Start();
	};

protected:

	void Run();
	void OnTerminate() {};

protected:

	void *m_pGD;
};

class CRenderVideo : public PThread
{
public:
	CRenderVideo();
	~CRenderVideo();

	void Create(void *pGD) {
		m_pGD = pGD;
		Start();
	};

protected:

	void Run();
	void OnTerminate() {};

protected:

	void *m_pGD;
};

#endif // _RENDERER_H_
