#ifndef _DECKLINK_INPUT_H_
#define _DECKLINK_INPUT_H_

#include "DeckLinkAPI.h"

class CDeckLinkInput : public IDeckLinkInputCallback
{
public:
	CDeckLinkInput();
	~CDeckLinkInput();

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv) { return E_NOINTERFACE; }
	virtual ULONG STDMETHODCALLTYPE AddRef(void);
	virtual ULONG STDMETHODCALLTYPE  Release(void);
	virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(BMDVideoInputFormatChangedEvents, IDeckLinkDisplayMode*, BMDDetectedVideoInputFormatFlags);
	virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame*, IDeckLinkAudioInputPacket*);

protected:
	
private:

	ULONG			m_refCount;
	pthread_mutex_t m_mutex;
};

#endif //_DECKLINK_INPUT_H_
