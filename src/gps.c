
//==============================================================================
// INCLUDES 
//==============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <svc.h>
#include <time.h>

#include <eoslog.h>
#include <ceif.h>
#include <vxgps.h>

//==============================================================================
// DEFINES
//==============================================================================

#ifndef APP_LOG_FILTER
#define APP_LOG_FILTER               0x80000000
#endif

//==============================================================================
// GLOBALS
//==============================================================================

struct gps_data_t g_stGpsData;

//==============================================================================
//==============================================================================
// FUNCTIONS

//==============================================================================

//==============================================================================
// 
//==============================================================================
int iStartGPS(void)
{
	int iRet;
	
	iRet = gps_engine_start(5, NULL, NULL, 0, NULL);
	LOG_PRINTF("gps_engine_start() returned %d.", iRet);
	
	return 0;
}

//==============================================================================
// 
//==============================================================================
int iStopGPS(void)
{
	int iRet;
	
	iRet = gps_engine_stop();
	LOG_PRINTF("gps_engine_stop() returned %d.", iRet);
	
	return 0;
}

//==============================================================================
// 
//==============================================================================
int iStartLibGPS(void)
{
	char cpVer[32];
	const char *cpRet;
	int iFlags;
	int iRet;

	// Get and print GPS Lib version
	memset(cpVer, 0, sizeof(cpVer));
	iRet = gps_version(cpVer);
	LOG_PRINTF("gps_version() returned %d, Ver=%s.", iRet, cpVer);

	// Init GPS structure and connect to GPSD
	memset(&g_stGpsData, 0, sizeof(struct gps_data_t));
	iRet = gps_open("127.0.0.1", "2947", &g_stGpsData);
	LOG_PRINTF("gps_open() returned %d.", iRet);
	if (iRet == -1)
	{
		cpRet = gps_errstr(errno);
		LOG_PRINTF("gps_errstr(%d) returned [%s].", errno, cpRet);
		return -1;
	}
	
	// Set receive updates from GPSD
	iFlags = WATCH_ENABLE | WATCH_JSON | WATCH_SCALED;
	iRet = gps_stream(&g_stGpsData, iFlags, NULL);
	LOG_PRINTF("gps_stream(0x%x) returned %d.", iFlags, iRet);
	
	
	return 0;
}

//==============================================================================
// 
//==============================================================================
int iStopLibGPS(void)
{
	int iRet;
	
	iRet = gps_close(&g_stGpsData);
	LOG_PRINTF("gps_close() returned %d.", iRet);

	return 0;	
}

//==============================================================================
// 
//==============================================================================
#define GPS_LOG_DBL(a)  LOG_PRINTF("GpsData.%s: %f", #a, g_stGpsData.a)
int iLogGPS(void)
{
	if (g_stGpsData.set & MODE_SET)
		GPS_LOG_DBL(fix.mode);
	if (g_stGpsData.set & LATLON_SET)
	{
		char gps_lat[64];
		char gps_lon[64];
		GPS_LOG_DBL(fix.latitude);
		GPS_LOG_DBL(fix.longitude);
		sprintf(gps_lat,"%f",g_stGpsData.fix.latitude);
		sprintf(gps_lon,"%f",g_stGpsData.fix.longitude);
		IRIS_StoreData("GPS_CFG","LAT", gps_lat, 0/* false */);
		IRIS_StoreData("GPS_CFG","LON", gps_lon, 0/* false */);
	}
	if (g_stGpsData.set & ALTITUDE_SET)
		GPS_LOG_DBL(fix.altitude);
	if (g_stGpsData.set & TRACK_SET)
		GPS_LOG_DBL(fix.track);
	if (g_stGpsData.set & SPEED_SET)
		GPS_LOG_DBL(fix.speed);
	if (g_stGpsData.set & CLIMB_SET)
		GPS_LOG_DBL(fix.climb);
	
}

//==============================================================================
// 
//==============================================================================
int iReadGPS(void)
{
	int iRet;
	bool bRet;

	bRet = gps_waiting(&g_stGpsData, 0);
	LOG_PRINTF("gps_waiting() returned %d.", bRet);
	if (bRet)
	{
		iRet = gps_read(&g_stGpsData);
		LOG_PRINTF("gps_read() returned %d.", iRet);
		if (iRet > 0)
		{
			iLogGPS();
		}
	} else {
		/* 
		const char *pRet = gps_data(&g_stGpsData);
		if(pRet==NULL) {
			LOG_PRINTF("gps_data() returned NULL.");
		} else
			LOG_PRINTF("gps_data() returned OK.");
		*/
	}

	LOG_PRINTF("%s: End.", __FUNCTION__);
	return 0;	
}


int gpsmain (int argc, char *argv[])
{}
int iReadGPS1(int waitevt, char *gps_lat, char *gps_lon)
{}
