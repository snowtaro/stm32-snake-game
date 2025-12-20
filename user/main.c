// main.c
#include "stm32f10x.h"
#include "joystick.h"
#include "display.h"
#include "snake.h"
#include "pir.h"
#include "bt.h"
#include "ds1302.h"
#include "cds.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

extern volatile uint32_t g_msTicks;
volatile uint8_t wait_flag = 1;
volatile uint8_t prev_key = KEY_RIGHT;

// 간단한 소프트웨어 딜레이 (게임 속도용)
static void delay_loop(volatile uint32_t count)
{
    while (count--) {
        __NOP();
    }
}

static uint32_t TimeToSeconds(DS1302_Time_t t) {
    return (t.hour * 3600) + (t.minute * 60) + t.second;
}

// 시작 시간과 종료 시간의 차이(초 단위) 계산
static uint32_t Calculate_Duration_Sec(DS1302_Time_t start, DS1302_Time_t end) {
    uint32_t s_sec = TimeToSeconds(start);
    uint32_t e_sec = TimeToSeconds(end);

    if (e_sec >= s_sec) {
        return e_sec - s_sec;
    } else {
        // 자정을 넘긴 경우 (예: 23:59:50 시작 -> 00:00:10 종료)
        return (e_sec + 86400) - s_sec;
    }
}

// PIR 기반 절전 기능에서 사용할 사람 부재 시간(ms)
// 10초 이상 사람이 없으면 절전 모드 진입
#define PIR_IDLE_SLEEP_MS   10000U

// 블루투스 HEARTBEAT 주기(ms)
#define BT_HEARTBEAT_MS     5000U

// 게임 종료 시 결과 프레임 전송
// 포맷: RPL|yyyy-MM-dd HH:mm:ss|경과시간(초)|점수\r\n
static void BT_SendResultFrame(uint32_t duration_sec, int score)
{
    char buffer[128];

    // 실제 RTC가 없으므로 예시로 고정된 날짜/시간 사용
    // 필요하면 RTC 연동해서 실제 시각으로 교체
    int year  = 2025;
    int month = 11;
    int day   = 13;
    int hour  = 17;
    int min   = 0;
    int sec   = 0;

    sprintf(buffer,
            "RPL|%04d-%02d-%02d %02d:%02d:%02d|%lu|%d\r\n",
            year, month, day, hour, min, sec,
            (unsigned long)duration_sec,
            score);

    BT_SendString(buffer);
}

int main(void)
{
    SystemInit();

    // SysTick 1ms 설정
    SysTick_Config(SystemCoreClock / 1000);
    Sound_Init();
    Joystick_Init();  // Changed from Button_Init
    DS1302_Init();
    BT_Init();  
    Display_Init();
    PIR_Init();
    snake_setup();
    CDS_Init();

    SysTick_Config((SystemCoreClock / 1000));   // 1ms마다 Tick 발생

    // 난수 시드 (필요시 고정값)
    srand(1);

    static char grid[GRID_ROWS][GRID_COLS];

    // PIR 관련 시간 관리
    uint32_t nowMs          = PIR_GetMillis();
    uint32_t lastMotionTime = nowMs;   // 마지막으로 사람이 감지된 시점

    // 게임 시간/HEARTBEAT 관리
    uint32_t gameStartTimeMs  = nowMs; // 현재 게임 시작 시각
    uint32_t lastHeartbeatMs  = nowMs; // 마지막 HEARTBEAT 전송 시각

    // 게임 시간 측정용 변수
    DS1302_Time_t start_time, end_time;
    
    // [중요] 최초 게임 시작 시간 기록
    DS1302_GetTime(&start_time);

    while (1)
    {
        
        delay_loop(1000000);    // 대기 -> 시작 전환 사이에 대기 시간 반영
        while(wait_flag) {  // 대기 모드
            Display_Wait();
            KeyInput key = Joystick_GetInput();
            
            if (key != prev_key) {  // 새로운 키를 입력하면 게임 시작
                wait_flag = 0;
            }
        }

        while(!wait_flag) {


            // 공통 시간 값 (루프 시작 시점 기준)
            nowMs = PIR_GetMillis();

            // 0) PIR 센서 상태 확인 및 절전 모드 관리
            uint8_t pirPresent = PIR_IsPersonPresent();

            if (pirPresent) {
                // 사람이 감지되면 마지막 감지 시점을 갱신하고, 절전 중이면 해제
                lastMotionTime = nowMs;
                if (Display_IsSleep()) {
                    Display_SetSleep(0);
                }
            } else {
                // 사람이 없고, 일정 시간 이상 부재 상태가 지속되면 절전 모드 진입
                if (!Display_IsSleep()) {
                    if ((nowMs - lastMotionTime) >= PIR_IDLE_SLEEP_MS) {
                        Display_SetSleep(1);
                    }
                }
            }

            // 0-1) LED 제어 (Joystick 기능)
            Joystick_HandleInput();

            // 1) 조이스틱 입력 → 방향 변경
    // 버튼 입력 → 방향 변경
            KeyInput key = Joystick_GetInput(); // Changed from Button_GetInput
            if (key != KEY_NONE) {
                snake_set_direction(key);

                // 간단 디바운스 (원하면 유지)
                delay_loop(50000);
            }

            // ------------------------------------------
            // 2) 게임 상태 업데이트
            // ------------------------------------------
            if (snake_update() && !Display_IsSleep()) {
                // === GAME OVER 상황 ===
                game_over_sound();
                // 1. 종료 시간 기록
                DS1302_GetTime(&end_time);

                // 2. 플레이 시간(초) 계산
                uint32_t play_duration = Calculate_Duration_Sec(start_time, end_time);

                // 3. 블루투스 전송 (점수(길이), 경과 시간(초))
                // snake_get_score()가 길이를 반환한다고 가정
                BT_SendScoreFrame(snake_get_score(), play_duration);

                // 4. 게임 리셋
                snake_setup();
                
                // 5. 다음 게임을 위해 시작 시간 재설정
                DS1302_GetTime(&start_time);

                wait_flag = 1;
            }

            // ------------------------------------------
            // 3) 화면 렌더링
            // ------------------------------------------
            // 현재 상태를 char 그리드로 변환
            snake_to_grid(grid);

            // 그리드를 LCD에 출력 (절전 모드라면 내부에서 자동으로 skip)
            Display_DrawGrid(grid);

            // 5) 게임 중 HEARTBEAT 전송 (5초 주기)
            if ((nowMs - lastHeartbeatMs) >= BT_HEARTBEAT_MS) {
                // 게임 중 keep-alive 용 단순 문자열
                BT_SendString("HEARTBEAT\r\n");
                lastHeartbeatMs = nowMs;
            }

            // 6) 게임 속도 조절 (값 조정해서 원하는 스피드 맞추면 됨)
            delay_loop(10000);


            // ------------------------------------------
            // 4) 입력 센서 처리
            // ------------------------------------------

            // 조도 센서 인터럽트가 발생했으면 다크/라이트 모드 변경
            if(is_light_event_occured()) {
                set_dark_mode(get_dark_mode_flag());
                set_light_event_flag(0);
            }

        }
    }

    // 여기까지 도달하지 않음
    // return 0;
}
