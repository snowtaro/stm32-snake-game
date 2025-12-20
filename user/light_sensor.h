//
#ifndef __LIGHT_SENSOR_H
#define __LIGHT_SENSOR_H

void RCC_Configure(void);
void GPIO_Configure(void);
void EXTI_Configure(void);
void NVIC_Configure(void);

uint8_t get_dark_mode_flag(void);
uint8_t is_light_event_occured(void);
void set_light_event_flag(uint8_t);

#endif