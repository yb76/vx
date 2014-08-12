#ifndef __GLOB_H
#define __GLOB_H

#ifdef __cplusplus
extern"C" {
#endif

#include <acldev.h>
#include "Date.h"
#include "define.h"
#include "EMVTypeDefs.hpp"
#include "EMVCStructs.h"

extern int HPrinter;
extern int HClock;
extern int HMagRead;

extern int  pinSigRequired;
extern char Perform_Chip;
extern int  APP_Type;
extern int  ExtPPadPrsnt; // Ext pinpad IN USE FOR CURRENT TXN (use EppConnFlg to see if it's physically connected)
extern long Bill_Amount;
extern char ExtendedVoucher;
extern char SWIPEFLAG;
extern int  doingSplitPay;
extern char pinEntryCancelled;
extern char Rs232ConxnFlg;
extern char* szKeyMap;
extern char APP_PATH[30];
extern char PrinBuff[100];
extern int	MerchBodyFlg;
extern int	MerchHdrFlg;
extern int	stayOfflineOpt;
extern int hospitalityEnabled;

extern int USER_BACKLIGHT_FORCE_OFF; // WHY ARE YOU SHOUTING? //COS NOBODY F*CKIN LISTENS // :-)

//#include "currency.h"
	
typedef struct CURCY_
{
	char AbbName[5],
		PreChars[10],
		MidChars[10],
		PostChars[10],
		Name[40];
	int DPs;
}CURCY; //Currency
	/*
typedef struct TRACK
{
	char acct[23];
	char exp[5];
	char name[27];
	char type[4];
	char PVN[6];
	char disc[17];
	char track[108];
	
}TRACK;
*/

typedef struct ACCOUNT_
{
	char  Name[40];
	char  Number[20];

	CURCY Currency;

	long  Flags1;         // See ACCF_... defines

	char  Ref_Pin[5];

	int   ModOffline;
	int   ModPreAuth;
	int   ModContAuth;    // Continuous Authority
	int   ModBet;         // Spread / Telebet
	int   ModCNP;
	int   ModEcommerce;

	char  GroupId[100];
	char  AuthCentBuff[91];
	char  AuthCentreID[3];
	char  SubCentBuff[91];
	char  SubCentreID[3];
	char  IsoCurCode[4];

	long  AccountId;
	char  TID[9];

	char  VoucherHead[22];
	char  MaxRefVal[12];

	int   PrintMerchantReceipts;

	int   GratuityCheckValue;

	char  purchasePin[5];
} ACCOUNT;

typedef struct TXN
{
	char		Origin,         //keyed ,swiped or ICC
				ReprntMerchVchr;
	int			TxnType,        //purchase ,refund
                WTIRes,
				CaptureMethod;	// Added 2005-09-15 (AT) for Ocius v3
	long		Modifier,        //winti modifier
				BillId;


	char		Track1 [TRACK_LEN + 1],
				Track2 [TRACK_LEN + 1],
				Track3 [TRACK_LEN + 1],
				PAN	[PAN_LEN + 1];
	Date		ExpiryDate;
	Date		StartDate;
	char		IssueNumber[3],
				CV2 [5],
				AuthCode[21],
				CopyAuthCode[21],
				MRef [40 + 1],	// Changed 2005-03-09 Neil Clark: To allow for null-terminator
                DateTime [20], // CCYYMMDDHHMMSS
                MID [20],
                TID [10],
                Scheme [61], // Card scheme name
				Floorlimit[8],
                EFTSN [9],
                Message [100],
                VTel    [50],
				Emv_Pin[14],
				Refpin [10],
				AuthRespCode[4],
				ARPC[17],
				IssOptData[17],
				IssScriptData[258+1],
				RespAddData[7],
				IssuerAuthdata[33];

	char		AmountValue[12],
				AmountCashBack[12],
				AmountGratuity[12],
				AVS[41];

	unsigned long Txn_Id;

} TXN;
int IsRefundOrChargeOnly(TXN* theTransaction);


typedef struct IIN_ENTRY_
{
	char		LowRange [11],
				HiRange  [11],
				PANLens  [21],
				Scheme	 [61];

	long		AllowedTxns,
				AllowedMods,
				Specials;

	int			IssueLen,
				ValidityLen,
				IssuePos,
				ValidityPos;

	char		Group_Id[4];

} IIN_ENTRY;


typedef struct RECEIPT_
{
	char 		MsgNum[4];
	char		buff[100];

	char        **MerchHdr,
				**MerchFtr,
			    **CustHdr,
		        **CustFtr;
	short		MhdnoLine,
				MFtnoLine,
				ChdnoLine,
				CFtnoLine;
} RECEIPT;

typedef struct CHIP_
{
	unsigned char	EMV_Term_Type[3];
	unsigned char	Term_CountryCode[5];
	unsigned char	ReasonOnlineCode[3];
	unsigned char	AppCryptogram[41];
	unsigned char	AppPanSeqNum[3];
	unsigned char   AIP[5];
	unsigned char   ATC[5];
	unsigned char   UnpredNum[9];
	unsigned char	TVR[17];
	unsigned char	CrypTransType[3];
	unsigned char	IADscheme[33],
					IADIssuer[70],
					AID[33],
					AppUsControl[5],
					CrypInfoData[4],
					CVM[8],
					CardAppVer[5],
					TSI[6],
					TermAppVer[5],
					TerType[3],
					TCAPS[7],
					PosEntryMode[3],
					OtherCardData[27],
					IAC[31],
					Iac_Online[12],
					Iac_denial[12],
					Iac_Default[12],
					TAC_Online[12],
					TAC_denial[12],
					TAC_Default[12],
					TC[41],
					AppEffdate[8],
					AppExpDate[8],
					ICCServiceCode[4],
					CrdHldname[54],
					Applabel[17],
					AppPrefName[17],
					IssuerCodeTableIndex[3],
					FloorLimit[11],
					TDOL[256],
					DDOL[256],
					MerchantCategoryCode[5],
					CAPKIndex[3];
	unsigned short  Force_Auth;
	int				Auth_Type;
	char			Currency_Code[4];
} CHIP;


typedef struct T_RECORD_
{
	short           AmountModified;
	int             TxnType;
	unsigned char	RecordType,
					AccountNumber		[ 3+1],
					CardNumber          [19+1],
					Expiry				[ 4+1],
					Issue				[ 2+1],
					StartDate           [ 4+1];
	char            TxnValue			[11+1];
	unsigned char	Cashback			[11+1],
					AuthCode			[ 9+1],
					DateTime			[14+1],
					Reference			[40+1];

	long            AccountId,
					Modifier,
					BillId;
} T_RECORD;

extern int       offset;
extern CURCY     CURNCY;
extern ACCOUNT   ACCNT;
extern TXN       Trans;
extern IIN_ENTRY IIN;
extern RECEIPT	 Voucher;
extern CHIP		 ICC_Details;
extern T_RECORD  tRec;
extern struct TRACK Card_Info;
extern char		 fOffline;		///< Flag to show that an online authorisation could not be performed

extern int		 Undocked_G_Bool;
extern char		 PINBUFFER[5];
extern int		 PP_Status;

#ifdef __cplusplus
} // extern"C"
#endif

#endif // __GLOB_H
