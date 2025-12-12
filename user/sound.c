#include "stm32f10x.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"
#include "misc.h"
#include "sound.h"

#define TIM_CLK 1000000
#define SOUND_QUEUE_SIZE 8

typedef struct {
    uint32_t freq;
    uint32_t duration;
} SoundItem;

// ------------- 사운드 큐 ---------------
static SoundItem sound_queue[SOUND_QUEUE_SIZE];
static uint8_t queue_head = 0;
static uint8_t queue_tail = 0;

static volatile uint32_t buzzer_time = 0;

void SND_RCC_Configure(void)
{  
    /* GPIOB 포트 활성화 */    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
}

void SND_GPIO_Configure(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // PB0 : PWM 출력
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;   // PWM
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void SND_TIM_Configure(void)
{
    TIM_TimeBaseInitTypeDef TIM3_InitStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    /* 1MHz(1us tick) 타이머 클럭 만들기 */
    uint16_t prescaler = (uint16_t)(SystemCoreClock / TIM_CLK);

    /* 1) 타이머 기본 설정 먼저 */
    TIM3_InitStructure.TIM_Period = 50000;     // 200Hz PWM
    TIM3_InitStructure.TIM_Prescaler = prescaler;
    TIM3_InitStructure.TIM_ClockDivision = 0;
    TIM3_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM3_InitStructure);

    /* 2) PWM 채널 설정 */
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_Pulse = 0; 
    TIM_OC3Init(TIM3, &TIM_OCInitStructure);

    /* Preload는 반드시 ENABLE */
    TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM3, ENABLE);

    /* 타이머 시작 */
    TIM_Cmd(TIM3, ENABLE);
}


// 사운드 모듈 초기 설정
void Sound_Init(void) {
    SND_RCC_Configure();
    SND_GPIO_Configure();
    SND_TIM_Configure();
}

// 주파수 설정
void set_freq(uint32_t freq) {
    if(freq == 0) {
        TIM3->CCR3 = 0;  
        return;
    }

    uint32_t period = TIM_CLK / freq;
    TIM3->ARR = period;
    TIM3->CCR3 = period / 2;    // 50% duty - 소리 on
    TIM3->EGR = TIM_EGR_UG;
}

// ------------ 큐 삽입 -----------------
void enqueue(uint32_t freq, uint32_t duration) {
    uint8_t next_tail = (queue_tail + 1) % SOUND_QUEUE_SIZE;
    if(next_tail == queue_head) return;     // 큐가 가득 찼으면 무시
    sound_queue[queue_tail].freq = freq;
    sound_queue[queue_tail].duration = duration;
    queue_tail = next_tail;
}

// 소리 출력 - Hz와 시간을 설정하면 출력
void sound(uint32_t Hz, uint32_t duration_ms) {
    enqueue(Hz, duration_ms);
}



// -------------- Tick 처리 ------------------

// SysTick_Handler에서 처리 - 부저 시간이 0이면 소리 off
void Sound_SysTick_Handler(void) {
    if(buzzer_time > 0) {
        buzzer_time--;
        if(buzzer_time == 0) {
            TIM3->CCR3 = 0;     // 0% duty - 소리 off
        }
    }

    // 현재 소리가 없으면 큐에서 꺼내 재생
    if(buzzer_time == 0 && queue_head != queue_tail) {
        SoundItem next = sound_queue[queue_head];
        queue_head = (queue_head + 1) % SOUND_QUEUE_SIZE;

        set_freq(next.freq);            // Hz 설정
        buzzer_time = next.duration;    // 시간 설정
    }
}



