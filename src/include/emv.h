
#ifndef _EMV_H_
#define _EMV_H_

#include <cardslot.h>
#include <vxemvap.h>
#include <vxemvapdef.h>
#include <EMVResult.hpp>
//#include <emvcommon.h>

#include <estfuncs.h>
#include <mvtfuncs.h>

#include "auris.h"

typedef struct {
	unsigned char pin[10] ;
	bool sign ;
	bool pinentry;
	bool onlinepin;
	bool offlinepin;
	bool pinbypass_enable;
	bool cracct_disable;
	long amt ;
	unsigned short termDesicion;
	unsigned short appsTotal;
	bool eftpos_mcard;
	bool chipfallback; // offline
	bool techfallback; // fallback to swipe
	char acct[10] ;
	bool acct_nocr ;
	char tac_denial[11] ;
	char tac_default[11] ;
	char tac_online[11] ;
} EMV_GLOBAL;
extern EMV_GLOBAL gEmv;

typedef enum	E_EMV
{
    ERR_EMV_NONE = 0,					/* Operation completed successfully */
    ERR_EMV_FAILURE = 100,				/* 100-A critical error occurred (100)*/
    ERR_EMV_CARD_REMOVED,				/* 101-Card was removed before the transaction was completed (101) */
    ERR_EMV_ALREADY_INIT,				/* 102-Initialisation function has been called already (102) */
    ERR_EMV_DATA_NOT_AVAILIBLE,			/* 103-Requested data is not available (103) */
    ERR_EMV_PARAMETER_SET_FAILED,		/* 104-Parameter set/update operation failed (104) */
    ERR_EMV_TABLE_FULL,					/* 105-Configuration data tables maintained	by the module are full (105) */
    ERR_EMV_CONFIG_ERROR,				/* 106-Data required to start a transaction has not be set in the module (106) */
    ERR_EMV_REQUIRED_DATA_MISSING,		/* 107-Data required to complete the transaction is not available (107) */
    ERR_EMV_INVALID_PARAMETER,			/* 108-Operation failed because an invalid parameter was passed in (108) */
    ERR_EMV_FUNCTION_MISSING,			/* 109-Required callback function required has not been implemented (109) */
    ERR_EMV_PIN_TIMEOUT,				/* 110-Operation timed out waiting for the user to enter a PIN (110) */
    ERR_EMV_PIN_CANCEL,					/* 111-Operation was cancelled by the user at the PIN entry stage (111) */
    ERR_EMV_APPLICATION_BLOCKED,		/* 112-One or more applications on the card is/are blocked. Fallback to magnetic-stripe
    												operation should not be allowed (112) */
    ERR_EMV_CARD_ERROR,					/* 113-Invalid response was received from the card (113) */
    ERR_EMV_CARD_FAILURE,				/* 114-Error occurred during communication with the
    												card. This error is only returned prior to
    												application selection (114) */
    ERR_EMV_CANDIDATE_LIST_EMPTY,		/* 115-No applications could be found that are supported
    												by both the card & the EMV module on the terminal.
    												This error will only be returned if there are no
    												blocked applications on the card (115) */
    ERR_EMV_CLIENT_ABORTED,				/* 116-Client application requested that the transaction be aborted (116) */
    ERR_EMV_CLIENT_BACKSTEP,			/* 117-Client application requested that the transaction
    												be returned to the previous step in processing (117) */
    ERR_EMV_CARD_BLOCKED,				/* 118-Card is blocked. Fallback to magnetic-stripe operation should not be allowed (118) */
	ERR_EMV_NO_RETRY_FAILURE,			/* 119-An error occurred when the selected application
													was started. Fallback to magnetic-stripe
    												operation may be allowed (119) */
	ERR_EMV_EXPLICIT_SELECT_REQD,		/* 120-Application must be selected explicitly (ie: cannot be selected automatically) (120) */
	ERR_EMV_CBFN_ERROR,					/* 121-Callback function returned an error (121) */
	ERR_EMV_CAPK_DATA,					/* 122-CAPK data is missing (122) */
	ERR_EMV_CAPK_EXPIRED,				/* 123-CAPK has expired (123) */
	ERR_EMV_CAPK_CHECKSUM,				/* 124-CAPK checksum is invalid (124) */
	ERR_EMV_DECLINE_OFFLINE,			/* 125-Transaction should be declined offline (125) */
	ERR_GO_ONLINE,						/* 126-Transaction has to be sent online 126*/

} E_EMV;

/**
**-------------------------------------------------------------------------->
** Maximum Ratings
**-------------------------------------------------------------------------->
*/
#define EMV_RID_LENGTH                     	5		/* EMV Registered Application Provider length */
#define EMV_MAX_AID_LENGTH                 	16		/* EMV Application-ID maximum length */
#define EMV_MAX_ASCII_AID_LENGTH           	((EMV_MAX_AID_LENGTH*2) + 1)	/* EMV Application-ID maximum length (ascii) */
#define EMV_APP_VERSION_LENGTH             	2		/* EMV Application version length */
#define EMV_MAX_APPLICATION_NAME           	16		/* EMV Application name length */
#define EMV_APPLICATION_PREF_NAME_LENGTH  	32		/* EMV Preferred application name maximum length (bytes) */
#define EMV_SUPPORTED_APP_VERSIONS         	4		/* Maximum number of EMV applications supported */
#define EMV_MAX_CARD_APPLICATIONS          	16		/* Maximum number of EMV applications on a card */

