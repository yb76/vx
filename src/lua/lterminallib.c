
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define lua_c

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include <svc.h>
#include "auris.h"
#include "as2805.h"
#include "security.h"
#include "irisfunc.h"
#include "utility.h"
#include "iris.h"
#include "alloc.h"
#include "my_time.h"
#ifdef __EMV
	#include "emv.h"
#endif

static int terminal_SetDisplayFlush (lua_State *L) {
  bool flag = lua_toboolean(L, 1);  /*  get argument */
  GetSetFlush(1,flag);
  return 0;  /*  number of results */
}

static int terminal_DisplayObject (lua_State *L) {
  const char* lines ;
  unsigned long keybmp ;
  unsigned long evtbmp ;
  int timeout ;
  char event[64]="";
  char input[256]="";

  lines = lua_tostring(L, 1);  /*  get argument */
  keybmp = (unsigned long)lua_tonumber(L, 2);  /*  get argument */
  evtbmp= (unsigned long)lua_tonumber(L, 3);  /*  get argument */
  timeout = (int)lua_tonumber(L, 4);  /*  get argument */


  DisplayObject( lines,keybmp,evtbmp,timeout,event,input);
  GetSetFlush(1,true);

  lua_pushstring(L, event);  /*  push result */
  lua_pushstring(L, input);  /*  push result */
  return 2;  /*  number of results */

}

static int terminal_DebugDisp (lua_State *L) {
  const char* lines ;
  lines = lua_tostring(L, 1);  /*  get argument */
  DebugDisp(lines);
  return 1;  /*  number of results */
}

static int terminal_SetNextObject (lua_State *L) {
  const char* nextobj ;
  nextobj = lua_tostring(L, 1);  /*  get argument */
  set_nextObj(nextobj);
  return 1;  /*  number of results */
}

static int terminal_SetScreenName (lua_State *L) {
  const char* scrName ;
  scrName = lua_tostring(L, 1);  /*  get argument */
//  currentScreen(1,(char*) scrName);
  return 1;  /*  number of results */
}

static int terminal_TextTable (lua_State *L) {
  const char* tableobj =lua_tostring(L,1) ; 
  const char* search = lua_tostring(L,2) ; 
  char *result=NULL;
  __text_table( search,tableobj,&result);
  lua_pushstring(L, result);  /*  push result */
  UtilStrDup(&result,NULL);

  return 1;  /*  number of results */
}

