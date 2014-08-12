#ifndef _DATE_HEADER_
#define _DATE_HEADER_

#ifdef  __cplusplus
extern "C" {
#endif

#define DATE_LEN			16

typedef struct
{
	unsigned short year;
	unsigned short month;
	unsigned short day;
	char formattedValue[DATE_LEN];
} Date;

int SetDate(Date* date, const char* const dateString, const char* const format);

char* FormatDate(Date* date, const char* const format);

int DateHasPassed(const Date* date);


#ifdef  __cplusplus
}
#endif

#endif // _DATE_HEADER_
