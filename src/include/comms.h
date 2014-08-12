#ifndef __COMMS_H
#define __COMMS_H

/*
**-----------------------------------------------------------------------------
** PROJECT:         AURIS
**
** FILE NAME:		comms.h
**
** DATE CREATED:    10 July 2007
**
** AUTHOR:			Tareq Hafez
**
** DESCRIPTION:     This file includes module declarations used with comms.c
**-----------------------------------------------------------------------------
*/
#define	false					0
#define	true					1

//
//-----------------------------------------------------------------------------
// Type Definitions
//-----------------------------------------------------------------------------
//
typedef	unsigned char	uchar;
typedef	unsigned int	uint;
typedef	unsigned long	ulong;
typedef	unsigned char	bool;
typedef	unsigned short	ushort;


/*
**-----------------------------------------------------------------------------
** Constants
**-----------------------------------------------------------------------------
*/
#define ERR_COMS_OFFSET					128

#define ERR_COMMS_NONE					0

#define	ERR_COMMS_FUNC_NOT_SUPPORTED	(ERR_COMS_OFFSET + 1)

#define	ERR_COMMS_CONNECT_NOT_SUPPORTED	(ERR_COMS_OFFSET + 20)
#define	ERR_COMMS_CONNECT_FAILURE		(ERR_COMS_OFFSET + 21)
#define	ERR_COMMS_ERROR					(ERR_COMS_OFFSET + 22)

#define ERR_COMMS_SIM					(ERR_COMS_OFFSET + 30)
#define ERR_COMMS_NO_SIGNAL				(ERR_COMS_OFFSET + 31)
#define ERR_COMMS_LOW_SIGNAL			(ERR_COMS_OFFSET + 32)
#define ERR_COMMS_LOW_BATTERY			(ERR_COMS_OFFSET + 33)
#define ERR_COMMS_NO_COVERAGE			(ERR_COMS_OFFSET + 34)
#define ERR_COMMS_NO_PDPCONTEXT			(ERR_COMS_OFFSET + 35)
#define ERR_COMMS_NETWORK_LOST			(ERR_COMS_OFFSET + 36)

#define	ERR_COMMS_RECEIVE_FAILURE		(ERR_COMS_OFFSET + 50)
#define	ERR_COMMS_RECEIVE_TIMEOUT		(ERR_COMS_OFFSET + 51)

#define	ERR_COMMS_ENGAGED_TONE			(ERR_COMS_OFFSET + 60)
#define	ERR_COMMS_NO_ANSWER				(ERR_COMS_OFFSET + 61)
#define	ERR_COMMS_NO_DIAL_TONE			(ERR_COMS_OFFSET + 62)
#define	ERR_COMMS_NO_PHONE_NO			(ERR_COMS_OFFSET + 63)
#define	ERR_COMMS_CARRIER_LOST			(ERR_COMS_OFFSET + 66)

#define	ERR_COMMS_SENDING_ERROR			(ERR_COMS_OFFSET + 71)
#define	ERR_COMMS_GENERAL				(ERR_COMS_OFFSET + 73)

#define	ERR_COMMS_NOSPACE				(ERR_COMS_OFFSET + 74)
#define	ERR_COMMS_ALREADYOPEN			(ERR_COMS_OFFSET + 75)
#define	ERR_COMMS_ALREADYCLOSED			(ERR_COMS_OFFSET + 76)
#define	ERR_COMMS_NOTOWNER				(ERR_COMS_OFFSET + 77)
#define	ERR_COMMS_NOTFOUND				(ERR_COMS_OFFSET + 78)

#define	ERR_COMMS_CTS_LOW				(ERR_COMS_OFFSET + 80)
#define	ERR_COMMS_PORT_NOT_OPEN			(ERR_COMS_OFFSET + 81)
#define	ERR_COMMS_NOT_CONNECTED			(ERR_COMS_OFFSET + 82)
#define	ERR_COMMS_TIMEOUT				(ERR_COMS_OFFSET + 83)
#define	ERR_COMMS_INTERCHAR_TIMEOUT		(ERR_COMS_OFFSET + 84)
#define	ERR_COMMS_NO_LINE				(ERR_COMS_OFFSET + 85)
#define	ERR_COMMS_CANCEL				(ERR_COMS_OFFSET + 86)
#define	ERR_COMMS_NOT_DIALED			(ERR_COMS_OFFSET + 87)
#define	ERR_COMMS_INVALID_PARM			(ERR_COMS_OFFSET + 88)
#define	ERR_COMMS_FEATURE_NOT_SUPPORTED	(ERR_COMS_OFFSET + 89)
#define	ERR_COMMS_PORT_USED				(ERR_COMS_OFFSET + 90)
#define	ERR_COMMS_PPP_PAP_CHAP_FAILED	(ERR_COMS_OFFSET + 91)
#define	ERR_COMMS_PPP_LCP_FAILED		(ERR_COMS_OFFSET + 92)
#define	ERR_COMMS_ETH_IPCP_FAILED		(ERR_COMS_OFFSET + 93)
#define	ERR_COMMS_ETH_CONNECTION_ABORTED (ERR_COMS_OFFSET + 94)
#define	ERR_COMMS_ETH_TCP_SHUTDOWN		(ERR_COMS_OFFSET + 95)
#define	ERR_COMMS_ETH_NOT_CONNECTED		(ERR_COMS_OFFSET + 96)
#define	ERR_COMMS_ETH_CMD_SEND			(ERR_COMS_OFFSET + 97)
#define	ERR_COMMS_ETH_NO_RESP			(ERR_COMS_OFFSET + 98)
#define	ERR_COMMS_ETH_INVALID_RESP		(ERR_COMS_OFFSET + 99)
#define	ERR_COMMS_ETH_NO_IPADDR			(ERR_COMS_OFFSET + 100)

