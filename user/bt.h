// bt.h
#ifndef BT_H
#define BT_H

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_exti.h"
#include "misc.h"

// 블루투스(USART2) 및 PC 터미널(USART1) 초기화
void BT_Init(void);

// USART2(Bluetooth)로 문자열 전송
void BT_SendString(const char *s);

// 간단 ms 딜레이 (BT 테스트용 main_bt.c에서 사용)
void BT_DelayMs(uint32_t ms);

// 스네이크 게임 종료 시 점수/플레이시간을
// main_bt 테스트와 동일한 포맷의 문자열로 전송.
// 포맷: RPL|YYYY-MM-DD HH:MM:SS|mm:ss:mss|SCORE=NN\r\n
void BT_SendScoreFrame(int score, uint32_t duration_ms);

#endif // BT_H
