
#pragma region Defines_returnVals

#define CTLS_FAILED 0x00
#define CTLS_SUCCESS 0x01
#define CTLS_NO_CARD 0x02

//NIBBLE 2:CardSpecType 1:ReaderRes
#define CTLS_SPEC_TYPE 0xF0
#define CTLS_RDR_RES 0x0F

#define CTLS_UNKNOWN 0x10
#define CTLS_MSD 0x20
#define CTLS_EMV 0x30

#define CTLS_AUTHD 0x01
#define CTLS_DECLINED 0x02
#define CTLS_REQ_OL 0x03
#define CTLS_FALL_FORWARD 0x04
#define CTLS_RETRY 0x05
#define CTLS_TRY_CONTACT 0x06

#define CTLS_OTHER_ACQUIRE -11
#define CTLS_ALREADY_ACQ_CARD -16
#define CTLS_RSP_TIMED_OUT -12
#define CTLS_NO_REPLY -13
#define CTLS_RSP_BUFF_FULL -14
#define CTLS_CRC_CHK_FAILED -15
#define CTLS_BAD_CARD_READ -17
#define CTLS_NO_CLR_REC -18
#define CTLS_TLV_ERR_T -19
#define CTLS_TERM_TIMED_OUT -20

#pragma endregion

typedef struct _ctlsStru_
{
	char sTLvData[1024];
	int nTLvLen;
	char sTrackOne[99];
	char sTrackTwo[49];
	char sAmt[15];
	char TxnStatus[3];
	int nosaf;
} _ctlsStru ;

int  RdrCommsOk			(void);
int  ProcessCard		(void);
int  SetLCDMessage		(char msgNdx);
void MaintainLCDMsg		(char msgNdx);
void CancelAcquireCard	(int reason);
void SendHeartBeats		(void);
