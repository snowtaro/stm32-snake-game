// pir.c
#include "pir.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

static volatile uint32_t pir_msTicks = 0;


uint32_t PIR_GetMillis(void)
{
    return pir_msTicks;
}

void PIR_Init(void)
{
    // GPIOC 클럭 활성화
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_StructInit(&GPIO_InitStructure);

    // PC5 를 입력(Pull-down)으로 사용
    // DYP-ME003 모듈 출력이 HIGH = 감지, LOW = 미감지라고 가정
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPD; // Input pull-down
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // 1ms SysTick 설정 (SystemCoreClock 은 SystemInit 이후 설정되어 있다고 가정)
    SysTick_Config(SystemCoreClock / 1000);
}

uint8_t PIR_IsPersonPresent(void)
{
    // PC5 가 HIGH 이면 사람/움직임 감지
    if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_5) != Bit_RESET) {
        return 1;
    }
    return 0;
}
