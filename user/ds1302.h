// ds1302.h 예시
#ifndef __DS1302_H
#define __DS1302_H

#include <stdint.h>

typedef struct {
    uint8_t year;   // 20xx 년 (예: 25)
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t week;
} DS1302_Time_t;

void DS1302_Init(void);
void DS1302_GetTime(DS1302_Time_t *time);

#endif
