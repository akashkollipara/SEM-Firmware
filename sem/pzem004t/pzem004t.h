/*
 * CYANCORE LICENSE
 * Copyrights (C) 2019, Cyancore Team
 *
 * File Name		: pzem004t.h
 * Description		: This file consists of pzem004t driver prototypes
 * Primary Author	: Akash Kollipara [akashkollipara@gmail.com]
 * Organisation		: Cyancore Core-Team
 */

#pragma once

#define pzem004t_uart	0x102

void pzem_update(uint8_t addr);
float pzem_voltage(void);
float pzem_current(void);
float pzem_power(void);
float pzem_energy(void);
float pzem_freq(void);
float pzem_pf(void);
float pzem_alarms(void);
