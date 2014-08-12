/*****************************************************************************/
/* This source code is confidential proprietary information of VeriFone Inc. */
/* and is an unpublished work or authorship protected by the copyright laws  */
/* of the United States. Unauthorized copying or use of this document or the */
/* program contained herein, in original or modified form, is a violation of */
/* Federal and State law.                                                    */
/*                                                                           */
/*                           Verifone                                        */
/*                        Copyright 2009                                     */
/*****************************************************************************/

/*
;|===========================================================================+
;| FILE NAME:  | LIBCTLS.H
;|-------------+-------------------------------------------------------------+
;| DEVICE:     | Verix OS11 Contactless Application Interface
;|-------------+-------------------------------------------------------------+
;| DESCRIPTION:|
;|-------------+-------------------------------------------------------------+
;| TYPE:       |
;|-------------+-------------------------------------------------------------+
;| COMMENTS:   |
;|===========================================================================+
*/

/*
$Archive:   //vfih3/FileServer/Engg/archives/VxT_CTLSLIB/LibCtls.h-arc  $
$Author:   Jeff_S9  $
$Date:   04 Dec 2009 11:10:56  $
$Modtime:   03 Dec 2009 12:20:42  $
$Revision:   1.7  $
$Log:   //vfih3/FileServer/Engg/archives/VxT_CTLSLIB/LibCtls.h-arc  $
 * 
 *    Rev 1.7   04 Dec 2009 11:10:56   Jeff_S9
 * Added Verix CTLS driver Get version ID.
 * Updated calls to support Driver buffering application data.
 *
 *    Rev 1.6   11 Nov 2009 12:19:52   Jeff_S9
 * Changed call parameters for PICC_PassThrow
 *
 *    Rev 1.5   02 Nov 2009 13:53:04   Jeff_S9
 * Changed to version 1.05
 * Removed direct access to HW paramters. Accessed through PICC_L1_Analog_Test()
 *
 *    Rev 1.4   29 Oct 2009 17:20:04   Jeff_S9
 * Add radio parameter modifying function to API
 *
 *    Rev 1.3   22 Oct 2009 14:47:26   Jeff_S9
 * Fixed return parameters and CTLS command data structure types
 *
 *    Rev 1.2   21 Aug 2009 07:48:04   Jeff_S9
 * Added Poll for card present functions
 *
 *    Rev 1.1   14 Aug 2009 09:30:06   Jeff_S9
 * PICC_Init changed to void return
 * Changed library version to 1.01
 *
 *    Rev 1.0   30 Jul 2009 17:24:16   Jeff_S9
 * Initial revision.
*/


#ifndef usint
typedef unsigned short usint;
#endif
#ifndef sint
typedef short sint;
#endif

#ifndef ulint
typedef unsigned int ulint;
#endif

#ifndef byte
typedef unsigned char   byte;
#endif

#ifndef ubyte
typedef unsigned char ubyte;
#endif

#ifndef boolean
typedef byte boolean;
#endif


typedef struct {
  char  *DataFromCard;  /*   response data, no NULL!!!!!                                 */
  char  *DataToCard;    /*   (in a command)                                              */
  unsigned short Le;      /*   (in a command) Max length of DataFromCard (length expected) */
  unsigned short Lr;      /*   (Nowhere) Actual length of DataFromCard                     */
  unsigned short SW1_SW2; /*   (in a response)                                             */
  char  Lc;             /*   (in a command) Length of DataToCard  (length of command)    */
  char  Class;          /*   (in a command: xxxxxx--)                                    */
  char  Instruct;       /*   (in a command)                                              */
  char  P1;             /*   (in a command)                                              */
  char  P2;             /*   (in a command)                                              */
  char  card_type;      /*   ISO14443A_CARD_TYPE or ISO14443B_CARD_TYPE                  */
} PICC_IccInstructStruct;


/** API for Level 1 CTLS Driver **/
unsigned char PICC_IsCardPresent(void);
int PICC_PollPresent(int on_off);               // on_off param - !0=on  0=off
int PICC_PollStatus(void);                      // return - !0=polling  0=not polling
int PICC_Command(PICC_IccInstructStruct* cmd);
void PICC_CardRemoval(void);
void PICC_Reset(void);
void PICC_GetVersion (char *version);
char PICC_GetCardType(void);
int PICC_L1_Analog_Test(char mode, char *buf);
void PICC_LED (int ledMap);
void PICC_Test(void);
unsigned long PICC_LibVersion(void);            /* 0xMMmmrrrr  M=Major Version; m=minor; r=reserved */
void CTLS_GetDriverVersion( char *version );    /* ASCII Build ID for Verix CTLS Driver */

/** Functions for PICCTest_Level1 **/
void  PICC_Init(void);
void PICC_FieldOn(void);
void PICC_FieldOff(void);
void PICC_ResetField(void);
void Cpuhw_IOCommands(char cmd, char value);
void NXP512_ResetField(void);
int  ISO14443A_WUPA(void);
int  ISO14443B_WUPB(void);
int  ISO14443A_AntiCollision(void );
void ISO14443A_GetParams(void *param);
int  ISO14443A_RATS(void);
int  ISO14443B_ATTRIB(void);

/** PICC L1 library extensions **/
int PiccEx( void *ptr, int type, void *rtnData );
char PiccEx_PassThrow( byte Command, void* InData, usint *InDataLen,
                                   void* OutData, usint *OutDataLen);

