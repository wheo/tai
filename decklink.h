#ifndef _DECKLINK_H_
#define _DECKLINK_H_

#include "decklink_input.h"

#define MAX_DECK_PORT		2

class CDeckLink
{
public:
	CDeckLink();
	~CDeckLink();

	bool Init();

protected:

	IDeckLinkIterator *m_pDeckLinkIterator;
	IDeckLink * m_pDeckLink[MAX_DECK_PORT]; // 1 for Input, another for Output

	CDeckLinkInput *m_pInput;

};
#endif