static int terminal_SysTicks (lua_State *L) {
  unsigned long ticks = systicks();
  lua_pushnumber(L, ticks);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_Now (lua_State *L) {
  char result[50];
  sprintf(result, "%lu", my_time(NULL));
  lua_pushstring(L, result);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_Time (lua_State *L) {
  char result[50];
  const char* format = lua_tostring(L,1) ; 

  __time_real(format,result);
  lua_pushstring(L, result);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_TimeSet (lua_State *L) {
  const char* currtime =lua_tostring(L,1) ; 
  const char* timeadjust =lua_tostring(L,2) ; 
  __time_set(currtime,(char *)timeadjust);
  return 0;  /*  number of results */
}

static int terminal_GetJsonValueInt (lua_State *L) {
  const char *jsonobj = lua_tostring(L,1);
  char *jsontag = NULL;
  unsigned int objlength;
  char *jsonvalue=NULL;
  int rtnnumber = 0;
  int top = lua_gettop(L);
  short i;

  char *data = (char*)IRIS_GetObjectData( jsonobj, &objlength);

  lua_checkstack(L,2);
  for(i=2;i<=top;i++) {
      jsontag = (char*) lua_tostring(L,i);
      if(jsontag == NULL) break;

      if(data) {
        jsonvalue = (char*)IRIS_GetObjectTagValue( data, jsontag );
        if(jsonvalue) {
	      lua_pushinteger(L, atol(jsonvalue));  /*  push result */
          UtilStrDup(&jsonvalue , NULL);
	    } else lua_pushinteger(L,0);
	  } else lua_pushinteger(L,0);
	  ++rtnnumber ;
  }
  if(data)  UtilStrDup(&data , NULL);

  return rtnnumber;  /*  number of results */
}

static int terminal_GetJsonValue (lua_State *L) {
  const char *jsonobj = lua_tostring(L,1);
  char *jsontag = NULL;
  unsigned int objlength;
  char *jsonvalue=NULL;
  int rtnnumber = 0;
  int top = lua_gettop(L);
  short i;

  char *data = (char*)IRIS_GetObjectData( jsonobj, &objlength);

  lua_checkstack(L,2);
  for(i=2;i<=top;i++) {
      jsontag = (char*) lua_tostring(L,i);
      if(jsontag == NULL) break;

      if(data) {
        jsonvalue = (char*)IRIS_GetObjectTagValue( (const char *)data, jsontag );
        if(jsonvalue) {
	      lua_pushstring(L, jsonvalue);  /*  push result */
          UtilStrDup(&jsonvalue , NULL);
	    } else lua_pushstring(L,"");
	  } else lua_pushstring(L,"");
	  ++rtnnumber ;
  }
  if(data)  UtilStrDup(&data , NULL);

  return rtnnumber;  /*  number of results */
}

static int terminal_GetBattery (lua_State *L) {
  int charging= 0;
  int batt= 0;
  GetBatteryRemaining(&charging,&batt);
  lua_pushinteger(L, charging);  /*  push result */
  lua_pushinteger(L, batt);  /*  push result */
  return 2;  /*  number of results */
}

static int terminal_GetDirInfo (lua_State *L) {
  const char *driver = lua_tostring(L,1);
  const char *filename = lua_tostring(L,2);
  char *filedata = NULL;

  int iret = 0;

  iret = IRIS_GetDirInfo( driver, "", filename, &filedata);
  if(iret==0 && filedata) {
	lua_pushstring(L,filedata);
  } else {
	lua_pushstring(L,"");
  }

  if(filedata) UtilStrDup(&filedata , NULL);

  return 1;  /*  number of results */
}

static int terminal_GetFileInfo (lua_State *L) {
  const char *jsonobj = lua_tostring(L,1);
  const char *gettype = lua_tostring(L,2);
  char filedate[30]="";
  int filesize = 0;
  char *filedata = NULL;
  int iret = 0;

  iret = IRIS_GetFileInfo(jsonobj, gettype, &filesize, filedate, &filedata);
  if(iret==0) {
	lua_pushinteger(L, filesize);  /*  push result */
	lua_pushstring(L, filedate);  /*  push result */
	lua_pushstring(L, filedata);  /*  push result */
  }
  else {
	lua_pushinteger(L,0);
	lua_pushstring(L,"");
	lua_pushstring(L,"");
  }

  if(filedata) UtilStrDup(&filedata , NULL);

  return 3;  /*  number of results */
}

static int terminal_GetJsonObject (lua_State *L) {
  const char *jsonobj = lua_tostring(L,1);
  unsigned int objlength;

  char *data = (char*)IRIS_GetObjectData( jsonobj, &objlength);
  if(data) {
	 lua_pushstring(L, data);  /*  push result */
     UtilStrDup(&data , NULL);
  }
  else lua_pushstring(L,"");


  return 1;  /*  number of results */
}

static int terminal_SetJsonValue (lua_State *L) {
  const char *jsonobj = lua_tostring(L,1);
  const char *jsontag = lua_tostring(L,2);
  const char *jsonvalue = lua_tostring(L,3);

  IRIS_StoreData(jsonobj,jsontag, jsonvalue, 0/* false */);

  return 0;  /*  number of results */
}

static int terminal_GetTrack (lua_State *L) {
  int trackind = (int) lua_tonumber(L,1);
  char trackdata[100]="";
  unsigned char trackLen;
  if( trackind ==1 ) { trackLen = 80; InpGetMCRTracks(trackdata, &trackLen, NULL, NULL, NULL, NULL);}
  if( trackind ==2 ) { trackLen = 40; InpGetMCRTracks(NULL,NULL,trackdata, &trackLen, NULL, NULL); }
  //if( trackind ==3 ) InpGetMCRTracks(NULL,NULL,NULL,NULL,trackdata, &trackLen);
  lua_pushstring(L, trackdata);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_ErrorBeep (lua_State *L) {
  InpBeep(2, 20, 5);
  return 0;  /*  number of results */
}

static int terminal_DoTmsCmd (lua_State *L) {
  	const char *jsonobj = lua_tostring(L,1);
    DoTmsCmd(jsonobj);
    return 0;
}

static int terminal_Sleep (lua_State *L) {
  int sleeptime = (int) lua_tonumber(L,1);
  SVC_WAIT(sleeptime);
  return 0;  /*  number of results */
}

static int terminal_Model (lua_State *L) {
  char model[13];
  int i ;

  i = sizeof(model) - 1;
  SVC_INFO_MODELNO(model);
  while (model[i-1] == ' ') i--;
  model[i] = '\0';

  lua_pushstring(L, model);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_UploadMsg (lua_State *L) {
  const char* msg = lua_tostring(L,1);
  __upload_msg(msg);
  return 0;  /*  number of results */
}

static int terminal_UploadObj (lua_State *L) {
  const char* objname = lua_tostring(L,1);
  __upload_obj(objname);
  return 0;  /*  number of results */
}

static int terminal_Remote (lua_State *L) {
  bool retval = __remote( );
  lua_pushboolean(L, retval);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_PstnConnect (lua_State *L) {
  const char* bufSize = lua_tostring(L,1);
  const char* baud = lua_tostring(L,2);
  const char* interCharTimeout = lua_tostring(L,3);
  const char* timeout = lua_tostring(L,4);
  const char* phoneNo = lua_tostring(L,5);
  const char* pabx = lua_tostring(L,6);
  const char* fastConnect = lua_tostring(L,7);
  const char* blindDial = lua_tostring(L,8);
  const char* dialType = lua_tostring(L,9);
  const char* sync = lua_tostring(L,10);
  const char* preDial = lua_tostring(L,11);
  const char* header = lua_tostring(L,12);

  char *errmsg = (char*)__pstn_connect(bufSize,baud,interCharTimeout,timeout,phoneNo,pabx,fastConnect,blindDial,dialType,sync,preDial,header);

  lua_pushstring(L, errmsg);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_PstnWait (lua_State *L) {
	char *result = (char *)__pstn_wait();
	lua_pushstring(L,result);
	return 1;
}


static int terminal_TcpConnect (lua_State *L) {
  const char* header = lua_tostring(L,1);
  const char* oip = lua_tostring(L,2);
  const char* gw = lua_tostring(L,3);
  const char* pdns = lua_tostring(L,4);
  const char* dns = lua_tostring(L,5);
  const char* hip = lua_tostring(L,6);
  const char* port = lua_tostring(L,7);
  const char* timeout = lua_tostring(L,8);
  const char* interCharTimeout = lua_tostring(L,9);
  const char* bufSize = lua_tostring(L,10);

  char *errmsg=NULL;
  __tcp_connect( bufSize, interCharTimeout, timeout, port, hip, dns, pdns, gw, oip , header, &errmsg );

  lua_pushstring(L, errmsg?errmsg:"");  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_TcpReconnect (lua_State *L) {
  const char* header = lua_tostring(L,1);
  const char* oip = lua_tostring(L,2);
  const char* gw = lua_tostring(L,3);
  const char* pdns = lua_tostring(L,4);
  const char* dns = lua_tostring(L,5);
  const char* hip = lua_tostring(L,6);
  const char* port = lua_tostring(L,7);
  const char* timeout = lua_tostring(L,8);
  const char* interCharTimeout = lua_tostring(L,9);
  const char* bufSize = lua_tostring(L,10);

  char *errmsg=NULL;
  __tcp_disconnect_do();
  __tcp_connect( bufSize, interCharTimeout, timeout, port, hip, dns, pdns, gw, oip , header, &errmsg );

  lua_pushstring(L, errmsg?errmsg:"");  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_IpConnect (lua_State *L) {
  const char* timeout = lua_tostring(L,1);
  const char* sdns = lua_tostring(L,2);
  const char* pdns = lua_tostring(L,3);
  const char* gw = lua_tostring(L,4);
  const char* oip = lua_tostring(L,5);
  char *errmsg=NULL;
  __ip_connect( timeout, sdns, pdns, gw, oip , &errmsg );
  lua_pushstring(L, errmsg?errmsg:"");  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_TcpSend (lua_State *L) {
  const char* msg = lua_tostring(L,1);
  char errmsg[128];
  __tcp_send( msg,errmsg);

  lua_pushstring(L, errmsg);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_TcpRecv (lua_State *L) {
  const char* interchartimeout = lua_tostring(L,1);
  const char* timeout = lua_tostring(L,2);
  char *rcvmsg = NULL;
  char errmsg[128] = "";
  __tcp_recv(&rcvmsg, interchartimeout, timeout, errmsg);

  lua_pushstring(L, errmsg);  /*  push result */
  if(rcvmsg) lua_pushstring(L, rcvmsg);  /*  push result */
  else lua_pushstring(L,"");
  if(rcvmsg) UtilStrDup(&rcvmsg, NULL);
  return 2;  /*  number of results */
}

static int terminal_PstnSend (lua_State *L) {
  const char* msg = lua_tostring(L,1);
  char errmsg[128];
  __pstn_send( msg,errmsg);

  lua_pushstring(L, errmsg);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_PstnRecv (lua_State *L) {
  const char* interchartimeout = lua_tostring(L,1);
  const char* timeout = lua_tostring(L,2);
  char *rcvmsg = NULL;
  char errmsg[128] = "";
  __pstn_recv(&rcvmsg, interchartimeout, timeout, errmsg);

  lua_pushstring(L, errmsg);  /*  push result */
  lua_pushstring(L, rcvmsg);  /*  push result */
  UtilStrDup(&rcvmsg, NULL);
  return 2;  /*  number of results */
}

static int terminal_PstnDisconnect (lua_State *L) {
  __pstn_disconnect();
  return 0;  /*  number of results */
}

static int terminal_TcpDisconnect (lua_State *L) {
  inCeTcpDisConnectIP();
  return 0;  /*  number of results */
}

static int terminal_SerConnect (lua_State *L) {
  const char* header = lua_tostring(L,1);
  const char* port = lua_tostring(L,2);
  const char* timeout = lua_tostring(L,3);
  const char* itimeout = lua_tostring(L,4);
  const char* baud = lua_tostring(L,5);
  const char* dataBits = lua_tostring(L,6);
  const char* parity = lua_tostring(L,7);
  const char* stopBits = lua_tostring(L,8);
  const char* bufSize = lua_tostring(L,9);
  int retval = __ser_connect(header, port,timeout,itimeout, baud, dataBits, parity, stopBits, bufSize);
  lua_pushinteger(L, retval);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_SerRecv (lua_State *L) {
  const char* port = lua_tostring(L,1);
  char *data = (char*)__ser_recv(port);
  if(data==NULL) lua_pushstring(L,"");
  else lua_pushstring(L,data);

  UtilStrDup(&data,NULL);
  return 1;  /*  number of results */
}

static int terminal_SerSend (lua_State *L) {
  const char* port = lua_tostring(L,1);
  const char* data = lua_tostring(L,2);
  int retval = __ser_send(port,data);
  lua_pushinteger(L, retval);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_SerData (lua_State *L) {
  const char* port = lua_tostring(L,1);
  bool retval = __ser_data(port);
  lua_pushboolean(L, retval);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_DesRandom (lua_State *L) {
  const char* keySize = lua_tostring(L,1);
  const char* key = lua_tostring(L,2);
  char value[33] = "";
  bool retval = __des_random( keySize, key, value);

  lua_pushboolean(L, retval);  /*  push result */
  lua_pushstring(L, value);  /*  push result */
  return 2;  /*  number of results */
}

static int terminal_PinBlockCba (lua_State *L) {
  unsigned char result[128];
  char string[256];
  const char * key1 = lua_tostring(L,1);
  const char * key2 = lua_tostring(L,2);
  const char * pan = lua_tostring(L,3);
  const char * emvflag = lua_tostring(L,4);
  bool emv = emvflag && (*emvflag == '1');
  bool ok = false;

  ok = SecurityPINBlockCba((unsigned char*)key1, (unsigned char*) key2, (char *)pan, emv,result);
  if( !ok ) lua_pushstring(L,"");
  else {
  	lua_pushstring(L, result);  /*  push result */
  }
  return 1;  /*  number of results */
}

static int terminal_PinBlock (lua_State *L) {
  unsigned char result[128];
  char string[256];
  const char * key = lua_tostring(L,1);
  const char * pan = lua_tostring(L,2);
  const char * amount = lua_tostring(L,3);
  const char * stan = lua_tostring(L,4);
  const char * emvflag = lua_tostring(L,5);
  bool emv = emvflag && (*emvflag == '1');
  bool ok = false;

  ok = SecurityPINBlockWithVariant("iris", (unsigned char) atoi(key), 16, (char *) pan,(char *)stan,(char *)amount,emv,result);
  if( !ok ) lua_pushstring(L,"");
  else {
  	UtilHexToString((const char *)result, 8 , string);
	string[16] = '\0';
  	lua_pushstring(L, string);  /*  push result */
  }
  return 1;  /*  number of results */
}

static int terminal_LocateCpat (lua_State *L) {
  const char* objname = lua_tostring(L,1);
  const char* cardprefix = lua_tostring(L,2);
  char *jsonvalue = NULL;
  char jsontag[20];
  char prefix[20];
  char * agc;
  char * code;
  unsigned int objlength ;
  bool found = false;
  int i;

  static char *data = NULL;

  if(data==NULL) data = (char*)IRIS_GetObjectData( objname, &objlength);

  for(i=0; ;i++) {
	  sprintf( jsontag, "PREFIX%d", i);	
      jsonvalue = (char*)IRIS_GetObjectTagValue( data, jsontag );

      if(jsonvalue) {
		char *p ;
		strcpy(prefix, jsonvalue );
		p = strchr( jsonvalue, 'F');	
		if(p) {
			*p = 0;
			if(strncmp(jsonvalue,cardprefix,strlen(jsonvalue)) == 0 )  {
				found = true;
				sprintf( jsontag,"AGC%d",i);
				agc  =  (char*)IRIS_GetObjectTagValue( data, jsontag );
				sprintf( jsontag,"CODE%d",i);
				code  =  (char*)IRIS_GetObjectTagValue( data, jsontag );
  				lua_pushstring(L, prefix);
  				lua_pushstring(L, agc);
  				lua_pushstring(L, code);
				UtilStrDup(&agc,NULL);	
				UtilStrDup(&code,NULL);	
	  			UtilStrDup(&jsonvalue,NULL);
				break;
			}
		}
	  }
	  else break;

	  UtilStrDup(&jsonvalue,NULL);
  }
  //UtilStrDup(&data,NULL);

  if(!found) {
  		lua_pushstring(L, "");
  		lua_pushstring(L, "");
  		lua_pushstring(L, "");
  }

  return 3;
}

static int terminal_NewObject (lua_State *L) {
  const char* objname = lua_tostring(L,1);
  const char* objdata = lua_tostring(L,2);
  if( objname && objdata && strlen(objdata))
  	IRIS_PutNamedObjectData(objdata, strlen(objdata), objname);
  return 0;  /*  number of results */
}

static int terminal_LRCxor (lua_State *L) {
  const char* msg = lua_tostring(L,1);
  unsigned char checksum = 0;
  unsigned char stmp[1024];
  unsigned int length = strlen(msg)/2;
  char *ptmp = stmp;

  memset(stmp,0,sizeof(stmp)); 
  UtilStringToHex( msg, strlen(msg), stmp);

  while(length--) checksum ^= *ptmp++;
  sprintf(stmp,"%02X",checksum);

  lua_pushstring(L, stmp);  /*  push result */

  return 1;  /*  number of results */
}

static int terminal_Kvc (lua_State *L) {
  const char* keySize = lua_tostring(L,1);
  const char* key = lua_tostring(L,2);
  char kvc[7] = "000000";

   __kvc(keySize, key, kvc);
  lua_pushstring(L, kvc);  /*  push result */

  return 1;  /*  number of results */
}

static int terminal_Owf (lua_State *L) {
  const char* ppasn = lua_tostring(L,1);
  const char* to = lua_tostring(L,2);
  const char* from = lua_tostring(L,3);
  int var = (int)lua_tonumber(L,4);
  const char* data = lua_tostring(L,5);
  bool result = false;

  if ( data &&  strlen(data) > 0 ) {
	unsigned char stmp[17];
	memset(stmp,0,sizeof(stmp)); 
	UtilStringToHex( data, strlen(data), stmp);
  	result =  __owf( NULL, to, from, var?true:false, (const char*)stmp );
  }
  else {
  	result =  __owf( ppasn, to, from, var?true:false, NULL );
  }
  lua_pushboolean(L, result);

  return 1;  /*  number of results */
}

static int terminal_Enc (lua_State *L) {
  const char* data = lua_tostring(L,1);
  const char* variant = lua_tostring(L,2);
  const char* keysize = lua_tostring(L,3);
  const char* key = lua_tostring(L,4);
  const char* iv = lua_tostring(L,5);
  char *data_e = NULL;
  bool ofb = false;

  if( iv && strlen(iv) == 16) ofb = true;
  if( ofb ) {
	char iv_hex[9];
	UtilStringToHex( iv, 16, (unsigned char*)iv_hex);
	SecuritySetIV(iv_hex);
  	data_e =  (char*)__crypt( data,keysize,key,variant,false,true);
	SecuritySetIV(NULL);
  } else {
  	data_e =  (char*)__crypt( data,keysize,key,variant,false,false);
  }
  lua_pushstring(L, data_e);
  UtilStrDup( &data_e ,NULL);

  return 1;  /*  number of results */
}

static int terminal_Dec (lua_State *L) {
  const char* data = lua_tostring(L,1);
  const char* variant = lua_tostring(L,2);
  const char* keysize = lua_tostring(L,3);
  const char* key = lua_tostring(L,4);
  char *data_d = NULL;

  data_d =  (char*)__crypt( data,keysize,key,variant,true,false);
  if(data_d == NULL) lua_pushstring(L, "");
  else lua_pushstring(L, data_d);
  UtilStrDup( &data_d ,NULL);

  return 1;  /*  number of results */
}

static int terminal_Mac (lua_State *L) {
  const char* data = lua_tostring(L,1);
  const char* variant = lua_tostring(L,2);
  const char* key = lua_tostring(L,3);
  char mac_value[17] = "";

  __mac( data,variant,key, strlen(data),false, mac_value ) ;
  lua_pushstring(L, mac_value);

  return 1;  /*  number of results */
}

static int terminal_Derive3Des (lua_State *L) {
  char emptystring[10]="";
  const char* data = lua_tostring(L,1);
  char* variant = (char *)lua_tostring(L,2);
  const char* key = lua_tostring(L,3);
  const char* kek = lua_tostring(L,4);
  bool result = false;

  if( variant == NULL) variant = (char*)emptystring;
  result = __derive_3deskey( data,variant,key,kek );

  lua_pushboolean(L, result);

  return 1;  /*  number of results */
}

static int terminal_RsaStore (lua_State *L) {
  const char* data = lua_tostring(L,1);
  const char* key = lua_tostring(L,2);
  bool result = __rsa_store(data,key);

  lua_pushboolean(L, result);
  return 1;  /*  number of results */
}

static int terminal_TdesRsaStore (lua_State *L) {
  const char* data = lua_tostring(L,1);
  const char* rsakey = lua_tostring(L,2);
  const char* kek = lua_tostring(L,3);
  bool result = false;
  result = __3des_rsa_store((char*)data,atoi(rsakey),atoi(kek));

  lua_pushboolean(L, result);
  return 1;  /*  number of results */
}

static int terminal_RsaEncrypt(lua_State *L) {
  const char* data = lua_tostring(L,1);
  const char* key = lua_tostring(L,2);
  const char* dformat1_len = lua_tostring(L,3);
  char *result = NULL;

  if( dformat1_len && atoi(dformat1_len) > 0) {
	char str[1024];
	__dformat1((char*)data, atoi(dformat1_len), str);
  	result = (char*)__rsa_crypt(str,key);
  }
  else result = (char*)__rsa_crypt(data,key);

  if( result == NULL ) lua_pushstring( L, "");
  else lua_pushstring(L, result);

  UtilStrDup(&result,NULL);
  return 1;  /*  number of results */
}

static int terminal_RsaWrap3Des(lua_State *L) {
  const char* key_rsa = lua_tostring(L,1);
  const char* key_3des = lua_tostring(L,2);
  char result[520] = "";

  bool ret = __rsa_wrap_3des (key_rsa,key_3des,result);

  if( !ret) lua_pushstring( L, "");
  else lua_pushstring(L, result);
  return 1;  /*  number of results */
}

static int terminal_DesStore (lua_State *L) {
  const char* data = lua_tostring(L,1);
  const char* keysize = lua_tostring(L,2);
  const char* key = lua_tostring(L,3);
  bool result = __des_store( data,keysize,key);

  lua_pushboolean(L, result);

  return 1;  /*  number of results */
}

static int terminal_XorData (lua_State *L) {
  const char* data1 = lua_tostring(L,1);
  const char* data2 = lua_tostring(L,2);
  const unsigned long len = lua_tonumber(L,3);

  char stmp1[32];
  char stmp2[32];
  char stmp[64]="";
  char s[5]="";
  int hlen = 0;
  int i=0;

  hlen = UtilStringToHex( data1, len, (unsigned char*)stmp1);
  hlen = UtilStringToHex( data2, len, (unsigned char*)stmp2);

  for(i=0;i<hlen;i++) {
	  stmp1[i] ^= stmp2[i];
	  sprintf(s,"%02X",stmp1[i]);
	  strcat(stmp,s);
  }

  lua_pushstring(L, stmp);  /*  push result */

  return 1;  /*  number of results */
}

static int terminal_Xor3Des (lua_State *L) {
  const char* to = lua_tostring(L,1);
  const char* from = lua_tostring(L,2);
  char* with_key = (char *)lua_tostring(L,3);
  char* with_data = (char *)lua_tostring(L,4);
  char *data=NULL;
  char stmp[9];
  bool result = false;

  if( with_key == NULL || strlen(with_key) == 0 ) with_key = NULL;
  if( with_data == NULL || strlen(with_data) == 0 ) data = NULL;
  else {
	memset(stmp,0,sizeof(stmp));
	UtilStringToHex( with_data, 16, (unsigned char*)stmp);
	data = stmp ;
  }

  result = __key_xor(to, from, with_key, data);
  lua_pushboolean(L, result);  /*  push result */

  return 1;  /*  number of results */
}

static int terminal_GetKey (lua_State *L) {
  const char* keynumber = lua_tostring(L,1);
  char stmp[33]="";

  __get_key(atoi(keynumber),stmp);
  lua_pushstring(L, stmp);  /*  push result */

  return 1;  /*  number of results */
}

static int terminal_InjectInternalKey (lua_State *L) {
	const char* keysize = lua_tostring(L,1);
	const char* key = lua_tostring(L,2);
	char data[33] = "04040404040404040404040404040404";
	bool result = __des_store( data,keysize,key);

	lua_pushboolean(L, result);

  return 1;  /*  number of results */
}

static int terminal_As2805SetBcdLength (lua_State *L) {
  const char* data = lua_tostring(L,1);
  if( data && *data == '1') AS2805BcdLength(true);
  else AS2805BcdLength(false);

  return 0;  /*  number of results */
}

static int terminal_As2805Make (lua_State *L) {
  char stmp[6];
  int fieldnum;
  short i = 0,j = 0;
  bool ok = true;
  T_AS2805FIELD *flds[128];
  char *msg=NULL;

  lua_checkstack(L,2);

  memset(flds,0,sizeof(flds));

  lua_pushnil(L);
  while(lua_next(L,-2)>0) {
      const char *fieldstr = (char*) lua_tostring(L,-1);
	  char *st;
      if(fieldstr == NULL) break;
	  st = (char*)fieldstr;

	  get_csv_str( st , ':', 0 ,stmp);
	  fieldnum = atoi(stmp);
      if(fieldnum < 0 || fieldnum > 128) {
			 ok = false; break; 
	  }
	  st = strchr(st,':') + 1;

	  if( strlen(st)) {
	    flds[j] = (void*)my_malloc(sizeof(T_AS2805FIELD));
	    flds[j]->action = 0;
	    strcpy(flds[j]->type,"");
	    flds[j]->fieldnum = fieldnum;
	    flds[j]->data = (void*)my_malloc(strlen(st) + 1);
	    memcpy( flds[j]->data,st,strlen(st) + 1 );
	    ++j;
	  } else {
		flds[j] = NULL;
	  }

	  lua_pop(L,1);
  }
  lua_pop(L,1);

  if(ok) __as2805_make(flds, &msg);

  for(i=0;i<j;i++)  {
	if(flds[i]) {
    	if(flds[i]->data)my_free(flds[i]->data);
    	my_free(flds[i]);
	}
  }

  
  if(msg) {
    lua_pushstring(L, msg);  /*  push result */
    UtilStrDup(&msg,NULL);
  } else
    lua_pushstring(L, "");  /*  push result */

  return 1;  /*  number of results */

}

static int terminal_As2805MakeCustom (lua_State *L) {
  char stmp[20];
  int fieldnum;
  short i = 0,j = 0;
  bool ok = true;
  T_AS2805FIELD *flds[128];
  char *msg=NULL;
  char fldtype[4];


  memset(flds,0,sizeof(flds));

  lua_pushnil(L);
  while(lua_next(L,-2)>0) {
      const char *fieldstr = (char*) lua_tostring(L,-1);
	  char *st;
      if(fieldstr == NULL) break;

      st = (char*)fieldstr;

	  get_csv_str( st , ',', 0,fldtype );
      st = strchr( (char*)st,',') + 1;

	  get_csv_str( st , ',', 0 ,stmp);
	  fieldnum = atoi(stmp);
      st = strchr( (char*)st,',') + 1;

	  flds[j] = (void*)my_malloc(sizeof(T_AS2805FIELD));
	  flds[j]->action = 0;
	  strcpy(flds[j]->type,fldtype);
	  flds[j]->fieldnum = fieldnum;
	  flds[j]->data = (void*)my_malloc(strlen(st) + 1);
	  memcpy( flds[j]->data,st,strlen(st) + 1 );

	  ++j;
	  lua_pop(L,1);
  }
  lua_pop(L,1);

  if(ok) __as2805_make_custom(flds, &msg);

  for(i=0;i<j;i++)  {
    if(flds[i] && flds[i]->data) my_free(flds[i]->data);
    if(flds[i]) my_free(flds[i]);
  }

  if(msg) {
    lua_pushstring(L, msg);  /*  push result */
    UtilStrDup(&msg,NULL);
  } else
    lua_pushstring(L, "");  /*  push result */

  return 1;  /*  number of results */
}

static int terminal_As2805Break (lua_State *L) {
  const char *msg = (char*) lua_tostring(L,1);
  int rtnnumber = 0;
  char stmp[20];
  short i = 0,j = 0;
  bool ok = true;
  T_AS2805FIELD flds[128];
  int errfld ;

  memset(flds,0,sizeof(flds));

  lua_pushnil(L);
  while(lua_next(L,-2)>0) {
      const char *fieldstr = (char*) lua_tostring(L,-1);
	  char *st;
      if(fieldstr == NULL) break;
      st = (char*)fieldstr;

	  flds[j].data = NULL;
	  get_csv_str( st , ',', 0,stmp );
	  if(strcmp(stmp,"GET")==0) flds[j].action = C_GET;
	  else if(strcmp(stmp,"GETS")==0) flds[j].action = C_GETS;
	  else if(strcmp(stmp,"CHK")==0) flds[j].action = C_CHK;
	  else flds[j].action = C_IGN;
      st = strchr( (char*)st,',')+1 ;
	  get_csv_str( st , ',', 0,stmp );
	  flds[j].fieldnum = atoi(stmp);
	  strcpy(flds[j].type,"");

	  ++j;
	  lua_pop(L,1);
  }
  lua_pop(L,1);

  if(ok) __as2805_break(j,msg,flds, &errfld);
  if( errfld >= 0 ) ok = false;

  if(!ok) lua_pushnumber(L, errfld);  /*  push result */
  else lua_pushstring(L,"NOERROR");
  rtnnumber = 1;

  for(i=0;i<j;i++)  {
    if(flds[i].data && ( flds[i].action == C_GET || flds[i].action == C_GETS)) {
			if(flds[i].data) lua_pushstring(L, (const char *)flds[i].data);  /*  push result */
			else lua_pushstring(L,"");
			rtnnumber ++;
	}
   
    if(flds[i].data) my_free(flds[i].data);
  }

  return rtnnumber;  /*  number of results */

}

static int terminal_As2805BreakCustom (lua_State *L) {
  const char *msg = (char*) lua_tostring(L,1);
  int rtnnumber = 0;
  char stmp[20];
  short i = 0,j = 0;
  bool ok = true;
  T_AS2805FIELD *flds[128];
  int errfld ;


  memset(flds,0,sizeof(flds));

  lua_pushnil(L);
  while(lua_next(L,-2)>0) {
      const char *fieldstr = (char*) lua_tostring(L,-1);
	  char *st;
      if(fieldstr == NULL) break;
      st = (char*)fieldstr;

	  flds[j] = (void*)my_malloc(sizeof(T_AS2805FIELD));
	  get_csv_str( st , ',', 0,stmp );
	  if(strcmp(stmp,"GET")==0) flds[j]->action = C_GET;
	  else if(strcmp(stmp,"GETS")==0) flds[j]->action = C_GETS;
	  else if(strcmp(stmp,"CHK")==0) flds[j]->action = C_CHK;

      st = strchr( (char*)st,',') + 1;
	  get_csv_str( st , ',', 0,stmp );
	  strcpy(flds[j]->type, stmp);

      st = strchr( (char*)st,',') + 1;
	  get_csv_str( st , ',', 0,stmp );
	  flds[j]->fieldnum = atoi(stmp);


	  strcpy(flds[j]->type,"");
	  if( flds[j]->action == C_CHK) {
        st = strchr( (char*)st,',') + 1;
	    flds[j]->data = (void*)my_malloc(strlen(st) + 1);
	    memcpy( flds[j]->data,st,strlen(st) + 1 );
	  } else flds[j]->data = NULL;

	  ++j;
	  lua_pop(L,1);
  }

  if(ok) __as2805_break_custom(msg,flds, &errfld);
  if( errfld >= 0 ) ok = false;

  if(!ok) lua_pushnumber(L, errfld);  /*  push result */
  else lua_pushstring(L,"NOERROR");
  rtnnumber = 1;

  for(i=0;i<j;i++)  {
    if(flds[i] && flds[i]->data && ( flds[i]->action == C_GET || flds[i]->action == C_GETS)) {
			lua_pushstring(L, (const char *)flds[i]->data);  /*  push result */
			rtnnumber ++;
	}
   
    if(flds[i] && flds[i]->data) my_free(flds[i]->data);
    if(flds[i]) my_free(flds[i]);
  }

  return rtnnumber;  /*  number of results */

}

static int terminal_Reboot (lua_State *L) {
  SVC_RESTART("");
  return 0;  /*  number of results */
}

static int terminal_SecInit (lua_State *L) {
  __sec_init();
  return 0;  /*  number of results */
}

static int terminal_HexToString(lua_State *L) {
  const char* data = lua_tostring(L,1);
  int len = strlen(data);
  char *string  = (char*)my_malloc(len*2+1);
  UtilHexToString( data, len, string );

  lua_pushstring(L, string);  /*  push result */
  if(string) my_free(string);
  return 1;  /*  number of results */
}

static int terminal_StringToHex (lua_State *L) {
  const char* data = lua_tostring(L,1);
  int len = (int)lua_tonumber(L,2);
  char *hex=NULL;

  if(len==0) len = strlen(data);
  hex  = (char*)my_malloc(len+1);
  len = UtilStringToHex( data, len, (unsigned char *)hex );
  hex[len] = 0;

  lua_pushstring(L, hex);  /*  push result */
  if(hex) my_free(hex);
  return 1;  /*  number of results */
}

static int terminal_Ppid (lua_State *L) {
  char ppid[17]="";
  __ppid(ppid);
  lua_pushstring(L, ppid);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_PpidUpdate (lua_State *L) {
  const char* ppid = lua_tostring(L,1);
  __ppid_update(ppid);
  return 0;  /*  number of results */
}

static int terminal_SerialNo (lua_State *L) {
  char sn[13]="";
  __serial_no(sn);
  lua_pushstring(L, sn);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_IvClr (lua_State *L) {
  bool result = SecuritySetIV(NULL);
  lua_pushboolean(L, result);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_SetIvMode (lua_State *L) {
  const char* ivmode = lua_tostring(L,1);
  bool iv = true;
  bool result;

  if(strcmp(ivmode,"0")==0) iv = false;

  result = getset_ivmode(1,iv);
  lua_pushboolean(L, result);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_PrinterStatus (lua_State *L) {
  char result[30];
  __print_err(result);
  lua_pushstring(L, result);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_Print (lua_State *L) {
  const char* prtdata = lua_tostring(L,1);
  int endflag = lua_toboolean(L,2);
  char* prtrtn = NULL;

  prtrtn = (char*)__print_cont( prtdata, endflag );
  lua_pushstring(L, prtrtn);  /*  push result */
  UtilStrDup(&prtrtn,NULL);
  return 1;  /*  number of results */
}

static int terminal_Luhn (lua_State *L) {
  const char* pan = lua_tostring(L,1);
  bool luhnok = __luhn(pan);
  lua_pushboolean(L, luhnok);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_GetArrayRange (lua_State *L) {
  const char* arrfile = lua_tostring(L,1);
  unsigned int objlength;
  char *data = (char*)IRIS_GetObjectData( "ARRAYRANGE", &objlength);
  char * jsonvalue = NULL;
  char stmp[100];
  int a_min = 0;
  int a_max = 0;
		  
  if(data)	{
    sprintf(stmp,"%s_MIN",arrfile);
  	jsonvalue = (char*)IRIS_GetObjectTagValue( data, stmp);
	if(jsonvalue)  {
		a_min = atoi(jsonvalue);
		UtilStrDup(&jsonvalue,NULL);
	}

    sprintf(stmp,"%s_MAX",arrfile);
  	jsonvalue = (char*)IRIS_GetObjectTagValue( data, stmp);
	if(jsonvalue)  {
		a_max = atoi(jsonvalue);
		UtilStrDup(&jsonvalue,NULL);
	}
	UtilStrDup(&data,NULL);
  }

  lua_pushinteger(L, a_min);  /*  push result */
  lua_pushinteger(L, a_max);  /*  push result */
  return 2;  /*  number of results */
}

static int terminal_SetArrayRange (lua_State *L) {
  const char* arrfile = lua_tostring(L,1);
  const char* a_min = lua_tostring(L,2);
  const char* a_max = lua_tostring(L,3);

  char *fullname= "ARRAYRANGE";
  char tag[100];

  if(a_min && strlen(a_min)) {
    sprintf(tag,"%s_MIN", arrfile);
    IRIS_StoreData(fullname,tag, a_min, 0/* false */);
  }

  if(a_max && strlen(a_max)) {
    sprintf(tag,"%s_MAX", arrfile);
    IRIS_StoreData(fullname, tag,a_max, 0/* false */);
  }
		  
  return 0;  /*  number of results */
}


static int terminal_FileExist (lua_State *L) {
  const char* filename = lua_tostring(L,1);
  bool ok = FileExist(filename);
  lua_pushboolean(L, ok);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_FileRemove (lua_State *L) {
  const char* filename = lua_tostring(L,1);
  bool ok = FileRemove(filename);
  lua_pushboolean(L, ok);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_FileRename (lua_State *L) {
  const char* ofilename = lua_tostring(L,1);
  const char* nfilename = lua_tostring(L,2);
  bool ok = FileRename(ofilename,nfilename);
  lua_pushboolean(L, ok);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_FileCopy (lua_State *L) {
  const char* s_filename = lua_tostring(L,1);
  const char* d_filename = lua_tostring(L,2);
  int ret = file_copy(s_filename,d_filename);
  if(ret == -1) {
	ret = _remove(d_filename);
  	ret = file_copy(s_filename,d_filename);
  }
  lua_pushinteger(L, ret);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_DisplayArray (lua_State *L) {
  const unsigned long tmout = lua_tonumber(L,1);
  const unsigned long cnt = lua_tonumber(L,2);
  short i = 0,j = 0;
  char *msglist[100];
  unsigned short selected = 0;

  memset( msglist,0,sizeof(msglist));
  lua_checkstack(L,2);
  lua_pushnil(L);
  while(lua_next(L,-2)>0) {
      const char *fieldstr = (char*) lua_tostring(L,-1);
      if(fieldstr == NULL) break;

	  if( strlen(fieldstr)) {
		msglist[i] = (void*)my_malloc(strlen(fieldstr)+1);
	    strcpy( msglist[i],fieldstr);
	  	++i;
	  }

	  lua_pop(L,1);
  }
  lua_pop(L,1);

  if(i>0) selected = DispArray( tmout, msglist, cnt );
  lua_pushnumber(L,selected);
  if (selected) lua_pushstring(L,msglist[selected-1]);
  else lua_pushstring(L, "");  /*  push result */

  for(j=0;j<i;j++)  {
    my_free(msglist[j]);
  }

  return 2;  /*  number of results */

}

static int terminal_PowerSaveMode (lua_State *L) {
  const unsigned long evt = lua_tonumber(L,1);
  const unsigned long tcpchk = lua_tonumber(L,2);
  const unsigned long tmout = lua_tonumber(L,3);
  const unsigned long tmout_2 = lua_tonumber(L,4);
  const unsigned long tmout_3 = lua_tonumber(L,5);
  __lowPower( evt,tcpchk,tmout,tmout_2,tmout_3);
  return 0;  /*  number of results */
}

#ifdef __EMV
static int terminal_EmvPowerOn (lua_State *L) {
  int emvret = EMVPowerOn();
  lua_pushinteger(L, emvret);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_EmvTransInit (lua_State *L) {
  int emvret = EMVTransInit();
  lua_pushinteger(L, emvret);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_EmvSelectApplication (lua_State *L) {
  const unsigned long amt = lua_tonumber(L,1);
  const unsigned long acc = lua_tonumber(L,2);
  int emvret = EMVSelectApplication(amt,acc);
  int apps = gEmv.appsTotal;
  int eftpos_mcard = gEmv.eftpos_mcard;

  lua_pushinteger(L, emvret);  /*  push result */
  lua_pushinteger(L, apps);  /*  push result */
  lua_pushinteger(L, eftpos_mcard);  /*  push result */
  return 3;  /*  number of results */
}

static int terminal_EmvSetAmt (lua_State *L) {
  long amt = lua_tonumber(L,1);
  long cashamt = lua_tonumber(L,2);
  int emvret = EmvSetAmt(amt,cashamt);
  lua_pushinteger(L, emvret);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_EmvSetAccount (lua_State *L) {
  int acc = lua_tonumber(L,1);
  unsigned char AccountType = acc;
  int ret = 0;
  if(acc==0x10||acc==0x20||acc==0x30) 
	 ret = EmvSetAccount(AccountType);
  else
     ret = -1;
  lua_pushinteger(L, ret);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_EmvReadAppData (lua_State *L) {
  int emvret = EMVReadAppData();
  lua_pushinteger(L, emvret);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_EmvDataAuth (lua_State *L) {
  int emvret = EMVDataAuth();
  lua_pushinteger(L, emvret);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_EmvProcRestrict (lua_State *L) {
  int emvret = EMVProcessingRestrictions();
  lua_pushinteger(L, emvret);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_EmvCardholderVerify (lua_State *L) {
  int emvret = EmvCardholderVerify();
  lua_pushinteger(L, emvret);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_EmvProcess1stAC (lua_State *L) {
  int emvret = EmvProcess1stAC();
  lua_pushinteger(L, emvret);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_EmvUseHostData (lua_State *L) {
  unsigned long hostdesc = (unsigned long)lua_tonumber(L,1);
  const char* value = lua_tostring(L,2);
  unsigned char hex[1024]="";
  int emvret = 0;

  if(value && strlen(value)) UtilStringToHex( value, strlen(value), hex );
  emvret = EmvUseHostData((int)hostdesc,(const char *)hex,strlen(value)/2);
  lua_pushinteger(L, emvret);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_EmvPowerOff (lua_State *L) {
  int emvret = EmvCardPowerOff();
  lua_pushinteger(L, emvret);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_EmvPackTLV (lua_State *L) {
  const char* tags_in = lua_tostring(L,1);
  char tags[1024]="";
  char tlv[2048]="";
  bool ok = false;

  strcpy(tags,tags_in);
  ok = _emv_pack_tlv(tags,tlv);
  if( !ok ) strcpy(tlv,"");
  lua_pushstring(L, tlv);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_EmvIsCardPresent (lua_State *L) {
  bool emvret = EmvIsCardPresent();
  lua_pushboolean(L, emvret);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_EmvGetTagData (lua_State *L) {
  char result[256]="";
  int top = lua_gettop(L);
  int i = 0;
  int retcnt = 0;

  for(i=1;i<=top;i++) {
  	unsigned short tag = (unsigned short)lua_tonumber(L,i);
    int emvret = EmvGetTagDataRaw(tag,result);
    lua_pushstring(L, result);  /*  push result */
	++retcnt;
  }

  return retcnt;  /*  number of results */
}

static int terminal_EmvSetTagData (lua_State *L) {
  unsigned char hex[256]="";
  unsigned short tag = (unsigned short)lua_tonumber(L,1);
  const char * value = lua_tostring(L,2);
  int len = 0;
  int ret = 0;
  char stag[3];

  if( value == NULL || strlen(value) ==0) {
  	lua_pushinteger(L, 0);  /*  push result */
  } else {
  	memcpy(stag,(char*)&tag,2);
  	len =  UtilStringToHex( value, strlen(value), hex );
  	ret = EmvSetTagData((const unsigned char *)stag,(const unsigned char *)hex,len);
  	lua_pushinteger(L, ret);  /*  push result */
  }
  return 1;  /*  number of results */
}

static int terminal_EmvGlobal (lua_State *L) {
  const char* action = lua_tostring(L,1);
  const char* name = lua_tostring(L,2);
  const char* value = lua_tostring(L,3);
  short retcnt = 0;

  if( strcmp(action,"GET") == 0 ) {
	if( strcmp(name, "SIGN") ==0) lua_pushboolean(L, gEmv.sign);
	else if( strcmp(name, "PINENTRY") ==0) lua_pushboolean(L, gEmv.pinentry);
	else if( strcmp(name, "OFFLINEPIN") ==0) lua_pushboolean(L, gEmv.offlinepin);
	else if( strcmp(name, "ONLINEPIN") ==0) lua_pushboolean(L, gEmv.onlinepin);
	else if( strcmp(name, "AMT") ==0) lua_pushinteger(L, gEmv.amt);
	else if( strcmp(name, "TERMDESICIION") ==0) lua_pushinteger(L, gEmv.termDesicion);
	else if( strcmp(name, "CHIPFALLBACK") ==0) lua_pushboolean(L, gEmv.chipfallback);
	else if( strcmp(name, "TECHFALLBACK") ==0) lua_pushboolean(L, gEmv.techfallback);
	else if( strcmp(name, "ACCT") ==0) lua_pushstring(L, gEmv.acct);
	else if( strcmp(name, "ACCT_NOCR") ==0) lua_pushboolean(L, gEmv.acct_nocr);
	else if( strcmp(name, "APPSTOTAL") ==0) lua_pushinteger(L, gEmv.appsTotal);
	else lua_pushstring(L,"");
	retcnt = 1;
  }
  else if( strcmp(action,"SET") == 0 ) {
	if( strcmp(name, "TERMDESICION") ==0) gEmv.termDesicion = atoi(value);
  }

  return retcnt;  /*  number of results */
}

static int terminal_EmvResetGlobal (lua_State *L) {
  memset( &gEmv,0,sizeof(gEmv));

  return 0;  /*  number of results */
}

static int terminal_EmvGetTacIac (lua_State *L) {
  char tac_df[30];
  char tac_dn[30];
  char tac_ol[30];
  char iac_df[30];
  char iac_dn[30];
  char iac_ol[30];
  int ret = EmvGetTacIac(tac_df,tac_dn,tac_ol,iac_df,iac_dn,iac_ol);
  lua_pushstring(L,tac_df);
  lua_pushstring(L,tac_dn);
  lua_pushstring(L,tac_ol);
  lua_pushstring(L,iac_df);
  lua_pushstring(L,iac_dn);
  lua_pushstring(L,iac_ol);
  return 6;  /*  number of results */
}

static int terminal_CTLSEmvGetTac (lua_State *L) {
	const char *AID = lua_tostring(L,1);
  char tac_df[30];
  char tac_dn[30];
  char tac_ol[30];
  int ret = 0;
  memset(tac_df,0x00,sizeof(tac_df));
  memset(tac_dn,0x00,sizeof(tac_dn));
  memset(tac_ol,0x00,sizeof(tac_ol));
  
  ret = CTLSEmvGetTac(tac_df,tac_dn,tac_ol,AID);
  lua_pushstring(L,tac_df);
  lua_pushstring(L,tac_dn);
  lua_pushstring(L,tac_ol);
  
  return 3;  /*  number of results */
}

static int terminal_CTLSEmvGetCfg (lua_State *L) {
  const char *AID = lua_tostring(L,1);
  unsigned long Tag = lua_tonumber(L,2);
  int ret = 0;
  char sValue[1024] = "";

  CtlsGetCfg(AID,Tag,sValue);
  lua_pushstring(L,sValue);

  return 1;  /*  number of results */
}

static int terminal_CTLSEmvGetLimit (lua_State *L) {
  const char *aid = lua_tostring(L,1);
  int ret = 0;
  int translimit = 0;
  int cvmlimit = 0;
  int floorlimit = 0;
  
  ret = GetCtlsTxnLimit(aid, &translimit, &cvmlimit, &floorlimit);
  lua_pushnumber(L,translimit);
  lua_pushnumber(L,cvmlimit);
  lua_pushnumber(L,floorlimit);
  
  return 3;  /*  number of results */
}


static int terminal_EmvTLVReplace(lua_State *L) {
  const char *tlvs = lua_tostring(L,1);
  unsigned short btag = 0;
  short len;
  int top = lua_gettop(L);
  short i;
  char newtlvs[2048];
  char h_tlvs[1024];

  strcpy(newtlvs,tlvs);
  len = UtilStringToHex( newtlvs, strlen(newtlvs), (unsigned char *)h_tlvs);

  lua_checkstack(L,2);
  for(i=2;i<=top;i++) {
      btag = (unsigned short) lua_tonumber(L,i);
      if(btag == 0) break;
	  emv_tlv_replace( h_tlvs,&len, btag);
  }

  memset(newtlvs,0,sizeof(newtlvs));
  UtilHexToString( h_tlvs, len, newtlvs);
  lua_pushstring(L, newtlvs);  /*  push result */
  return 1;  /*  number of results */
}

static int terminal_EmvFindCfgFile (lua_State *L) {
  const char *aid = lua_tostring(L,1);
  char cfgfile[50]="";
  int ret = 0;

  ret = EmvFindCfgFile(aid,cfgfile);
  lua_pushstring(L,cfgfile);

  return 1;  /*  number of results */
}

#endif

static int terminal_InitCommEng (lua_State *L) {
  const char *initType = lua_tostring(L,1);
  if(initType == NULL || strlen(initType) == 0 || strcmp(initType,"COMM") == 0) InitComEngine();
  if(initType == NULL || strlen(initType) == 0 || strcmp(initType,"CTLS") == 0) InitCtlsPort();
  return 1;  /*  number of results */
}

static int terminal_CtlsCall (lua_State *L) {
  long acc = lua_tonumber(L,1);
  long amt = lua_tonumber(L,2);
  long nosaf = lua_tonumber(L,3);
  char tr1[100]="";
  char tr2[50]="";
  char tlvs[1024]="";
  char emvres[10]="";

  memset(tlvs,0,sizeof(tlvs));
  ctlsCall(acc,amt,nosaf,tr1,tr2,tlvs,emvres);
	 
  lua_pushstring(L, tr1);  /*  push result */
  lua_pushstring(L, tr2);  /*  push result */
  lua_pushstring(L, tlvs);  /*  push result */
  if(strlen(emvres)) 
		lua_pushstring(L, emvres);  /*  push result */
  else
		lua_pushstring(L, "100");  /*  push result */
		
  return 4; /*  number of results */
}

static int terminal_SysEnv(lua_State *L) {
  const char *method = lua_tostring(L,1);
  const char *key = lua_tostring(L,2);
  const char *k_value = lua_tostring(L,3);
  char value[64]="";

  memset(value,0,sizeof(value));

  if(!strcmp(method, "GET") && key) get_env(key, value, sizeof(value));
  if(!strcmp(method, "PUT") && key && k_value) put_env(key, k_value, strlen(k_value));

  lua_pushstring(L, value);  /*  push result */
  return 1; /*  number of results */
}

static int terminal_iStartGPS (lua_State *L)
{
    const char *method = lua_tostring(L,1);
	if(method == NULL || strcmp(method,"ENGINE")==0)
		iStartGPS();
	if(method == NULL || strcmp(method,"LIBRARY")==0)
		iStartLibGPS();
	return 0; /*  number of results */
}
static int terminal_iStopGPS (lua_State *L)
{
    const char *method = lua_tostring(L,1);
	if(method == NULL || strcmp(method,"LIBRARY")==0)
		iStopLibGPS();
	if(method == NULL || strcmp(method,"ENGINE")==0)
		iStopGPS();
	return 0; /*  number of results */
}

static int terminal_SetTcpBreak (lua_State *L)
{
	int allowbreak = lua_tonumber(L,1);
	SetNonBlockTcp(allowbreak);
	return(0);
}

static int terminal_SetBackLight (lua_State *L)
{
	int backlight = lua_tonumber(L,1);
	set_backlight(backlight);
	return(0);
}

static const luaL_Reg terminallib[] = {
  {"SetDisplayFlush",terminal_SetDisplayFlush},
  {"DisplayObject", terminal_DisplayObject},
  {"DebugDisp", terminal_DebugDisp},
  {"SetNextObject", terminal_SetNextObject},
  {"SetScreenName", terminal_SetScreenName},
  {"TextTable", terminal_TextTable},
  {"Now", terminal_Now},
  {"SysTicks", terminal_SysTicks},
  {"Time", terminal_Time},
  {"TimeSet", terminal_TimeSet},
  {"GetJsonValue", terminal_GetJsonValue},
  {"GetJsonValueInt", terminal_GetJsonValueInt},
  {"GetJsonObject", terminal_GetJsonObject},
  {"SetJsonValue", terminal_SetJsonValue},
  {"GetTrack",terminal_GetTrack},
  {"ErrorBeep",terminal_ErrorBeep},
  {"Sleep",terminal_Sleep},
  {"NewObject",terminal_NewObject},
  {"LocateCpat",terminal_LocateCpat},
  {"DoTmsCmd",terminal_DoTmsCmd},
  {"LRCxor",terminal_LRCxor},
  {"GetFileInfo",terminal_GetFileInfo},
  {"GetDirInfo",terminal_GetDirInfo},
  {"GetBattery",terminal_GetBattery},

  {"UploadObj",terminal_UploadObj},
  {"UploadMsg",terminal_UploadMsg},
  {"Remote",terminal_Remote},
  {"TcpConnect",terminal_TcpConnect},
  {"TcpReconnect",terminal_TcpReconnect},
  {"IpConnect",terminal_IpConnect},
  {"TcpSend",terminal_TcpSend},
  {"PstnConnect",terminal_PstnConnect},
  {"PstnWait",terminal_PstnWait},
  {"PstnSend",terminal_PstnSend},
  {"TcpRecv",terminal_TcpRecv},
  {"PstnRecv",terminal_PstnRecv},
  {"PstnDisconnect",terminal_PstnDisconnect},
  {"TcpDisconnect",terminal_TcpDisconnect},
  {"SerConnect",terminal_SerConnect},
  {"SerData",terminal_SerData},
  {"SerSend",terminal_SerSend},
  {"SerRecv",terminal_SerRecv},

  {"DesRandom",terminal_DesRandom},
  {"PinBlock",terminal_PinBlock},
  {"PinBlockCba",terminal_PinBlockCba},
  {"Kvc",terminal_Kvc},
  {"Owf",terminal_Owf},
  {"Dec",terminal_Dec},
  {"Enc",terminal_Enc},
  {"Mac",terminal_Mac},
  {"IvClr",terminal_IvClr},
  {"SetIvMode",terminal_SetIvMode},
  {"Derive3Des",terminal_Derive3Des},
  {"DesStore",terminal_DesStore},
  {"RsaStore",terminal_RsaStore},
  {"TdesRsaStore",terminal_TdesRsaStore},
  {"RsaEncrypt",terminal_RsaEncrypt},
  {"RsaWrap3Des",terminal_RsaWrap3Des},
  {"Xor3Des",terminal_Xor3Des},
  {"XorData",terminal_XorData},
  {"GetKey",terminal_GetKey},
  {"InjectInternalKey",terminal_InjectInternalKey},

  {"As2805SetBcdLength",terminal_As2805SetBcdLength},
  {"As2805Make",terminal_As2805Make},
  {"As2805MakeCustom",terminal_As2805MakeCustom},
  {"As2805Break",terminal_As2805Break},
  {"As2805BreakCustom",terminal_As2805BreakCustom},

  {"Model",terminal_Model},
  {"Ppid",terminal_Ppid},
  {"PpidUpdate",terminal_PpidUpdate},
  {"SerialNo",terminal_SerialNo},
  {"Print",terminal_Print},
  {"PrinterStatus",terminal_PrinterStatus},
  {"Luhn",terminal_Luhn},
  {"GetArrayRange",terminal_GetArrayRange},
  {"SetArrayRange",terminal_SetArrayRange},
  {"FileExist",terminal_FileExist},
  {"FileRemove",terminal_FileRemove},
  {"FileRename",terminal_FileRename},
  {"FileCopy",terminal_FileCopy},
  {"Reboot",terminal_Reboot},
  {"SecInit",terminal_SecInit},
  {"HexToString",terminal_HexToString},
  {"StringToHex",terminal_StringToHex},
  {"DisplayArray",terminal_DisplayArray},
  {"PowerSaveMode",terminal_PowerSaveMode},
  {"InitCommEng",terminal_InitCommEng},
  {"CtlsCall",terminal_CtlsCall},	  
  {"SysEnv",terminal_SysEnv},
  {"CTLSEmvGetTac",terminal_CTLSEmvGetTac},
  {"CTLSEmvGetCfg",terminal_CTLSEmvGetCfg},
  {"CTLSEmvGetLimit",terminal_CTLSEmvGetLimit},
  
#ifdef __EMV

  {"EmvPowerOn",terminal_EmvPowerOn},
  {"EmvTransInit",terminal_EmvTransInit},
  {"EmvSelectApplication",terminal_EmvSelectApplication},
  {"EmvSetAmt",terminal_EmvSetAmt},
  {"EmvSetAccount",terminal_EmvSetAccount},
  {"EmvReadAppData",terminal_EmvReadAppData},
  {"EmvDataAuth",terminal_EmvDataAuth},
  {"EmvProcRestrict",terminal_EmvProcRestrict},
  {"EmvCardholderVerify",terminal_EmvCardholderVerify},
  {"EmvProcess1stAC",terminal_EmvProcess1stAC},
  {"EmvUseHostData",terminal_EmvUseHostData},
  {"EmvPowerOff",terminal_EmvPowerOff},
  {"EmvPackTLV",terminal_EmvPackTLV},
  {"EmvIsCardPresent",terminal_EmvIsCardPresent},
  {"EmvGetTagData",terminal_EmvGetTagData},
  {"EmvSetTagData",terminal_EmvSetTagData},
  {"EmvGlobal",terminal_EmvGlobal},
  {"EmvResetGlobal",terminal_EmvResetGlobal},
  {"EmvGetTacIac",terminal_EmvGetTacIac},
  {"EmvTLVReplace",terminal_EmvTLVReplace},
  {"EmvFindCfgFile",terminal_EmvFindCfgFile},
#endif
  {"iStartGPS",terminal_iStartGPS},
  {"iStopGPS",terminal_iStopGPS},
  {"SetTcpBreak",terminal_SetTcpBreak},
  {"SetBackLight",terminal_SetBackLight},
  {NULL, NULL}
};


LUALIB_API int luaopen_terminal (lua_State *L) {
  luaL_register(L, "terminal", terminallib);
  return 1;
}

