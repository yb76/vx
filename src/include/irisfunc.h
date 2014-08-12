#ifndef __IRISFUNC_H
#define __IRISFUNC_H

/*
**-----------------------------------------------------------------------------
** PROJECT:         AURIS
**
** FILE NAME:       irisfunc.h
**
** DATE CREATED:    
**
** AUTHOR:			
**
** DESCRIPTION:    
**-----------------------------------------------------------------------------
*/

/*
**-----------------------------------------------------------------------------
** Constants
**-----------------------------------------------------------------------------
*/

/*
**-----------------------------------------------------------------------------
** Type definitions
**-----------------------------------------------------------------------------
*/


/*
**-----------------------------------------------------------------------------
** Functions
**-----------------------------------------------------------------------------
*/

#include "auris.h"
#include "as2805.h"

#define LOG_APPFILTER				0x00000001L

/*as2805.c*/
void AS2805SetBit(uchar field);
uchar *AS2805Init(uint size);
void AS2805Close(void);
uchar *AS2805Position(uint * length);
void AS2805BcdLength(bool state);
void AS2805BufferPack(char * data, uchar format, uint size, uchar * buffer, uint * index);
uint AS2805Pack(uchar field, char * data);
void AS2805BufferUnpack(char * data, uchar format, uint size, uchar * buffer, uint * index);
void AS2805Unpack(uchar field, char * data, uchar * buffer, uint length);
void AS2805OFBAdjust(uchar * source, uchar * dest, uint length);
void AS2805OFBVariation(int my_maxField, int my_addField);

/* display.c */
void DispInit(void);
void DispClearScreen(void);
void DispText(const char * text, uint row, uint col, bool clearLine, bool largeFont, bool inverse);
void DispGraphics(const char * graphics, uint row, uint col);
int DispArray(int timeout,char **pcMenuItems, int iMenuItemsTotal);
void DispSignal(uint row, uint col);
void DispUpdateBattery(uint row, uint col);
char DebugDisp (const char *template, ...);

/* emv.c */
int EmvGetPTermID(char *ptid);
int EMVPowerOn(void);
int EMVTransInit(void);
int EMVSelectApplication(const long amt, const long acc);
int EMVReadAppData(void);
int EMVDataAuth(void);
int EMVProcessingRestrictions (void);
int EmvCardholderVerify(void);
int EmvProcess1stAC(void);
int EmvUseHostData(int hostDecision,const char *hexdata,short len);
int EmvCardPowerOff(void);
bool EmvIsCardPresent(void);
int EmvGetAmtCallback(void);
int EmvSetAmt(long emvAmount,long emvCash);
int EmvSetAccount(unsigned char emvAcc);

int EmvCallbackFnSelectAppMenu(char **pcMenuItems, int iMenuItemsTotal);
void EmvCallbackFnPromptManager(unsigned short usPromptId);
unsigned short EmvFnAmtEntry(unsigned long *pulTxnAmt);
unsigned short EmvGetUsrPin(unsigned char *ucPin);
int GetSetEmvPin( unsigned short action,unsigned char *pin);
unsigned short EmvSignature(void);
unsigned short EmvFnOnlinePin(void);
int EmvCallbackFnInit(void);
unsigned short EmvFnLastAmtEntry(unsigned long *pulTxnAmt);
int EmvSetTagData(const unsigned char* pcTag, const unsigned char* pbDataBuf, int iDataBufLen);
int EmvGetTagDataRaw( unsigned short pcTag, char *databuf);
int EmvGetTagData(const unsigned char* pcTag, unsigned char* pbDataBuf, int *piTagLength, int iDataBufLen);
unsigned short EmvTag2VxTag(const unsigned char* pcTag);
unsigned short EmvTagLen(const unsigned char* pcTag);
int EmvMiscVerixVEmvErr2RisEmvErr(int iVerixVEmvErr);
unsigned short EmvMiscTagLenLen(const unsigned char* pcTagLenBytes);
unsigned short EmvMiscTagDataLen(const unsigned char* pcTagLenBytes);
void ascii_to_binary(char *dest, const char *src, int length);
short findTag(unsigned short tag, unsigned char *value, short *length, const unsigned char *buffer,short bufLen);
short getNextRawTLVData(unsigned short *tag, unsigned char *data, const unsigned char *buffer);
short getNextTLVObject(unsigned short *tag, short *length, unsigned char *value, const unsigned char *buffer);
//int createScriptFiles(const unsigned char *scriptBuf,short bufLen,char *sc1,short *sc1len, char *sc2, short *sc2len);
int EmvGetTacIac(char * tac_df, char * tac_dn, char *tac_ol, char * iac_df, char *iac_dn, char *iac_ol);


/*input.c*/
void InpSetOverride(bool override);
void InpSetString(char * value, bool hidden, bool override);
char * InpGetString(void);
void InpSetNumber(ulong value, bool override);
ulong InpGetNumber(void);
void InpTurnOn(void);
int InpTurnOff(bool serial0);
//uchar InpGetKeyEvent(T_KEYBITMAP keyBitmap, T_EVTBITMAP * evtBitmap, T_INP_ENTRY inpEntry, ulong timeout, bool largeFont, bool flush, bool * keyPress);
void InpBeep(uchar count, uint onDuration, uint offDuration);
bool InpGetMCRTracks( char * ptTk1, uchar * ulTk1, char * ptTk2, uchar * ulTk2, char * ptTk3, uchar * ulTk3);

