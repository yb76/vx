#ifndef HEADER_FILE_SYSTEM_H
#define HEADER_FILE_SYSTEM_H

#ifdef __cplusplus
extern"C" {
#endif

#include <portdef.h>
#include <acldev.h> // for struct TRACK

/***** System.h *****/
extern int HConsole;

int   Open (char *filename, int attributes);
//Dwarika
//int   Close(int handle);
int   Get_Msg_file( char *file_name );
char  *Get_Msg (unsigned message_num, char *buf_ptr);
int   Display_At(unsigned int column, unsigned int line, char*display_string, unsigned int clear);
int   WINDOW(int x1, int y1, int x2, int y2);
int   DISP_Cursor(short flag);
int   Prompt( int x1, int y1 , unsigned int Msg_Num , unsigned int clear );
char	 *Get_TID (void);
int   GetChar( void );
int   GetAmount( int Type );
int   Read( int handle , char *ValBuff , int Size );
int   Get_Date( char *date );
int   Write( int handle , char *buffer , int Size );
int   Read_Track( char *track, struct TRACK *parsed, char *order);
unsigned long Read_Ticks (void);
void  Get_Time( char * Time_Buff , int Msg_Num);
void  TimeToLong( char *hhmmss, long *secs );
long  Time_Diff( long TimeSec );
void  Key_Flush( void );
void	 Press_Clear( void );
void  Press_Enter( void );
int   Enter_Cancel( void );
int   asc_to_binary(BYTE* dest, BYTE* src, int srcSize, int destSize);
void  DbgPPrint( char* str );
void  debugAscii(char *format, ...);
void GetModel (char * modelName);
int   set_config(const char* szParam, short maxLen, char* szDisplayName, char storeBinary);
int   set_config_num(const char *szParam, char *szDisplayName, float max, unsigned decimalPlaces);

/* Store the current font and replace it with the specified font.
 * [in] newFont: Filename of the font to use
 * Notes: When finished with the specified font, call PopFont() to restore
 * the previous font.
 */
void PushFont (char * newFont);


/* PopFont: Restore the last font stored with PushFont
 */
void PopFont(void);

/* logPrintf: Print formatted output to RS232 if available, or to logfile.txt otherwise.
 * format     [in] Format-control string
 * other args [in] Optional arguments
 * This function is intended as a replacement for Verifone's Nprintf. It should be called
 * in exactly the same way as printf.
 */
int logPrintf(const char* format, ...);

void CheckBattery(void);
/* If the terminal is not docked, checks the battery status. If low, either
 * displays a warning or locks the terminal depending on two thresholds.
 */

/* EnvIsTrue: Determines whether the specified config.sys variable is non-zero.
 * in envString: Environment variable name
 * returns: nonzero if envString exists and is set to a nonzero value, 0 otherwise.
 */
int EnvIsTrue(const char* envString);


/* EnvIsTrueOrMissing: Determines whether the specified config.sys variable is non-zero.
 * Used for environment flags where absence should be interpreted as True for backwards
 * compatibility.
 * in envString: Environment variable name
 * returns: zero if envString exists and is set to zero, non-zero otherwise.
 */
int EnvIsTrueOrMissing(const char* envString);


unsigned long GetUserTimeout10ms(void);

#define USER_ENTRY_TIMED_OUT 0
int GetCharOrTimeout(void);

unsigned long NextTimeout(void);

void RebootTerminal(const char* newStarGo);

#ifdef __cplusplus
} // extern"C"
#endif

#endif // !defined HEADER_FILE_SYSTEM_H
