// main_bt.c
#include "stm32f10x.h"
#include "bt.h"
#include <stdio.h>

int main(void)
{
    char buffer[128];
    char boolean_val[8] = "true";

    uint32_t total_ms = 0;

    BT_Init();  // 블루투스 및 USART 초기화

    while (1)
    {
        total_ms += 5000; // 예시: 5초마다 한 번 전송

        uint32_t d_min  = (total_ms / 1000) / 60;
        uint32_t d_sec  = (total_ms / 1000) % 60;
        uint32_t d_msec = total_ms % 1000;

        // 날짜/시간 값은 예시(고정값) - 실제 RTC가 있으면 여기에 채우면 됨
        int year  = 2025;
        int month = 1;
        int day   = 1;
        int hour  = 0;
        int min   = 0;
        int sec   = (total_ms / 1000) % 60;

        // 테스트와 동일 포맷 유지
        sprintf(buffer,
                "RPL|%04d-%02d-%02d %02d:%02d:%02d|%02lu:%02lu:%03lu|%s\r\n",
                year, month, day, hour, min, sec,
                (unsigned long)d_min,
                (unsigned long)d_sec,
                (unsigned long)d_msec,
                boolean_val);

        BT_SendString(buffer);

        // 5초마다 전송
        BT_DelayMs(5000);
    }

    return 0;
}