/* iris.c */
void IRIS_AppendToUpload(const char * addition);
bool FileExist(const char *filename);
bool FileRemove(const char *filename);
bool FileRename(const char *ofilename,const char *nfilename);
void IRIS_StoreData(const char * objectName, const char *tag, const char * value, bool deleteFlag);
void IRIS_PutNamedObjectData(const char * objectData, uint length, const char * name);
void IRIS_PutObjectData(const char * objectData, uint length);
bool remoteTms(void);
char * IRIS_GetObjectData(const char * objectName, unsigned int * length);
char * IRIS_GetObjectTagValue(const char *data, const char * intag);
void IRIS_DeallocateStringValue(char * value);
char * IRIS_GetStringValue(const char * data, int size, const char * name);

/*iris2805.c */
void __as2805_make(T_AS2805FIELD **flds, char **pmsg);
void __as2805_make_custom(T_AS2805FIELD **flds, char **pmsg);
void __as2805_break(int fldnum,const char *msg,T_AS2805FIELD *flds , int *errfld);
void __as2805_break_custom(const char *msg,T_AS2805FIELD **flds, int *errfld);

/*iris_io.c*/
char * __print_cont(const char * data,bool endflag);
void __print_err(char * result);
void __shutdown(void);
void __lowPower(int event,int tcpcheck, long timeout, long timeout_fail,long timeout_shutdown);

/*iris_cfg.c*/
void __sec_init(void);
void __ppid(char* ppid);
void __ppid_update(const char *ppid);
void __ppid_remove(void);
void __serial_no(char* sn);
char* __manufacturer(char* mf);
void __model(char *model_s);

/*iriscomms.c*/
//void IRIS_CommsSend(char *data,T_COMMS * comms, int * retVal,char*retMsg);
//void IRIS_CommsRecv(T_COMMS * comms, char* interCharTimeout, char* timeout, int bufLen, int * retVal,char** pstring,char* retMsg);
//void IRIS_CommsDisconnect(T_COMMS * comms, int retVal);

/*iriscrypt.c*/
bool __des_store(const char *data, const char* keySize, const char* key);
bool __3des_rsa_store(char *data, int rsakey, int kek);
bool __des_random(const char* keySize, const char *key,char *value);
bool __iv_set(const char* iv);
char* __crypt(const char* data, const char *keySize, const char* key, const char *variant, bool decrypt, bool ofb);
char* __mac(const char* data, const char* variant, const char *key , int length, bool dataIsHex, char * mac_value);
void __kvc(const char* keySize, const char * key, char *kvc_);
bool __owf(const char* ppasn, const char* to, const char* from, bool variant, const char * data );
bool __derive_3deskey(const char* data, const char* variant, const char* key, const char* kek);
bool __rsa_store(const char *data, const char* rsa);
bool __rsa_clear(const char *rsa);
char * __rsa_crypt(const char *data, const char *rsa);
bool __rsa_wrap_3des(const char *rsa_key, const char *des_key,char * result);
void __dformat1(char *data, int int_blockSize, char *result);
bool __key_xor( const char* to, const char* from, const char* with_key, const char * with_data );
bool __get_key(int key,char *value);

/*irisemv.c*/
bool _emv_pack_tlv(const char* tag,char* results);
/* emvmisc.c */
bool emv_tlv_replace( char *newtlvs,short* bufLen,unsigned short tag);

/*irismain.c*/
//int DisplayObject(char *lines,T_KEYBITMAP keyBitmap,T_EVTBITMAP keepEvtBitmap,int timeout, char* outevent, char* outinput);
int set_nextObj( const char* nextObj);
char * currentScreen(int getset, char* scrname);
int inGetApplVer(char *ver);

/*irispstn.c*/
void __pstn_init(void);
char* __pstn_connect(const char *bufSize,const char *baud,const char *interCharTimeout,const char *timeout,const char *phoneNo,const char *pabx,const char *fastConnect,const char *blindDial,const char *dialType,const char *sync,const char *preDial,const char* header);
void __pstn_send(const char* data,char*retMsg);
void __pstn_recv(char **pmsg,const char* interCharTimeout, const char *timeout, char * errmsg);
void __pstn_disconnect(void);
void __pstn_err(void);
void __pstn_clr_err(void);
char * __pstn_wait(void);

/*irisser.c*/
void __ser_init(void);
int __ser_connect(const char *header, const char *port,const char* timeout,const char* interCharTimeout,const  char* baud, const char* dataBits, const char* parity, const char* stopBits, const char* bufSize);
int __ser_reconnect(int myPort);
int __ser_send(const char *port,const char* data);
char* __ser_recv(const char *port);
bool __ser_data(const char* port);
int ____ser_disconnect(int myPort);

