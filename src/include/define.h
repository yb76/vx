#ifndef __DEF_H
#define __DEF_H

#define Version "v02.00.05.20"

#define O3600_P

// Optional compilation arguments
#define _USE_SSL /* don't check in with this line commented out! */

#define _DUAL_CVM // if defined, print "PIN & Signature" on receipts when appropriate

//#define _PPDBG
#define _DEBUG_SC5000_BASE
//#define _DEBUG_SC5000_VERBOSE
//#define _DEBUG_ACCID
#define GEN_DEBUG //compiles in all the Nprintfs.

#define CASH_PAYMENT		3
#define KBD_F1				250
#define KBD_F2				251
#define KBD_F3				252
#define KBD_F4				253
#define KBD_CLR				155
#define MY_EVT_TIMER		(1L<<30)

#define INVALID             -1
#define MSG_FILE_ERR	    -2
#define CLOCK_OPEN_ERR      -3

#define ENTER_KEY			 0x0D
#define INVALIDTXN          -2		// Invalid transaction type
#define CANCEL_PRESS	    -3
#define UNKNOWN_CARD		-4		// IIN range not found
#define UNACCEPTABLE_CARD	-5		// IIN range not accepted
#define BAD_LUHN			-6		// Invalid LUHN check digit
#define BAD_PAN_LEN			-7		// Invalid PAN length
//#define BAD_PCD			-8		// Invalid penultimate check digit (no longer used)
#define EXPIRED_CARD		-9		// Card expiry date has passed
#define NOT_YET_VALID		-10		// Start date has not passed

#define ENTER_PRESS	        13      // '\r'

#define TRUE				1
#define FALSE				0

#define ZERO				0 // why?

#define SETUPPIN			"X8K2"

#define OPENTIMEOUT			25000  // Time out value, in milliseconds,  to be used by UCL Open method
#define CONNECTTIMEOUT		20000  // Time out value, in milliseconds,  to be used by UCL Connect method
#define DISCONNECTTIMEOUT	9000   // Time out value, in milliseconds,  to be used by UCL Disconnect method
#define CLOSETIMEOUT		9000   // Time out value, in milliseconds,  to be used by UCL Close method
#define SENDTIMEOUT			6000   // Time out value, in milliseconds,  to be used by UCL Send method
#define RECEIVETIMEOUT		15000
#define MAX_WINTI_TIMEOUT	60000  // winti timeout to get response from host

#define IIN_PATH			"F:IINTable.txt"
#define LOG_PATH			"I:Log%s.txt"		//	%s gets replaced with Mon, Tues etc
#define ACCOUNT_PATH		"I:Accounts.txt"
#define CFG_PATH			"I:Config.txt"
#define MENU_PATH           "I:MENU.txt"
#define RECPT_PATH			"I:Voucher.txt"
#define RECPT_PATHB			"I:VoucherB.txt"

#define CARD_SCHEME_PATH	"F:CardScheme.dat"
#define ACC_CS_PATH			"I:AccCS.dat"

#define MAX_NUM_ACCOUNT     10
#define BUFFERSIZE          1024
#define FLDLDBLKSZ			416
#define WIFIFLBLKSZ			216

#define NUM_DISPLAY_LINE	3
#define MAX_AMT_RANGE		1000000000L
#define MIN_AMT_RANGE		0L
#define TRACK_LEN			200
#define PAN_LEN				23
#define MERCH_LEN			40
#define AUTH_LEN			9
#define MAX_TID_LEN         9
// accounts.txt file Flags

#define MERCH_REF_NUM_FLG	0x00000004  // flag in account.txt
#define GRATUITY_FLG		0x00000008
#define ACC_PWCB_FLG		0x00000001
#define ACC_PURCH_FLG		0x00000010
#define ACC_REFUND_FLG		0x00000020
#define ACC_MODIFIER_FLG    0x00000080
#define ACC_CASHADV_FLG     0x00000040
#define MERCH_REF_FORCE_FLG 0x00000100

