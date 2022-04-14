/*
 * CYANCORE LICENSE
 * Copyrights (C) 2022, Cyancore Team
 *
 * File Name		: pzem004t_serial.c
 * Description		: This file contains sources of pzem00t4 uart i/f
 * Primary Author	: Akash Kollipara [akashkollipara@gmail.com]
 * Organisation		: Cyancore Core-Team
 */

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <status.h>
#include <syslog.h>
#include <lock/spinlock.h>
#include <resource.h>
#include <machine_call.h>
#include <arch.h>
#include <driver.h>
#include <interrupt.h>
#include <hal/uart.h>
#include <hal/gpio.h>
#include <driver/sysclk.h>
#include "pzem004t.h"

FILE *pdac;
static FILE dev;

static uart_port_t pzem_port;
static gpio_port_t io[2];

static void pzem004t_serial_irq_handler(void);

static status_t pzem004t_serial_setup(void)
{
	uart_get_properties(&pzem_port, pzem004t_uart);
	pzem_port.irq_handler = pzem004t_serial_irq_handler;

	sysdbg2("UART engine @ %p\n", pzem_port.baddr);
	sysdbg2("UART baud @ %lubps\n", pzem_port.baud);
	sysdbg2("UART irqs - %u\n", pzem_port.irq);
	return uart_setup(&pzem_port, trx, no_parity);
}

static status_t pzem004t_serial_write(const char c)
{
	status_t ret;
	while(!uart_buffer_available(&pzem_port));
	ret = uart_tx(&pzem_port, c);
	return ret;
}

static int_wait_t con_read_wait;
static char con_buff[32];
static uint8_t wp, rp, occ;


static status_t pzem004t_serial_read(char *c)
{
	status_t ret = success;
	if(!occ)
	{
		ret = wait_lock(&con_read_wait);
		ret |= wait_till_irq(&con_read_wait);
	}

	*c = con_buff[(wp++)];
	wp = wp % 32;
	occ--;

	return ret;
}

static void pzem004t_serial_irq_handler(void)
{
	if(uart_rx_pending(&pzem_port))
	{
		wait_release_on_irq(&con_read_wait);
		while(uart_rx_pending(&pzem_port))
		{
			uart_rx(&pzem_port, &con_buff[(rp++)]);
			rp = rp % 32;
			occ++;
		}
	}
}

static status_t pzem004t_serial_pre_clk_config(void)
{
	return success;
}

static status_t pzem004t_serial_post_clk_config(void)
{
	uart_update_baud(&pzem_port);
	return success;
}

static sysclk_config_clk_callback_t bt_handle =
{
	.pre_config = &pzem004t_serial_pre_clk_config,
	.post_config = &pzem004t_serial_post_clk_config
};

status_t pzem004t_serial_driver_setup(void)
{
	status_t ret;
	ret = pzem004t_serial_setup();
	ret |= sysclk_register_config_clk_callback(&bt_handle);
	dev.write = &pzem004t_serial_write;
	dev.read = &pzem004t_serial_read;
	pdac = &dev;
	return ret;
}

status_t pzem004t_serial_driver_exit(void)
{
	status_t ret;
	ret = sysclk_deregister_config_clk_callback(&bt_handle);
	ret |= uart_shutdown(&pzem_port);
	for(uint8_t i = 0; i < pzem_port.pmux->npins; i++)
	{
		ret |= gpio_disable_alt_io(&io[i]);
		ret |= gpio_pin_free(&io[i]);
	}
	return ret;
}

INCLUDE_DRIVER(pzem, pzem004t_serial_driver_setup, pzem004t_serial_driver_exit, 0, 255, 255);
