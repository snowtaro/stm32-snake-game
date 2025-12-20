// bt.c
#include "bt.h"
#include "snake.h"
#include "ds1302.h"
#include <stdio.h>

// 내부 사용 함수 정의
static void RCC_Configure(void);
static void GPIO_Configure(void);
static void USART1_Init(void);
static void USART2_Init(void);
static void NVIC_Configure(void);

static void send_USART1(uint16_t data);
static void send_USART2(uint16_t data);

// RTC 초기 세팅(1회) 보장 함수
static void RTC_EnsureInit(void);

// ==============================
// Public API
// ==============================

void BT_Init(void)
{
    SystemInit();
    RCC_Configure();
    GPIO_Configure();
    USART1_Init();
    USART2_Init();
    NVIC_Configure();

    DS1302_Init();
    RTC_EnsureInit(); // ★ 추가: 처음 한 번만 시간 세팅
}

// 문자열 전체 전송 (USART2 = Bluetooth)
void BT_SendString(const char *s)
{
    while (*s)
    {
        send_USART2((uint16_t)(*s));
        s++;
    }
}

// SysTick 기반 간단 ms 딜레이
void BT_DelayMs(uint32_t ms)
{
    // SysTick 1ms tick 가정 (SystemCoreClock 기준)
    SysTick_Config(SystemCoreClock / 1000);

    for (uint32_t i = 0; i < ms; i++)
    {
        while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0)
            ;
    }

    SysTick->CTRL = 0;
}

void BT_SendScoreFrame(int snake_length, uint32_t duration_ms)
{
    char buffer[128];
    uint32_t total_sec = duration_ms / 1000;

    DS1302_Time_t now;
    DS1302_GetTime(&now);

    sprintf(buffer,
            "RPL|20%02d-%02d-%02d %02d:%02d:%02d|%lu|%d\r\n",
            now.year, now.month, now.day,
            now.hour, now.minute, now.second,
            (unsigned long)total_sec,
            snake_length);

    BT_SendString(buffer);
}

// ==============================
// 내부 구현부
// ==============================

// DS1302 RAM을 이용해서 "최초 1회만" 시간 설정
static void RTC_EnsureInit(void)
{
    uint8_t magic = 0x00;
    DS1302_ReadRAM(0, &magic, 1);

    if (magic != 0xA5)
    {
        // ★ 여기 시간을 네가 원하는 "현재"로 바꿔서 한 번 세팅하면 됨
        // year는 20xx에서 xx만 (예: 2025 -> 25)
        DS1302_Time_t t;
        t.year   = 25;
        t.month  = 12;
        t.day    = 20;
        t.week   = 6;   // (예시) 토요일=6 이런 식, 네 기준에 맞게
        t.hour   = 16;
        t.minute = 28;
        t.second = 0;

        DS1302_SetTime(&t);

        magic = 0xA5;
        DS1302_WriteRAM(0, &magic, 1);
    }
}

static void RCC_Configure(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |
                           RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_GPIOC |
                           RCC_APB2Periph_GPIOD |
                           RCC_APB2Periph_AFIO  |
                           RCC_APB2Periph_USART1, ENABLE);

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
}

static void GPIO_Configure(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // USART1 TX (PA9), RX (PA10)
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // USART2 TX (PA2), RX (PA3) - Bluetooth
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

static void USART1_Init(void)
{
    USART_InitTypeDef USART_InitStructure;

    USART_StructInit(&USART_InitStructure);
    USART_InitStructure.USART_BaudRate            = 115200;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(USART1, &USART_InitStructure);
    USART_Cmd(USART1, ENABLE);
}

static void USART2_Init(void)
{
    USART_InitTypeDef USART_InitStructure;

    USART_StructInit(&USART_InitStructure);
    USART_InitStructure.USART_BaudRate            = 9600;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(USART2, &USART_InitStructure);
    USART_Cmd(USART2, ENABLE);
}

static void NVIC_Configure(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

static void send_USART1(uint16_t data)
{
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
        ;
    USART_SendData(USART1, data & 0xFF);
}

static void send_USART2(uint16_t data)
{
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
        ;
    USART_SendData(USART2, data & 0xFF);
}
