#ifndef __DS1302_H__
#define __DS1302_H__

#include <stdint.h>

typedef struct
{
    uint8_t year;   // 0~99 (예: 2025년 -> 25)
    uint8_t month;  // 1~12
    uint8_t day;    // 1~31 (date)
    uint8_t week;   // 1~7 (weekday)
    uint8_t hour;   // 0~23
    uint8_t minute; // 0~59
    uint8_t second; // 0~59
} DS1302_Time_t;

void DS1302_Init(void);
void DS1302_GetTime(DS1302_Time_t *time);
void DS1302_SetTime(DS1302_Time_t *time);

// DS1302 내부 RAM (31 bytes) Read/Write
void DS1302_ReadRAM(uint8_t offset, uint8_t *buf, uint8_t len);
void DS1302_WriteRAM(uint8_t offset, const uint8_t *buf, uint8_t len);

#endif // __DS1302_H__
