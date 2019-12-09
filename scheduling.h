#ifndef _SCHEDULING_H_
#define _SCHEDULING_H_

#include "global.h"

#define MAX_NUM_COMMON			256
#define MAX_NUM_HISTORY			5
#define MAX_NUM_SCHEDULES		128	
#define MAX_NUM_ADFILE			64
#define MAX_NUM_INDEXED         512

typedef struct 
{
	int is_used;
    int is_after;
	
	int dum_start;
	int dum_end;

	int index;
	int first;

	TIME start;

	char title[256];
	char program_id[64];
	char brd_ymd[32];
	
	DInfo di[MAX_NUM_CM];
	CAdFile *pa[MAX_NUM_CM];

    //manual mode only
    int count;
}PL; // ADfile program list

typedef struct
{
    DInfo di[MAX_NUM_COMMON];
	CAdFile *pa[MAX_NUM_COMMON];

	int pos;
	int count;
}CAL; // common adfile list

typedef struct tagSCHE
{
    char lic_info;

    char title[64];
    char program_id[64];
    char brd_ymd[32];
	//int run;
	//int epi;

	int pre_count;
	int post_count;

	DInfo pre_file[MAX_NUM_ADFILE];
	DInfo post_file[MAX_NUM_ADFILE];

    TIME start;
    TIME end;

}SCHE;

class CMySchedule : public PThread
{
public:
	CMySchedule();
	~CMySchedule();

	void LoadFile(char *filename);

	void Put(SCHE *pc);
	SCHE *Get(int nIndex) { 
		if (m_pSche[nIndex]) {
			return m_pSche[nIndex];
		}
		return NULL;
	};

    void AddSchedule(SCHE *ps);

	void Print();
	void Create(void *pGD) {
		m_pGD = pGD;
		Start();
	};

	PL *GetPlayList();
	void CompletePlayList();

protected:

	PL m_pl;
	void *m_pGD;

	pthread_mutex_t m_mutex;

	SCHE *m_pSche[MAX_NUM_SCHEDULES];
    bool m_bIsUpdate;

    CAdFile *m_pAdFile[MAX_NUM_INDEXED];

protected:

    void Run();
    void OnTerminate() {};

protected:

    CAdFile *GetAdFile(char *filename);
    void UpdateAdFiles();
    void UpdateCommonList(CAL *pcal);
};

#endif // _SCHEDULING_H_
