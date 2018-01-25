#include <rtcconversions.h>

#include <Arduino.h>

#define NDEBUG
#include <Trace.h>

#include <strbuf.h>

#include <strutils.h>


typedef enum {
	TIME_OBJECT = 0,
	TIME_STAMP = 1
} TimeFormat;

typedef struct Time {
	unsigned int year;
	unsigned int month;
	unsigned int day;
	unsigned int weekday;
	unsigned int hour;
	unsigned int minute;
	unsigned int second;
}Time;

Timestamp_t Time_Convert_TO2TS(Time * time) {
  if (time) {
    int mth[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    int mthb[12] = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335};
    bool isLeapYear = (((!(time->year % 4)) && (time->year % 100) ) || (!(time->year % 400)));
    int centuryCrossingsSince1970 = (time->year / 1000) - (1970 / 1000);
    int numLeapYearsSince1970 = ((time->year - 1970) / 4) - centuryCrossingsSince1970;
    unsigned long days = isLeapYear ?
      (numLeapYearsSince1970 + (time->year - 1970) * 365 + mthb[time->month-1] + (time->day)) :
      (numLeapYearsSince1970 + (time->year - 1970) * 365 + mth [time->month-1] + (time->day));

    Timestamp_t timestamp = days * 86400 + time->hour * 3600 + time -> minute * 60 + time -> second;
    
    return timestamp;
  } else {
    return 0;
  }
}


/* STATIC */
bool RTCConversions::cvtToTimestamp(const char *readable, Timestamp_t *result)
{
    TF("RTCConversions::cvtToTimestamp");

    // will parse the following:
    //    "Sat, 28 Jan 2017 22:24:18 GMT"
    // 

    TRACE3("starting with: \'", readable, "\'");
    readable = StringUtils::eatWhitespace(readable);
    TRACE3("after eating leading whitespace: \'", readable, "\'");

    StrBuf dayOfWeek;
    readable = StringUtils::eatPunctuation(StringUtils::getToken(readable, &dayOfWeek), ',');
    TRACE3("day of week: \'", dayOfWeek.c_str(), "\'");
    TRACE3("after eating day of week: \'", readable, "\'");

    StrBuf dayOfMonth;
    readable = StringUtils::eatWhitespace(StringUtils::getToken(readable, &dayOfMonth));
    TRACE3("day of month: \'", dayOfMonth.c_str(), "\'");
    TRACE3("after eating day of month: \'", readable, "\'");

    StrBuf month;
    readable = StringUtils::eatWhitespace(StringUtils::getToken(readable, &month));
    TRACE3("month: \'", month.c_str(), "\'");
    TRACE3("after eating month: \'", readable, "\'");

    StrBuf year;
    readable = StringUtils::eatWhitespace(StringUtils::getToken(readable, &year));
    TRACE3("year: \'", year.c_str(), "\'");
    TRACE3("after eating year: \'", readable, "\'");
    
    StrBuf hour;
    readable = StringUtils::eatPunctuation(StringUtils::getToken(readable, &hour), ':');
    TRACE3("hour: \'", hour.c_str(), "\'");
    TRACE3("after eating hour: \'", readable, "\'");
    
    StrBuf minute;
    readable = StringUtils::eatPunctuation(StringUtils::getToken(readable, &minute), ':');
    TRACE3("minute: \'", minute.c_str(), "\'");
    TRACE3("after eating minute: \'", readable, "\'");
    
    StrBuf seconds;
    readable = StringUtils::eatWhitespace(StringUtils::getToken(readable, &seconds));
    TRACE3("seconds: \'", seconds.c_str(), "\'");
    TRACE3("after eating seconds: \'", readable, "\'");

    StrBuf timezone;
    readable = StringUtils::eatWhitespace(StringUtils::getToken(readable, &timezone));
    TRACE3("timezone: \'", timezone.c_str(), "\'");
    TRACE3("after eating timezone: \'", readable, "\'");

    Time t;
    t.year = atoi(year.c_str());
    t.day = atoi(dayOfMonth.c_str());
    t.hour = atoi(hour.c_str());
    t.minute = atoi(minute.c_str());
    t.second = atoi(seconds.c_str());

    t.month = 0;
    const char *months[] =
      {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    bool foundMonth = false;
    for (int i = 0; !foundMonth && (i < 12); i++)
      if (strcmp(month.c_str(), months[i]) == 0) {
	t.month = i+1;
	foundMonth = true;
      }

    bool foundWeekday = false;
    const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    for (int i = 0; !foundWeekday && (i < 7); i++)
      if (strcmp(dayOfWeek.c_str(), days[i]) == 0) {
	t.weekday = i+1;
	foundWeekday = true;
      }

    TRACE2("Year: ", t.year);
    TRACE2("Month: ", t.month);
    TRACE2("Day: ", t.day);
    TRACE2("Hour: ", t.hour);
    TRACE2("Minute: ", t.minute);
    TRACE2("Second: ", t.second);
    
    *result = Time_Convert_TO2TS(&t);
    TRACE2("Timestamp: ", *result);
    
    return true;
}