#define EMV_MAX_CAPK_MODULUS               	248		/* CAPK modulus maximum length (bytes) */
#define EMV_CERT_SERIAL_LENGHT             	3		/* CAPK Certificate Serial Number length */

#define EMV_MERCHANT_CAT_CODE_LENGTH       	2		/* EMV Merchant Category Code length */
#define EMV_MAX_MERCHANT_NAME              	128		/* EMV Merchant name maximum length */
#define EMV_MERCHANT_ID_LENGTH             	15		/* EMV Merchant-ID length */

#define EMV_PIN_TRY_COUNTER_LENGTH         	1		/* EMV PIN attempts counter length */
#define EMV_MIN_PIN_LENGTH                 	4		/* Minium PIN length */
#define EMV_MAX_PIN_LENGTH                 	12		/* Maximum PIN length */

#define EMV_AC_LENGTH                      	8		/* EMV AC length */
#define EMV_ACCOUNT_TYPE_LENGTH			   	1		/* EMV account-type length */
#define EMV_ACQUIRER_ID_LENGTH             	12		/* Acquirer-ID length */
#define EMV_ARC_LENGTH                     	2		/* EMV Authorisation Response Code length */
#define EMV_ATC_LENGTH                     	2		/* EMV Application Transaction Counter ength */
#define EMV_AUTH_CODE_LENGTH               	6		/* EMV Authorisation Code length */
#define EMV_BINARY_AMOUNT_LENGTH           	4		/* EMV transaction amount (binary) length */
#define EMV_BCD_AMOUNT_LENGTH              	6		/* EMV transaction amount (BCD) length */
#define EMV_CID_LENGTH                     	1		/* EMV Cryptogram Information Data length */
#define EMV_COUNTRY_CODE_LENGTH            	2		/* EMV Country Code length */
#define EMV_CURRENCY_CODE_LENGTH           	2		/* EMV Currency Code length */
#define EMV_MAX_CVM_LIST_LEN               	252		/* EMV Cardholder Verification Methods list maximum length */
#define EMV_CVM_RESULT_LENGTH              	3		/* EMV Cardholder Verification Results length */
#define EMV_MAX_DEFAULT_DOL_LENGTH         	128		/* EMV Default DOL maximum length (bytes) */
#define EMV_TRANS_CAT_CODE_LENGTH          	1		/* EMV Transaction Category Code length */
#define EMV_MAX_ISS_AUTH_DATA_LENGTH       	16		/* EMV Issuer Authentication Data length */
#define EMV_PAN_LENGTH                     	10		/* Primary Account Number length */
#define EMV_MAX_ASCII_PARAM_LENGTH         	512		/* Maximum length of an ascii tag */
#define EMV_SHA1_HASH_LENGTH               	20		/* SHA-1 hash length */
#define EMV_TAC_LENGTH                     	5		/* EMV Terminal Action Code length */
#define EMV_TRANSACTION_TYPE_LENGTH        	1		/* EMV Transaction Type length */
#define EMV_TSI_LENGTH                     	2		/* EMV Transaction Status Information length */
#define EMV_TVR_LENGTH                     	5		/* EMV Terminal Verification Results length */

#define EMV_TERM_TYPE_LENGTH               	1		/* Terminal type length */
#define EMV_TERMCAP_LENGTH                 	3		/* Terminal Capababilities data length */
#define EMV_ADD_TERMCAP_LENGTH             	5		/* Terminal Additional Capababilities data length */
#define EMV_TERMINAL_ID_LENGTH             	8		/* Terminal-ID length */
#define EMV_TERMINAL_SERIAL_LENGTH         	8		/* Terminal serial number length */
#define	EMV_AIP_LENGTH						2		/* Application Interchange Profile length */

#define	EMV_T2_DATA_LEN_MAX					19		/* Application Track-2 data maximum length */
#define	EMV_PAN_LEN_MAX						10		/* Application PAN maximum length */
#define	EMV_EXP_DATE_YYMMDD_LEN_MAX			3		/* Application Exp Date (YYMMDD) maximum length */
#define	EMV_EXP_DATE_YYMM_LEN_MAX			2		/* Application Exp Date (YYMM) maximum length */



/**
**-------------------------------------------------------------------------->
** name EMV Transaction Response Codes
**-------------------------------------------------------------------------->
*/
#define	RESP_CODE_EMV_OFFLINE_APPROVED    "Y1"	/* Transaction approved offline */
#define	RESP_CODE_EMV_DEFAULT_APPROVED    "Y3"	/* Transaction approved */
#define	RESP_CODE_EMV_OFFLINE_DECLINED    "Z1"	/* Transaction declined offline */
#define	RESP_CODE_EMV_DEFAULT_DECLINED    "Z3"	/* Transaction declined */
#define	RESP_CODE_EMV_ONLINE_DECLINED     "Z4"	/* Transaction declined online */

