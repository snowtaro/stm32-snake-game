// pir.h
#ifndef PIR_H_
#define PIR_H_

#include "stm32f10x.h"
#include <stdint.h>

// PIR 인체감지 센서 초기화
// - 본 예제에서는 PC5 핀을 PIR 입력으로 사용
void PIR_Init(void);

// PIR 센서 감지 여부 반환
// 반환값: 0 = 감지 없음, 1 = 사람/움직임 감지
uint8_t PIR_IsPersonPresent(void);

// 시스템 시작 이후 경과한 시간(ms) 반환
// main에서 설정한 SysTick + g_msTicks 기반
uint32_t PIR_GetMillis(void);

#endif // PIR_H_