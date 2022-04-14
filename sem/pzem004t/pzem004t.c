/*
 * CYANCORE LICENSE
 * Copyrights (C) 2019, Cyancore Team
 *
 * File Name		: pzem004t.c
 * Description		: This file consists of pzem004t driver prototypes
 * Primary Author	: Akash Kollipara [akashkollipara@gmail.com]
 * Organisation		: Cyancore Core-Team
 */

#include <stdint.h>
#include <stdio.h>
#include <stddev.h>
#include <status.h>
#include <string.h>
#include <syslog.h>
#include <driver.h>
#include <hal/clint.h>
#include "pzem004t.h"
#include "pzem004t_private.h"

static uint16_t __addr;

static status_t pzem_read(char *a)
{
	return fgetc(pdac, a);
}

static status_t pzem_write(const char a)
{
	return fputc(pdac, a);
}

static status_t pzem_rstream(char *a, uint8_t size)
{
	status_t ret = success;
	for(uint8_t i = 0; i < size; i++)
		ret |= pzem_read(&a[i]);
	return ret;
}

static status_t pzem_wstream(char *a, uint8_t size)
{
	status_t ret = success;
	for(uint8_t i = 0; i < size; i++)
		ret |= pzem_write(a[i]);
	return ret;
}

static uint16_t __read_crc_from_lut(char *a, uint8_t size)
{
	uint8_t index;
	uint16_t crc = 0xffff;
	for(uint8_t i = 0; i < size; i++)
	{
		index = a[i] ^ crc;
		crc >>= 8;
		crc ^= crc_lut[index];
	}
	return crc;
}

status_t pzem_verifyCRC(char *a, uint8_t size)
{
	uint16_t crc = __read_crc_from_lut(a, size - 2);
	uint8_t temp = 0;
	temp |= a[size-1] ^ HI8B(crc);
	temp |= a[size-2] ^ LO8B(crc);
	return temp ? error_generic : success;
}

static void pzem_updateCRC(char *a, uint8_t size)
{
	uint16_t crc = __read_crc_from_lut(a, size - 2);
	a[size-2] = LO8B(crc);
	a[size-1] = HI8B(crc);
}

static status_t pzem_sendCmd(uint8_t cmd, uint16_t reg_addr, uint16_t val, uint16_t rx_addr)
{
	status_t ret = success;
	uint8_t txBuff[8];
	uint8_t rxBuff[8];

	if((rx_addr == 0xffff) || (rx_addr < 0x1) || (rx_addr > 0xf7))
		rx_addr = __addr;

	txBuff[0] = rx_addr;
	txBuff[1] = cmd;
	txBuff[2] = HI8B(reg_addr);
	txBuff[3] = LO8B(reg_addr);
	txBuff[4] = HI8B(val);
	txBuff[5] = LO8B(val);
	pzem_updateCRC((char *)txBuff, 8);

	pzem_wstream((char *)txBuff, 8);

	pzem_rstream((char *)rxBuff, 8);

	if(memcmp(txBuff, rxBuff, 8))
		ret = error_device;
	return ret;

}

status_t pzem_resetEnergy(uint8_t rx_addr)
{
	uint8_t cmb_buff[] = {rx_addr, PZEM004TV3_C_RE_REG, 0, 0};
	uint8_t rxBuff[5];
	pzem_updateCRC((char *)cmb_buff, 4);
	pzem_wstream((char *)cmb_buff, 4);
	return pzem_rstream((char *)rxBuff, 5);
}

static un16_t V, pf, freq, alarms;
static un32_t I, E, P;
static uint8_t rx_addr;

void pzem_update(uint8_t addr)
{
	rx_addr = addr;
}

static status_t pzem_fetchSamples(uint8_t rx_addr)
{
	status_t ret;
	uint8_t rxBuff[25];
	static uint64_t prevTime = 0;

	if(clint_read_time() - prevTime < PZEM004TV3_UPDATE_TIME)
		return success;

	prevTime = clint_read_time();

	ret = pzem_sendCmd(PZEM004TV3_C_R_IP_REG, 0x00, 10, rx_addr);
	ret |= pzem_rstream((char *)rxBuff, 25);

	V.vb.b0 = rxBuff[4];
	V.vb.b1 = rxBuff[3];

	I.vb.b0 = rxBuff[6];
	I.vb.b1 = rxBuff[5];
	I.vb.b2 = rxBuff[8];
	I.vb.b3 = rxBuff[7];

	P.vb.b0 = rxBuff[10];
	P.vb.b1 = rxBuff[9];
	P.vb.b2 = rxBuff[12];
	P.vb.b3 = rxBuff[11];

	E.vb.b0 = rxBuff[14];
	E.vb.b1 = rxBuff[13];
	E.vb.b2 = rxBuff[16];
	E.vb.b3 = rxBuff[15];

	freq.vb.b0 = rxBuff[18];
	freq.vb.b1 = rxBuff[17];

	pf.vb.b0 = rxBuff[20];
	pf.vb.b1 = rxBuff[19];

	alarms.vb.b0 = rxBuff[22];
	alarms.vb.b1 = rxBuff[21];

	return ret;
}

float pzem_voltage(void)
{
	if(!pzem_fetchSamples(rx_addr))
		return (float)0.0;
	return (float)V.val / 10.0;
}

float pzem_current(void)
{
	if(!pzem_fetchSamples(rx_addr))
		return (float)0.0;
	return (float)I.val/1000.0;
}

float pzem_power(void)
{
	if(!pzem_fetchSamples(rx_addr))
		return (float)0.0;
	return (float)P.val/10.0;
}

float pzem_energy(void)
{
	if(!pzem_fetchSamples(rx_addr))
		return (float)0.0;
	return (float)E.val/1000.0;
}

float pzem_freq(void)
{
	if(!pzem_fetchSamples(rx_addr))
		return (float)0.0;
	return (float)freq.val/10.0;
}

float pzem_pf(void)
{
	if(!pzem_fetchSamples(rx_addr))
		return (float)0.0;
	return (float)pf.val/100.0;
}

float pzem_alarms(void)
{
	if(!pzem_fetchSamples(rx_addr))
		return (float)0.0;
	return (float)alarms.val;
}
