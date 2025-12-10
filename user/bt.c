// bt.c
#include "bt.h"
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

// 스네이크 게임용 점수 프레임 전송
void BT_SendScoreFrame(int score, uint32_t duration_ms)
{
    char buffer[128];
    char score_str[32];

    // duration_ms 를 mm:ss:mss로 변환
    uint32_t total_ms = duration_ms;
    uint32_t d_min  = (total_ms / 1000) / 60;
    uint32_t d_sec  = (total_ms / 1000) % 60;
    uint32_t d_msec = total_ms % 1000;

    // 테스트와 동일 포맷: "RPL|YYYY-MM-DD HH:MM:SS|mm:ss:mss|%s\r\n"
    // 날짜/시각은 RTC를 쓰지 않으므로 예시로 고정 값 사용 (원하면 수정 가능)
    // 마지막 필드에 SCORE=NN 형식으로 점수 삽입
    sprintf(score_str, "SCORE=%d", score);

    sprintf(buffer,
            "RPL|2025-01-01 00:00:00|%02lu:%02lu:%03lu|%s\r\n",
            (unsigned long)d_min,
            (unsigned long)d_sec,
            (unsigned long)d_msec,
            score_str);

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