//IIN Allowed transaction type
#define TRANS_ALLOW_PWCB	0x00000010
#define TRANS_ALLOW_CA		0x00000008
#define TRANS_ALLOW_PUR		0x00000001
#define TRANS_ALLOW_REF		0x00000002

//IIN special modifiers (RFU = Rugby Football Union^W^W^WReserved for Future Use)
#define IIN_SPC_ONLINE_ONLY        0x00000001
#define IIN_SPC_SWITCH             0x00000002
#define IIN_SPC_SPECIAL_MASTERCARD 0x00000004
#define IIN_SPC_AMEX               0x00000008

#define IIN_SPC_GE_CAPITAL         0x00000010
// RFU                             0x00000020
// RFU                             0x00000040
// RFU                             0x00000080

#define IIN_SPC_VISA_CPC           0x00000100
#define IIN_SPC_AMEX_CPC           0x00000200
#define IIN_SPC_ALLSTAR_CPC        0x00000400
// RFU                             0x00000800

#define INTL_MAESTRO               0x00001000
#define DONTCHKEXPDT               0x00002000
#define DONTCHKPANLEN              0x00004000
#define DONTCHKLUHN                0x00008000

#define IIN_SPC_FUEL_CARD          0x00010000
// RFU                             0x00020000
#define IIN_SPC_ICC_ONLY           0x00040000
#define IIN_SPC_IGNORE_SERV_CODE   0x00080000
// end of IIN special modifiers

// extract
#define MSG_FIELD1			1
#define MSG_FIELD2			2

// File download Response
#define RES_OK              1
#define ERR                -2
#define FH                  3
#define FD                  4
#define E_O_F               5
#define PRN                 6
#define DONE                7
#define REQ_DLD            -100
#define NOT_RECOGNISED      0

// WINTI Modifiers
#define EFT_MOD_NOSPLHAND		0x00000000 // No special handling

#define EFT_INVALID 			0x00000001
#define EFT_MOD_OFFLINE			0x00000002 // Keep transaction offline. App will get the opportunity to supply an auth code.
#define EFT_MOD_ONLINE			0x00000004 // Disregard floor limits and force online.
#define EFT_MOD_CNP				0x00000008 // Cardholder not present (mail order etc.)
#define EFT_MOD_KYD				0x00200000 // Keyed Txn

#define EFT_MOD_PREAUTH			0x00000010 // Authorise but don't transfer funds.
#define EFT_MOD_CONT_AUTH		0x00000020 // Continuous authority
#define EFT_MOD_REKEY			0x00000040 // Transaction is being re-keyed (recovery)
#define EFT_MOD_ZERO_PREAUTH	0x00000080 // Force acceptance of a zero value (pre-auth only)

#define EFT_MOD_UNATTENDED      0x00000100 // System is an unattended terminal
#define EFT_MOD_E_COMMERCE      0x00000200 // Transaction came from e-Commerce
#define EFT_MOD_NO_CPC_OK       0x00000400 // Set if CPC card and its OK that there's no invoice data
#define EFT_MOD_BET             0x00000800 // Set if txn is a bet.  This allows CNP Electron cards

#define EFT_MOD_ASSUME_SWIPED   0x00001000 // Not allowed unless OFFLINE is also selected.

#define EFT_MOD_REMITTANCE      0x00010000 // for store card accounts: set payment is a remittance

#define EFT_MOD_GIVE_A_XXXX     0x80000000 // Terminal sends "XXXX" instead of real modifiers. Used for offline decline. NB ensure this doesn't conflict with any mods defined in future.

// ACCNT.Flags1 values
#define ACC_CV2_FLG			    0x00000002
#define ACC_AVS_FLG				0x00000100
#define MER_REF_NO_F			0x00000999


//WINTI Results
#define COMPLETED				0
#define REFERRED				2
#define DECLINED				5
#define AUTHORISED				6
#define REVERSED				7
#define COMMS_FAIL				8
#define REFERAUTH				99
#define CARD_DEC_OFFLINE        20

