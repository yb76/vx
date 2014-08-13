////////////////////////////////////////////////////////////////////////////////
//
// Project    : Contactless Library (libctls)
//              Provides higher level functions for reading Contactless data.
//
// File Name  : ctlsmod.h
//
// Author(s)  : David Conn (david_c12@verifone.com)
//
// Version History
// 1.0  2007-Jun-16  David Conn           Creation (ported from MX platform)
//
////////////////////////////////////////////////////////////////////////////////

#ifndef CTLS_H_
#define CTLS_H_

#define CTLS_READ_RETRY		-2
////////////////////////////////////////////////////////////////////////////////
 
////////////////////////////////////////////////////////////////////////////////
//
// ctls callback type define
//
////////////////////////////////////////////////////////////////////////////////

typedef struct
{
	unsigned char AID[16];				// x9F06 - Application Identifier (AID)
	int Enabled;
	int AIDLength;
	int GroupNo;						// xFFE4 - Group Number
	int TranLimitExists;
	int FloorLimitExists;
	int CVMReqLimitExists;
	int TermCapCVMReqExists;

	unsigned char TranLimit[6];			// xFFF1 - Terminal Contactless Transaction Limit
	unsigned char FloorLimit[4];		// x9F1B - Termnal Floor Limit
	unsigned char CVMReqLimit[6];		// xFFF5 - CVM Required Limit
	unsigned char TermCap[3];			// x9F33 - Terminal Capabilites
	unsigned char TermCapNoCVMReq[3];	// xDF28 - Terminal Capabilities - No CVM Required
	unsigned char TermCapCVMReq[3];		// xDF29 - Terminal Capabilities - No CVM Required
	unsigned char AddTermCap[5];		// x9F40 - Additional Terminal Capabilities
	unsigned char DefaultTDOL[64];		// x97	 - Default Transaction Certificate Data Object List (TDOL)
	int DefaultTDOLLen;
	unsigned char TTQ[4];				// x9F66 - Terminal Transaction Qualifier (TTQ)
	unsigned char TACOnline[5];			// xFFFD - Terminal Action Code (Online)
	unsigned char TACDefault[5];		// xFFFE - Terminal Action Code (Default)
	unsigned char TACDenial[5];			// xFFFF - Terminal Action Code (Denial)

	unsigned char AppVer[5];	
	unsigned char TermPriority[5];	
	unsigned char MaxTargetD[5];
	unsigned char TargetPerD[5];
	unsigned char ThresholdD[11];

} AID_DATA;

AID_DATA* getCtlsAIDlist(void);
typedef void (*ctls_read_cb)(int timeout, int firsttime, unsigned char *data, int length);

////////////////////////////////////////////////////////////////////////////////
//
// ctls function prototypes
//
////////////////////////////////////////////////////////////////////////////////

int ctlsInitialise(int port);
int ctlsEnable( int timeout, int trantype, unsigned long amount, ctls_read_cb callback);
int ctlsDisable(void);
int ctlsGoodSeqNumber(unsigned int seqnumber);
int ctlsParseData(unsigned int *cardtype, long amount, unsigned char *data, int datalen);
int ctlsGetEMVTag(int EMVTag, unsigned char *data, int *datalen);
int ctlsSetEMVTag(int EMVTag, unsigned char *data, int datalen);
int ctlsGetTrackData(int trackno, unsigned char *trackdata, int *tracklen);
int ctlsOnlineAuthReq(void);
long ctlsGetLimit(int LimitType, char *AID);
void ctlsReset(void);
int nProcessESTFile(void);

////////////////////////////////////////////////////////////////////////////////

#endif