/**
**-------------------------------------------------------------------------->
** Maximum Ratings
**-------------------------------------------------------------------------->
*/
#define	EST_REC_AID_MAX			10		/* Max no. of AID's in each EST.DAT record */
#define	EST_REC_CAPK_MAX		15		/* Max no. of CAPK's in each EST.DAT record */
#define	CAPK_EXP_DATE_LEN		6		/* CAPK Expiry Date Length */

#define EMV_MAX_AID_RECORDS     	32		/*!< \brief Maximum number of AID records */
#define EMV_MAX_EXCEPTION_RECORDS	32		/*!< \brief Maximum number of EXCEPTION records */
#define EMV_MAX_CAPK_RECORDS    	32		/*!< \brief Maximum number of CAPK records */
#define EMV_MAX_CSN_RECORDS      	32		/*!< \brief Maximum number of CSN records */

/**
**-------------------------------------------------------------------------->
**Miscellaneous Definitions
**-------------------------------------------------------------------------->
*/
#define	EST_CAPK_INDEX_INVALID	256						/* Value used to signify an invalid
																CAPK Index in the Verifone VerixV
																EMV Module EST.DAT file */

/* Function declarations and prototypes */
int	EmvSetTagData(const unsigned char* pcTag,const unsigned char* pbDataBuf,int iDataBufLen);
void	EmvSw1Sw2Set(unsigned short usStatus);
unsigned short	EmvTag2VxTag(const unsigned char* pcTag);
unsigned short	EmvTagLen(const unsigned char* pcTag);
//bool	EmvIsCardPresent(void);
void	EmvSw1Sw2Init(void);
unsigned short	EmvSw1Sw2Get(void);
void	EmvCallbackFnPromptManager(unsigned short usPromptId);
int	EmvCallbackFnSelectAppMenu(char **pcMenuItems, int iMenuItemsTotal);
unsigned short	EmvCallbackFnAmtEntry(unsigned long *pulTxnAmt);
unsigned char	EmvCallbackFnIsUsrCncl(void);
int	EmvCardDataReadAuth(void);
int		EmvTotalApps(void);
int	EmvMiscVerixVEmvErr2RisEmvErr(int iVerixVEmvErr);
int		EmvGetTagData(const unsigned char* pcTag, unsigned char* pbDataBuf, int *piTagLength, int iDataBufLen);
static int	EmvIsCardDomestic(unsigned char* pfIsCardDomestic);
void	EmvDbAidDataInit(void);
int		EmvDbGetAidIndex(const unsigned char *pbAid);
static int	EmvDbEstRecLoad(int iRecNo);
int	EmvMiscAscii2LenBinArray(char* pcDataAscii, int iDataAsciiLen, unsigned  char* pbDataBin, int iDataBinLenMax);
int		EmvMiscAidMatchLen(unsigned char* pbAid1, unsigned char* pbAid2);
static int	EmvDbEstRecRestore(void);
static int	EmvDbMvtRecLoad(int iRecNo);
static int	EmvDbMvtRecRestore(void);
unsigned short	EmvSignature(void);
unsigned short	EmvOnlinePin(void);
unsigned short	EmvGetUsrPin(unsigned char *ucPin);
int	EmvCallbackFnInit(void);
unsigned short	EmvFnAmtEntry(unsigned long *pulTxnAmt);
unsigned short	EmvFnCandListModify(srAIDList *psrCandidateList);
unsigned short	EmvFnLastAmtEntry(unsigned long *pulTxnAmt);
static int	EmvIssuerAuth(void);
static int	Emv71ScriptProc(void);
static int	Emv72ScriptProc(void);
void	EmvAidLoad(void);
int EmvPopulateTags (void);
unsigned char * EmvPackField55 (char *fileName);
int	EmvCardholderVerify(void);
int EmvGetAmtCallback(void);
int EmvGetOfflinePinCallback(void);


/*
* Functions to be called by the payment application
*/
int	EMVPowerOn(void);
BYTE	EMVGetAccountsAvailable(void);
int	EMVProcessingRestrictions (void);
int	EMVDataAuth(void);
int	EMVTransPrepare(void);
void	EMVForceOnlineOrDecline (int decision);
int		Emv2ndAcGen(int iHostTxnDecision);
int	EmvProcess1stAC(void);
int		EmvTxnProcessOnline(int hostStatus);
int	EmvCardPowerOff(void);

bool EmvIsCardPresent(void);

short CheckScriptLen( byte *Scdata , int totalLen );

void vPrintEMVAllAids(void);
int inPrintCTLSEmvPrm(void);
int bcd_2_asc( char *chars_in,char *chars_out,int input_size );
void BCD2ASC(unsigned char src, char *dest);
long Hex_To_Dec(char *s );
int CTLSEmvGetTac(char *tac_df,char *tac_dn,char *tac_ol, const char *AID);

#endif