// error results from terminal
#define INVALIDTXN          -2
#define CANCEL_PRESS	    -3
#define UNKNOWN_CARD		-4
#define UNACCEPTABLE_CARD	-5
#define OVER_MAX_REFUND_VAL -6

//pan
#define LAST_DIGITS				4

//emv
#define NON_EMV                 207
#define SALE_WITH_CASHBACK      0x09
#define CASH_ADVANCE			0x01
#define ICC_FORCED_ONLINE       2
#define ICC_REFERRAL			3
#define SERVICE_NOT_ALLOWED		0x00

#define NEXT_DAY				1
#define AFTER_3_DAYS			2
#define AFTER_5_DAYS			3

#define USER_PIN				1
#define REFUND_PIN				2
#define STAT_PIN				3

#define CONFIRM					1
#define REJECT					2

#define YES						1
#define NOT						2

#define PURCH_WITH_CASHBACK		5
#define CASH_ADV				4

#define ALPHA					1
#define TYPE_PIN				2
#define NULLDATA_ENTRY			-5

//menu
#define REPORT					0
#define REPRINT					1
#define SYSTEM					2
#define TILL_ORDER				3
#define CHANGE_PIN				4
#define DOWNLOAD				5
#define SUBMIT_TXNS				6
#define ADD_NEW_ID				7
#define CHANGE_USER_ID			8


#define CLIENT_CONNECTED		367

#define MANUAL_ENTRY            1
#define DHCP                    2

#define MAX_IP_LEN              16
#define MAX_WEP_KEY_LEN         27
#define WORK_OFFLINE			83

#define E_COMMAND_NOT_SUPPORTED (Ushort)0x6985
#define E_EMV_FILE_NOT_FOUND    (Ushort)0x6A82
#define E_EMV_RECORD_NOT_FOUND  (Ushort)0x6A83

//EPP
#define STX						0x02
#define ETX						0x03
#define EOT						0x04
#define ACK						0x06
#define NAK						0x15
#define SUB						0x1A
#define FS						0x1C
#define SI						0x0F
#define SO						0x0E
#define ENQ						0x05
#define EPP_TIMEOUT             -8

#define PP_LEN					0
#define PP_DATA					1

#define EPP_HOST_AUTHORISED		0
#define EPP_HOST_DECLINED		1
#define EPP_FAILED_TO_CONNECT	2
#define EPP_REFERRAL_AUTHORISE  3
#define EPP_REFERRAL_DECLINE	4

#define BAD_TXN_VAL				-40		// User has entered a bad refund / purchase value (integrated)
#define CASHBACK_VAL_BAD_MISS   -41     // Passed cashback value with irrelevant modifier
#define INVALID_ICC_DATA		-58		// Ociu-737 AT fix for Missing/Invalid ICC data
#define EXCEEDS_MAX_REFUND		-78		// User has tried to refund more than this account allows
#define INVALREC2POS			-83		// Invalid record
#define LOGGEDIN2POS			-84		// Received a login record when PED user is already logged in
#define NOTLOGGEDIN2POS			-85		// PED user is not logged in - send a login record
#define SUBMITFAILED2POS		-86		// Submission of offline transactions failed
#define PROBINNW                -87		// Problem in network
#define REFERRAL_TIMEOUT		-88		// Added v56 - if voice referral takes too long
#define INVALID_ACCOUNT_ID		-89		//
#define SRVCNTALWD2POS			-90		// Service not allowed
#define UNACPTBLECRDPOS			-91		// Card not accepted
#define UNKNWNCRD2POS           -92		// unknown card
#define NTIINRNG2POS            -93		// not in IIN range
#define APPBLKED2POS			-94		// Application blocked
#define CRDBLKED2POS			-95		// Card blocked
#define CRDERR2POS				-96		// Card error
#define AUTHERR2POS             -97		// Authorisation error (timeout)
#define TERMCAN2POS             -99		// Transaction cancelled

#define REPRNT_CUSTVOUCH		101
//Options for backlight
#define ON		1
#define OFF		2
#define TIMED	3

#endif
