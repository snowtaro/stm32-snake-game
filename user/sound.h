// sound.h 
#ifndef __SOUND_H
#define __SOUND_H

void SND_RCC_Configure(void);
void SND_GPIO_Configure(void);
void SND_TIM_Configure(void);
void Sound_Init(void);
void Sound_SysTick_Handler(void);
void sound(uint32_t, uint32_t);

#endif