/*
 * CYANCORE LICENSE
 * Copyrights (C) 2019, Cyancore Team
 *
 * File Name		: project.c
 * Description		: This file consists of project srouces
 * Primary Author	: Akash Kollipara [akashkollipara@gmail.com]
 * Organisation		: Cyancore Core-Team
 */

#include <status.h>
#include <stdio.h>
#include <string.h>
#include <terravisor/bootstrap.h>
#include <arch.h>
#include <driver.h>
#include <interrupt.h>
#include <mmio.h>
#include <driver/sysclk.h>
#include <hal/clint.h>
#include <hal/gpio.h>
#include "pzem004t/pzem004t.h"

static unsigned long long t;
static unsigned int ticks = 16384;
static gpio_port_t bled;

static void test()
{
	arch_di_mtime();
	t = clint_read_time();
	clint_config_tcmp(0, (t + ticks));
	arch_ei_mtime();
	gpio_pin_toggle(&bled);
}


void plug()
{
	bootstrap();
	driver_setup_all();
	link_interrupt(int_local, 7, test);

	gpio_pin_alloc(&bled, PORTA, 5);
	gpio_pin_mode(&bled, out);
	gpio_pin_clear(&bled);

	pzem_update(0x00);

	t = clint_read_time();
	clint_config_tcmp(0, (t + ticks));
	arch_ei_mtime();

	return;
}

void play()
{
	printf("Voltage: %u", (uint16_t)pzem_voltage());
	arch_wfi();
	return;
}
