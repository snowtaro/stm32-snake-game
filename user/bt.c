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
    // SysTick 1ms tick 가정 (72MHz 기준)
    SysTick_Config(SystemCoreClock / 1000);

    for (uint32_t i = 0; i < ms; i++)
    {
        // COUNTFLAG가 1 될 때까지 대기
        while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0)
            ;
    }

    // SysTick 비활성화
    SysTick->CTRL = 0;
}

void BT_SendScoreFrame(int snake_length, uint32_t duration_ms)
{
    char buffer[128];

    // 1. 밀리초(ms)를 단순 초(sec)로 변환
    uint32_t total_sec = duration_ms / 1000;

    // 2. DS1302에서 현재 시간 가져오기
    DS1302_Time_t now;
    DS1302_GetTime(&now);

    // 3. 포맷팅: RPL|날짜 시간|경과초|SCORE=길이
    // 예: "RPL|2025-01-01 12:30:00|125|SCORE=15" (125초 경과, 길이 15)
    sprintf(buffer,
            "RPL|20%02d-%02d-%02d %02d:%02d:%02d|%lu|SCORE=%d\r\n",
            now.year, now.month, now.day,
            now.hour, now.minute, now.second,
            (unsigned long)total_sec, // 단순히 초 단위 정수만 출력
            snake_length);

    // 4. 전송
    BT_SendString(buffer);
}

// ==============================
// 내부 구현부
// ==============================

static void RCC_Configure(void)
{
    // GPIOA, B, C, D, AFIO, USART1, USART2 클럭 활성화
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
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9; // TX
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10; // RX
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // USART2 TX (PA2), RX (PA3) - Bluetooth
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_2; // TX
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3; // RX
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

    // RX interrupt 필요 시 활성화
    // USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    USART_Cmd(USART1, ENABLE);
}

static void USART2_Init(void)
{
    USART_InitTypeDef USART_InitStructure;

    USART_StructInit(&USART_InitStructure);
    USART_InitStructure.USART_BaudRate            = 9600;  // BT 모듈 설정에 맞게 조정
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(USART2, &USART_InitStructure);

    // RX interrupt 필요 시 활성화
    // USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

    USART_Cmd(USART2, ENABLE);
}

static void NVIC_Configure(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    // USART1 IRQ (필요시 사용)
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // USART2 IRQ (필요시 사용)
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

// 필요하면 USART1/2 IRQHandler도 여기서 구현 가능
/*
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        uint16_t data = USART_ReceiveData(USART1);
        // TODO: 수신 처리
    }
}

void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        uint16_t data = USART_ReceiveData(USART2);
        // TODO: 수신 처리
    }
}
*/
