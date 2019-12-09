#include "global.h"
#include "decklink_input.h"

CDeckLinkInput::CDeckLinkInput()
{
	m_refCount = 0;
	pthread_mutex_init(&m_mutex, NULL);
}

CDeckLinkInput::~CDeckLinkInput()
{
	pthread_mutex_destroy(&m_mutex);
}

ULONG CDeckLinkInput::AddRef(void)
{
	pthread_mutex_lock(&m_mutex);
	m_refCount++;
	pthread_mutex_unlock(&m_mutex);

	return (ULONG)m_refCount;
}

ULONG CDeckLinkInput::Release(void)
{
	pthread_mutex_lock(&m_mutex);
	m_refCount--;
	pthread_mutex_unlock(&m_mutex);

	if (m_refCount == 0)
	{
		delete this;
		return 0;
	}

	return (ULONG)m_refCount;
}

long long g_t0 = -1;

HRESULT CDeckLinkInput::VideoInputFrameArrived(IDeckLinkVideoInputFrame* videoFrame, IDeckLinkAudioInputPacket* audioFrame)
{
#if 0
	if (ignore_flag_feed) {
		return S_OK;
	}

	if (videoFrame) {
		if (videoFrame->GetFlags() & bmdFrameHasNoInputSource) {
		} else {
			void *pFrame;
			videoFrame->GetBytes(&pFrame);
			g_pqv->Put((char*)pFrame, 2*1920*1080);
		}
	}

	if (audioFrame) {
		void *pSamples;
		audioFrame->GetBytes(&pSamples);

        g_pqa->Put((char*)pSamples, 2*2*audioFrame->GetSampleFrameCount());
	}

    if (g_t0 < 0) {
        g_t0 = GetTickCount();
    } else if (g_pqv->IsEnable() == false) {
        long long t1 = GetTickCount();
        if ((t1 - g_t0) >= g_out_delay) {
            g_pqv->Enable();
            g_pqa->Enable();
        }
    }

	return S_OK;
#endif

    if (ignore_flag_feed) {
        return S_OK;
    }   

    if (videoFrame) {
        if (videoFrame->GetFlags() & bmdFrameHasNoInputSource) {

        } else {
            void *pFrame;
            videoFrame->GetBytes(&pFrame);
            g_pqv->Put((char*)pFrame, 2*1920*1080);
        }   
    }   

    if (audioFrame) {
        void *pSamples;
        audioFrame->GetBytes(&pSamples);

        g_pqa->Put((char*)pSamples, 2*2*audioFrame->GetSampleFrameCount());
    }   

    if (g_t0 < 0) {
        g_t0 = GetTickCount();
    } else if (g_pqv->IsEnable() == false) {
        long long t1 = GetTickCount();
        if ((t1 - g_t0) >= g_out_delay) {
            g_pqv->Enable();
            g_pqa->Enable();
        }
    }

    return S_OK;
}

HRESULT CDeckLinkInput::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents events, IDeckLinkDisplayMode *mode, BMDDetectedVideoInputFormatFlags)
{
	HRESULT	result;
	char*	displayModeName = NULL;

	if (!(events & bmdVideoInputDisplayModeChanged)) {
		return S_OK;
	}

	mode->GetName((const char**)&displayModeName);
	_l("[DIOI] Video format changed to %s\n", displayModeName);

	if (displayModeName) {
		free(displayModeName);
	}

	if (g_pDLI) {
		g_pDLI->StopStreams();

		result = g_pDLI->EnableVideoInput(mode->GetDisplayMode(), bmdFormat8BitYUV, bmdVideoInputEnableFormatDetection);
		if (result != S_OK) {
			_l("[DIOI] Failed to switch video mode\n");
			return result;
		}

		g_pDLI->StartStreams();
	}

	return S_OK;
}

