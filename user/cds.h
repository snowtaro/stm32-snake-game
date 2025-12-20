//
#ifndef __CDS_H
#define __CDS_H

#include "misc.h"

void CDS_Init(void);

uint8_t get_dark_mode_flag(void);
uint8_t is_light_event_occured(void);
void set_light_event_flag(uint8_t);

#endif