#ifndef __INPUTMACRO_H
#define __INPUTMACRO_H

/*
**-----------------------------------------------------------------------------
** Constant Definitions.
**-----------------------------------------------------------------------------
*/

/*
* Key Definitions
*/

typedef unsigned long T_KEYBITMAP;    /* Key bitmap type */
typedef unsigned long T_EVTBITMAP;    /* Event bitmap type */
#define KEY_NONE		0xFF
#define	KEY_NUM			0xFE
#define	KEY_ALL			0xFD
#define	KEY_NO_PIN		0xFC

#define KEY_0			'0'
#define KEY_1			'1'
#define KEY_2			'2'
#define KEY_3			'3'
#define KEY_4			'4'
#define KEY_5			'5'
#define KEY_6			'6'
#define KEY_7			'7'
#define KEY_8			'8'
#define KEY_9			'9'

#define	KEY_ASTERISK	0x2A
#define KEY_FUNC		0x23
#define KEY_CNCL		0x1B

#define KEY_CLR			0x08
#define	KEY_LCLR		0x0E
#define KEY_OK			0x0D

#define	KEY_F0			0x6E
#define KEY_F1			0x7A
#define	KEY_F2			0x7B
#define	KEY_F3			0x7C
#define	KEY_F4			0x7D
#define	KEY_F5			0x6F

#define	KEY_ALPHA		0x0F

#define KEY_SK1			0x61
#define KEY_SK2			0x62
#define KEY_SK3			0x63
#define KEY_SK4			0x64

/*
** Bit position definitions for bitmaps.
*/
#define BIT0		0x00000001L
#define BIT1		0x00000002L
#define BIT2		0x00000004L
#define BIT3		0x00000008L
#define BIT4		0x00000010L
#define BIT5		0x00000020L
#define BIT6		0x00000040L
#define BIT7		0x00000080L
#define BIT8		0x00000100L
#define BIT9		0x00000200L
#define BIT10		0x00000400L
#define BIT11		0x00000800L
#define BIT12		0x00001000L
#define BIT13		0x00002000L
#define BIT14		0x00004000L
#define BIT15		0x00008000L
#define BIT16		0x00010000L
#define BIT17		0x00020000L
#define BIT18		0x00040000L
#define BIT19		0x00080000L
#define BIT20		0x00100000L
#define BIT21		0x00200000L
#define BIT22		0x00400000L
#define BIT23		0x00800000L
#define BIT24		0x01000000L
#define BIT25		0x02000000L
#define BIT26		0x04000000L
#define BIT27		0x08000000L
#define BIT28		0x10000000L
#define BIT29		0x20000000L
#define BIT30		0x40000000L
#define BIT31		0x80000000L

/*
* The following define valid key bits
*/
#define KEY_0_BIT		(T_KEYBITMAP) BIT0
#define KEY_1_BIT		(T_KEYBITMAP) BIT1
#define KEY_2_BIT		(T_KEYBITMAP) BIT2
#define KEY_3_BIT		(T_KEYBITMAP) BIT3
#define KEY_4_BIT		(T_KEYBITMAP) BIT4
#define KEY_5_BIT		(T_KEYBITMAP) BIT5
#define KEY_6_BIT		(T_KEYBITMAP) BIT6
#define KEY_7_BIT		(T_KEYBITMAP) BIT7
#define KEY_8_BIT		(T_KEYBITMAP) BIT8
#define KEY_9_BIT		(T_KEYBITMAP) BIT9

#define KEY_FUNC_BIT	(T_KEYBITMAP) BIT10
#define KEY_CNCL_BIT	(T_KEYBITMAP) BIT11

#define KEY_CLR_BIT		(T_KEYBITMAP) BIT12
#define KEY_OK_BIT		(T_KEYBITMAP) BIT13

#define KEY_ALPHA_BIT	(T_KEYBITMAP) BIT14

#define KEY_SK1_BIT		(T_KEYBITMAP) BIT15
#define KEY_SK2_BIT		(T_KEYBITMAP) BIT16
#define KEY_SK3_BIT		(T_KEYBITMAP) BIT17
#define KEY_SK4_BIT		(T_KEYBITMAP) BIT18

#define KEY_F0_BIT		(T_KEYBITMAP) BIT19
#define KEY_F1_BIT		(T_KEYBITMAP) BIT20
#define KEY_F2_BIT		(T_KEYBITMAP) BIT21
#define KEY_F3_BIT		(T_KEYBITMAP) BIT22
#define KEY_F4_BIT		(T_KEYBITMAP) BIT23
#define KEY_F5_BIT		(T_KEYBITMAP) BIT24

#define KEY_ASTERISK_BIT	(T_KEYBITMAP) BIT25
#define KEY_LCLR_BIT	(T_KEYBITMAP) BIT26

#define KEY_NO_PIN_BIT	(T_KEYBITMAP) BIT27

/*
* Valuable combinations of key bits
*/
#define	KEY_NO_BITS		(T_KEYBITMAP) 0


#define KEY_NUM_BITS    (KEY_0_BIT | \
                         KEY_1_BIT | \
                         KEY_2_BIT | \
                         KEY_3_BIT | \
                         KEY_4_BIT | \
                         KEY_5_BIT | \
                         KEY_6_BIT | \
                         KEY_7_BIT | \
                         KEY_8_BIT | \
                         KEY_9_BIT)

#define KEY_SK_BITS	(KEY_SK1_BIT | KEY_SK2_BIT | KEY_SK3_BIT | KEY_SK4_BIT)

#define KEY_SCROLL_BITS	KEY_ALPHA_BIT

#define KEY_CTRL_BITS	(KEY_SK_BITS	| \
                         KEY_FUNC_BIT	| \
                         KEY_OK_BIT	| \
                         KEY_CLR_BIT	| \
                         KEY_CNCL_BIT	| \
                         KEY_NO_PIN_BIT	| \
                         KEY_SCROLL_BITS)

#define KEY_ALL_BITS     (KEY_NUM_BITS | KEY_CTRL_BITS)

/*
** Event Bitmap Constants
*/
#define EVT_NONE				(T_EVTBITMAP) 0
#define EVT_TIMEOUT				(T_EVTBITMAP) BIT0
#define EVT_SCT_IN				(T_EVTBITMAP) BIT1
#define EVT_SCT_OUT				(T_EVTBITMAP) BIT2
#define EVT_MCR					(T_EVTBITMAP) BIT3
#define	EVT_IP_CONNECTION		(T_EVTBITMAP) BIT4
#define	EVT_IP_DATA				(T_EVTBITMAP) BIT5
#define	EVT_SERIAL_CONNECTION	(T_EVTBITMAP) BIT6
#define	EVT_SERIAL_DATA			(T_EVTBITMAP) BIT7
#define	EVT_SERIAL2_DATA		(T_EVTBITMAP) BIT8
#define	EVT_MODEM_CONNECTION	(T_EVTBITMAP) BIT9
#define	EVT_MODEM_DATA			(T_EVTBITMAP) BIT10
#define	EVT_INIT				(T_EVTBITMAP) BIT11
#define	EVT_INIT2				(T_EVTBITMAP) BIT12
#define	EVT_LAST				(T_EVTBITMAP) BIT13
#define	EVT_LAST2				(T_EVTBITMAP) BIT14
#define	EVT_INIT0				(T_EVTBITMAP) BIT15
#define	EVT_CTLS_CARD			(T_EVTBITMAP) BIT16
#define	EVT_TOUCH				(T_EVTBITMAP) BIT17

#endif