#define	ERR_COMMS_SSL_SELF_SIGNED		(ERR_COMS_OFFSET + 105)
#define	ERR_COMMS_SSL_NO_ROOT_CA		(ERR_COMS_OFFSET + 106)
#define	ERR_COMMS_SSL_NO_ISSUER_CA		(ERR_COMS_OFFSET + 107)
#define	ERR_COMMS_SSL_UNTRUSTED_SGL_CERT (ERR_COMS_OFFSET + 108)
#define	ERR_COMMS_SSL_INVALID_CA		(ERR_COMS_OFFSET + 109)
#define	ERR_COMMS_SSL_WRONG_PURPOSE_CA	(ERR_COMS_OFFSET + 110)
#define	ERR_COMMS_SSL_CERT_REJECTED		(ERR_COMS_OFFSET + 111)
#define	ERR_COMMS_SSL_UNTRUSTED_CERT	(ERR_COMS_OFFSET + 112)
#define	ERR_COMMS_SSL_WEAK_KEY			(ERR_COMS_OFFSET + 113)
#define	ERR_COMMS_SSL_DECODING_PUBLIC_KEY (ERR_COMS_OFFSET + 114)
#define	ERR_COMMS_SSL_SIGNATURE_FAILURE	(ERR_COMS_OFFSET + 115)
#define	ERR_COMMS_SSL_CERT_NOT_YET_VALID (ERR_COMS_OFFSET + 116)
#define	ERR_COMMS_SSL_CERT_EXPIRED		(ERR_COMS_OFFSET + 117)
#define	ERR_COMMS_SSL_GENERAL			(ERR_COMS_OFFSET + 118)

//
// This is more portable and backward compatible with Java applications. A mapping is required in the communication module
//
#define COMMS_300					1
#define COMMS_600					2
#define COMMS_1200					3
#define	COMMS_2400					4
#define	COMMS_4800					5
#define COMMS_9600					6
#define COMMS_12000					7
#define COMMS_14400					8
#define COMMS_19200					9
#define COMMS_28800					10
#define COMMS_38400					11
#define COMMS_57600					12
#define COMMS_115200				13
#define	COMMS_BAUDRATE_MAX_COUNT	13

#define FUNC_COMMS					1
#define	FUNC_COMMS_ERROR_DESC		2


/*
**-----------------------------------------------------------------------------
** Type definitions
**-----------------------------------------------------------------------------
*/
typedef enum
{
	E_COMMS_FUNC_CONNECT,
	E_COMMS_FUNC_SEND,
	E_COMMS_FUNC_RECEIVE,
	E_COMMS_FUNC_DISCONNECT,
	E_COMMS_FUNC_SERIAL_DATA_AVAILABLE,
	E_COMMS_FUNC_SERIAL2_DATA_AVAILABLE,
	E_COMMS_FUNC_DATA_AVAILABLE,
	E_COMMS_FUNC_SYNC_SWITCH,
	E_COMMS_FUNC_SET_SERIAL,
	E_COMMS_FUNC_PSTN_WAIT,
	E_COMMS_FUNC_DISP_GPRS_STS,
	E_COMMS_FUNC_SIGNAL_STRENGTH,
	E_COMMS_FUNC_PING
} E_COMMS_FUNC;

typedef enum
{
	E_CONNECTION_TYPE_UART_1,
	E_CONNECTION_TYPE_UART_2,
	E_CONNECTION_TYPE_PSTN,
	E_CONNECTION_TYPE_IP,
	E_CONNECTION_TYPE_IP_SETUP,
	E_CONNECTION_TYPE_LAST
} E_CONNECTION_TYPE;

typedef enum
{
	E_HEADER_NONE = 0,
	E_HEADER_LENGTH,
	E_HEADER_HTTP,

	E_HEADER_SSL,
	E_HEADER_SSL_LENGTH,
	E_HEADER_HTTPS,
	E_HEADER_CLNP,
	E_HEADER_TPDU_20,
	E_HEADER_TPDU_30,
	E_HEADER_TPDU_WESTPAC,
	E_HEADER_CBA_LENGTH,
	E_HEADER_FLAG_LENGTH,

	E_HEADER_MAX
} E_HEADER;

typedef struct
{
	uint wHandle;
	E_CONNECTION_TYPE eConnectionType;
	bool fBlindDial;
	char ptDialPrefix[50];
	char ptPhoneNumber[50];
	bool fSync;
	bool fFastConnect;
	char tDialType;
	uint bBaudRate;
	uint bDataBits;
	uint bStopBits;
	char tParityBit;

	char ownIpAddress[50];
	char gateway[50];
	char pdns[50];
	char sdns[50];
	char ipAddress[50];
	uint wPortNumber;

	E_HEADER eHeader;
	char * ptCLNPSource;
	char * ptCLNPDest;
	char * ptTPDUHeader;

	uint bConnectionTimeout;
	uint bResponseTimeout;
	ulong dwInterCharTimeout;

	bool fPreDial;

	// This data changes per message
	uchar * pbData;
	uint wLength;
} T_COMMS;

/*
**-----------------------------------------------------------------------------
** Module Declarations
**-----------------------------------------------------------------------------
*/
uint CommsConnect(T_COMMS * psComms);
uint CommsSend( T_COMMS * psComms );
uint CommsReceive( T_COMMS * psComms, bool fFirstChar );
uint CommsDisconnect(T_COMMS * psComms );

void CommsReInitPSTN(void);

/*
** Only export (register) the following applet functions
*/
uint Comms(E_COMMS_FUNC eFunc, T_COMMS * psComms);
char * CommsErrorDesc(uint wError);

#endif /* __COMMS_H */
