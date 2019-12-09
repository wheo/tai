#include "global.h"
#include "decklink.h"

IDeckLinkInput *g_pDLI = NULL;
IDeckLinkOutput *g_pDLO = NULL;

CDeckLink::CDeckLink()
{
	for (int i=0; i<MAX_DECK_PORT; i++) {
		m_pDeckLink[i] = NULL;
	}
	m_pDeckLinkIterator = NULL;

	m_pInput = NULL;
}

CDeckLink::~CDeckLink()
{
	if (g_pDLO) {
		g_pDLO->DisableAudioOutput();
		g_pDLO->DisableVideoOutput();
		g_pDLO->Release();
		g_pDLO = NULL;
	}
	if (g_pDLI) {
		g_pDLI->StopStreams();
		g_pDLI->DisableAudioInput();
		g_pDLI->DisableVideoInput();
		g_pDLI->Release(); // m_pInput also released here..so don't delete again
		g_pDLI = NULL;
	}
	for (int i=0; i<MAX_DECK_PORT; i++) {
		if (m_pDeckLink[i]) {
			m_pDeckLink[i]->Release();
		}
	}
	if (m_pDeckLinkIterator) {
		m_pDeckLinkIterator->Release();
	}
}
		
bool CDeckLink::Init()
{
	m_pDeckLinkIterator = CreateDeckLinkIteratorInstance();
	if (!m_pDeckLinkIterator) {
		_l("[DIO] No Decklink drivers...\n");
		goto exit_dio_fail;
	}

	for (int i=0; i<MAX_DECK_PORT; i++) {
		char *name = NULL;
		if (m_pDeckLinkIterator->Next(&m_pDeckLink[i]) != S_OK) {
			_l("[DIO] Unable to get DeckLink device\n");
			goto exit_dio_fail;
		}

		m_pDeckLink[i]->GetModelName((const char**)&name);
		_l("[DIO] %s found\n", name);
	}

	if (m_pDeckLink[0]->QueryInterface(IID_IDeckLinkInput, (void **)&g_pDLI) != S_OK) {
		_l("[DIO] No input port...\n");
		goto exit_dio_fail;
	}
	if (m_pDeckLink[1]->QueryInterface(IID_IDeckLinkOutput, (void **)&g_pDLO) != S_OK) {
		_l("[DIO] No output port...\n");
		goto exit_dio_fail;
	}

	m_pInput = new CDeckLinkInput();
	if (g_pDLI->SetCallback(m_pInput) != S_OK) {
		_l("[DIO] Failed to set input callback\n");
		goto exit_dio_fail;
	}

	if (g_pDLI->EnableVideoInput(bmdModeHD1080i5994, bmdFormat8BitYUV, bmdVideoInputEnableFormatDetection) != S_OK) {
		_l("[DIO] Failed to enable video input\n");
		goto exit_dio_fail;
	}

	if (g_pDLI->EnableAudioInput(bmdAudioSampleRate48kHz, bmdAudioSampleType16bitInteger, 2) != S_OK) {
		_l("[DIO] Failed to enable audio input\n");
		goto exit_dio_fail;
	}

	if (g_pDLO->EnableVideoOutput(bmdModeHD1080i5994, bmdVideoOutputFlagDefault) != S_OK) {
		_l("[DIO] Failed to enable video output\n");
		goto exit_dio_fail;
	}

	if (g_pDLO->EnableAudioOutput(bmdAudioSampleRate48kHz, bmdAudioSampleType16bitInteger, 2, bmdAudioOutputStreamContinuous) != S_OK) {
		_l("[DIO] Failed to enable audio output\n");
		goto exit_dio_fail;
	}

	if (g_pDLI->StartStreams() != S_OK) {
		_l("[DIO] Failed to start input stream\n");
		goto exit_dio_fail;
	}

	return true;

exit_dio_fail:
	if (g_pDLI) {
		g_pDLI->StopStreams();
		g_pDLI->DisableAudioInput();
		g_pDLI->DisableVideoInput();
		g_pDLI->Release(); // m_pInput also released here..so don't delete again
	}
	if (g_pDLO) {
		g_pDLO->DisableAudioOutput();
		g_pDLO->DisableVideoOutput();
		g_pDLO->Release();
	}

	for (int i=0; i<MAX_DECK_PORT; i++) {
		if (m_pDeckLink[i]) {
			m_pDeckLink[i]->Release();
		}
	}
	if (m_pDeckLinkIterator) {
		m_pDeckLinkIterator->Release();
	}

	for (int i=0; i<MAX_DECK_PORT; i++) {
		m_pDeckLink[i] = NULL;
	}
	m_pDeckLinkIterator = NULL;

	g_pDLI = NULL;
	g_pDLO = NULL;

	return false;
}