/*iristcp.c*/
void __ip_connect(const char* timeout, const char* sdns, const char *pdns, const char *gw, const char * oip, char **retmsg);
void __tcp_connect(const char * bufSize, const char * interCharTimeout, const char * timeout, const char * port, const char * hip, const char *sdns, const char * pdns, const char *gw, const char* oip , const char * header, char **perrmsg);
void __tcp_recv(char **pmsg,const char* interCharTimeout, const char *timeout, char * errmsg);
void __tcp_send(const char* data,char*retMsg);
void __tcp_disconnect(void);
void __tcp_disconnect_now(void);
void __tcp_disconnect_do(void);
int __tcp_disconnect_ip_only(void);
int __tcp_disconnect_completely(void);
int __tcp_disconnect_check(void);
int __tcp_disconnect_extend(void);
void __tcp_gprs_sts(void);

/*iristime.c*/
void __now(char *result);
void __time_set(const char *newTime,char *adjust);
void ____time(char * value, int * which, char * output, int * j);
char* __time_real(const char* format,char *now);
void __timer_start(int tm);
bool __timer_started(int tm);
long __timer_stop(int tm);

/*irisutil.c*/
void __text_table(const char* search, const char* textTableObjectName, char** pfound);
void __store_objects(int unzip,char *objects, int*nextpart,char **response);
void __upload_msg(const char *message);
void __upload_obj(const char *objectName);
bool __remote(void);
void __vmac_app(void);
void __battery_status(void);
void __dock_status(void);
char* get_csv_str( char * line, char sep, int idx , char *result);
bool __luhn(const char *pan);

/*malloc.c/calloc.c*/
void * my_malloc(unsigned int size);
void my_free(void * block);
void * my_realloc(void * ptr, unsigned int size);
void * my_calloc(unsigned int size);

/*security.c*/
void SecurityInit(void);
bool SecurityWriteKey(char * appName, uchar location, uchar keySize, uchar * key);
bool SecurityEraseKey(char * appName, uchar location, uchar keySize);
bool SecurityGenerateKey(char * appName, uchar location, uchar keySize);
bool SecurityKVCKey(char * appName, uchar location, uchar keySize, uchar * kvc);
bool SecurityCopyKey(char * appName, uchar location, uchar keySize, uchar to);
bool SecurityMoveKey(char * appName, uchar location, uchar keySize, uchar to);
bool SecurityXorKey(char * appName, uchar location, uchar keySize, uchar with, uchar to);
bool SecurityXorKeyWithData(char * appName, uchar location, uchar keySize, uchar *xdata, uchar to);
bool SecuritySetIV(uchar * iv);
bool SecurityCrypt(char * appName, uchar location, uchar keySize, int eDataSize, uchar * eData, bool decrypt, bool ofb);
int _getVariant(uchar * data, uchar keySize, uchar * variant);
bool SecurityCryptWithVariant(char * appName, uchar location, uchar keySize, int eDataSize, uchar * eData, uchar * variant, bool decrypt);
bool SecurityMAB(char * appName, uchar location, uchar keySize, int eDataSize, uchar * eData, uchar * variant, uchar * mab);
bool SecurityInjectKey(char * appName, uchar location, uchar keySize, char * appName2, uchar to, uchar eKeySize, uchar * eKey, uchar * variant);
bool SecurityOWFKey(char * appName, uchar location, uchar keySize, uchar to, uchar ppasn, bool variant);
bool SecurityOWFKeyWithData(char * appName, uchar location, uchar keySize, uchar to, uchar * variant);
bool Security3DESWriteRSA(char * appName, uchar location, char * appName2, uchar to, int blockLen, uchar * block);
bool SecurityWriteRSA(char * appName, uchar location, int blockLen, uchar * block);
bool SecurityClearRSA(char * appName, uchar location);
bool SecurityCryptRSA(char * appName, uchar location, int eDataSize, uchar * eData);
bool SecurityRSAInjectKey(char * appName, uchar location, char * appName2, uchar to, uchar eKeySize, int eDataSize, uchar * eData);
bool SecurityRSAInjectRSA(char * appName, uchar location, char * appName2, uchar to, int eDataSize, uchar * eData);
bool SecurityRSAWrap3Des(char * appName, uchar location, uchar to, int *eDataSize, uchar * eData);
bool SecurityPINBlock(char * appName, uchar location, uchar keySize, char * pan, uchar * ePinBlock);
bool SecurityPINBlockWithVariant(char * appName, uchar location, uchar keySize, char * pan, char * stan, char * amount, bool emv,uchar * ePinBlock);
bool SecurityGetKey(char * appName, uchar location, uchar* value);
bool SecurityPINBlockCba(uchar* location_1, uchar *location_2,char * pan, bool emv,uchar * ePinBlock);

/*util.c */
void UtilBinToBCD(uchar data, uchar * bcd, uchar length);
char * UtilHexToString(const char * hex, int length, char * string);
char * UtilStrDup(char ** dest, const char * source);
int UtilStringToHex(const char * string, int length, uchar * hex);
long UtilStringToNumber(const char * string);

int lua_main(char *file);
#endif /* __IRISFUNC_H */
