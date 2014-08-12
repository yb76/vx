#ifndef __CECALLS_H_
#define __CECALLS_H_


#include <svc.h>
#include <ceif.h>

#define	RET_SUCCESS					0
#define	RET_FAILED					-1
#define	SCREEN_DELAY				500

#define	MAX_DATASIZE				4096
#define	MTU							3200	// original 1200
#define MAX_VERSION_LENGTH			10
#define	MAX_DISPLAY_BUFFER			128
#define MAX_IP_LENGTH				20
#define MAX_DATASIZE				4096
#define	MAX_USERNAME_LENGTH			20
#define	MAX_PASSWORD_LENGTH			20
#define	MAX_PHONE_LENGTH			20
#define	MAX_MEDIA_NAME				10
#define	MAX_FILE_LENGTH				12
#define	MAX_APN_LENGTH				20

#define NET_FAIL -1
#define NET_SUCCESS 0

#define GPRS_CONNECTED 0
#define GPRS_DISCONNECTED -1

#define COMM_ETH					"/DEV/ETH1"
#define	COMM_DIAL					"/DEV/COM3"
#define COMM_GPRS					"/DEV/COM2"
#define COMM_DPPP					"/DEV/COM3"

enum
{
	STATE_INIT = 200,
	STATE_OPEN ,
	STATE_LINK ,
	STATE_NETWORK,
	STATE_FAIL
};

typedef void (*CEEvent) (unsigned int nwifHandle, int neParam1, int neParam2, int neParam3, unsigned char* szData, int iDataLen);

typedef struct tagCEEvent {
	CEEvent OnCENetUp;
	CEEvent OnCENetDown;
	CEEvent	OnCENetFailed;
	CEEvent	OnCENetOut;
	CEEvent	OnCENetRestored;
	CEEvent	OnCEEvtSignal;
	CEEvent	OnCEStartOpen;
	CEEvent OnCEStartLink;
	CEEvent OnCEStartNW;
	CEEvent OnCEStartFail;
	CEEvent OnCEStopNW;
	CEEvent	OnCEStopLink;
	CEEvent OnCEStopClose;
	CEEvent	OnCEStopFail;
	CEEvent	OnCEEvtDDIAPPL;
} ceEvent_t;


int InitComEngine (void);
int InitCEEvents (void);
int InitNWIF (void);
int NetworkConfig (int iMedia);

// Callback function prototypes

void MainMenu_OnNetUp (unsigned int nwifHandle, int neParam1, int neParam2, int neParam3, unsigned char* szData, int iDataLen);
void MainMenu_OnNetDown (unsigned int nwifHandle, int neParam1, int neParam2, int neParam3, unsigned char* szData, int iDataLen);
void MainMenu_OnStartFail (unsigned int nwifHandle, int neParam1, int neParam2, int neParam3, unsigned char* szData, int iDataLen);
void MainMenu_OnStartOpen (unsigned int nwifHandle, int neParam1, int neParam2, int neParam3, unsigned char* szData, int iDataLen);

void MainMenu_OnNetOut (unsigned int nwifHandle, int neParam1, int neParam2, int neParam3, unsigned char* szData, int iDataLen);
void MainMenu_OnNetRestored (unsigned int nwifHandle, int neParam1, int neParam2, int neParam3, unsigned char* szData, int iDataLen);

void MainMenu_OnSignal (unsigned int nwifHandle, int signalPercent, int signaldBm, int signalRSSI, unsigned char* szData, int iDataLen);
void MainMenu_OnNetFailed (unsigned int nwifHandle, int neParam1, int neParam2, int neParam3, unsigned char* szData, int iDataLen);


void MainMenu_OnStartNW (unsigned int nwifHandle, int neParam1, int neParam2, int neParam3, unsigned char* szData, int iDataLen);
void MainMenu_OnStopClose (unsigned int nwifHandle, int neParam1, int neParam2, int neParam3, unsigned char* szData, int iDataLen);
void MainMenu_OnStopFail(unsigned int nwifHandle, int neParam1, int neParam2, int neParam3, unsigned char* szData, int iDataLen);
void MainMenu_OnStopLink (unsigned int nwifHandle, int neParam1, int neParam2, int neParam3, unsigned char* szData, int iDataLen);

void MainMenu_OnStopNW (unsigned int nwifHandle, int neParam1, int neParam2, int neParam3, unsigned char* szData, int iDataLen);
void MainMenu_OnStartLink(unsigned int nwifHandle, int neParam1, int neParam2, int neParam3, unsigned char* szData, int iDataLen);
void MainMenu_OnEvtDDIAPPL (unsigned int nwifHandle, int neParam1, int neParam2, int neParam3, unsigned char* szData, int iDataLen);


stNIInfo GetCurrMediaInfo (stNIInfo stArray[], int arrayCount, const char* szMedia);

void vdDisplayGPRSSignalStrength(int inSignalStatus, int inNetStatus,int x,int y);
void vdDisplayGPRSSignalStrength(int inSignalStatus, int inNetStatus,int x,int y);
void ListNWIF (stNIInfo stArray[], int arrayCount);

//int SocketConnect(int* pSocketHandle,char * tAPN, char * tHIP, int tPort);
//int inSendTCPCommunication(char *pchSendBuff, int inSendSize);
//int inReceiveTCPCommunication(char *pchReceiveBuff, int inReceiveTimeout);

int ceTcpConn2(void);

#endif	// __CECALLS_H_
