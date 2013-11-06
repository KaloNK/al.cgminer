#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "tm_i2c.h"
#include "spidevc.h"

#include "bitfury-config.h"

#ifndef BITFURY_MUX_OE

//#define BF_OE_ACTIVE_LOW

#if (BITFURY_MAXBANKS > 1)
//static const int bf_bank_gpio[BITFURY_MAXBANKS] = {};     // Define which gpio pins are used for OE instead of MUX

static const int bf_bank_gpio[BITFURY_MAXBANKS] = {18,23,24,25};       // BFSB master boards V1-V3 ?

//static const int bf_bank_gpio[BITFURY_MAXBANKS] = {4,17,18,22,23,24,25};       // All possible RPi P1 pins - GPIO without special functionality both revisions
//static const int bf_bank_gpio[BITFURY_MAXBANKS] = {4,17,18,22,23,24,25,21};    // All possible RPi P1 pins - GPIO without special functionality Revision 1 header pin 13 is 21
//static const int bf_bank_gpio[BITFURY_MAXBANKS] = {4,17,18,22,23,24,25,27};    // All possible RPi P1 pins - GPIO without special functionality Revision 2 header pin 13 is 27
//static const int bf_bank_gpio[BITFURY_MAXBANKS] = {28,29,30,31};               // All possible RPi P5 pins - Revision 2 only
#endif	// BITFURY_MAXBANKS > 1
#endif

int tm_i2c_init() {
	unsigned char slot;

	if ((tm_i2c_fd = open("/dev/i2c-1", O_RDWR)) < 0) {
		return 1;
	}

#if (!defined(BITFURY_MUX_OE) && (BITFURY_MAXBANKS > 1))
	for ( slot = 0; slot < BITFURY_MAXBANKS; slot++) {
		tm_i2c_clear_oe(slot); // do not set them to Outs yet
	}
#endif

	return 0;
}

void tm_i2c_close() {
	close(tm_i2c_fd);
}

unsigned int tm_i2c_req(unsigned char slot, unsigned char cmd, unsigned int data) {
	int i;
	unsigned char buf[16];
	struct i2c_msg msg;
	tm_struct *tm = (tm_struct *) buf;
	struct i2c_rdwr_ioctl_data msg_rdwr;
	unsigned int ret;

	tm->cmd = cmd;
	tm->data_lsb = data & 0xFF;
	tm->data_msb = (data & 0xFF00) >> 8;

	/* Write CMD */
	msg.addr = (TM_ADDR >> 1) + slot;
	msg.flags = 0;
	msg.len = 3;
	msg.buf = buf;
	msg_rdwr.msgs = &msg;
	msg_rdwr.nmsgs = 1;
	if ((i = ioctl(tm_i2c_fd, I2C_RDWR, &msg_rdwr)) < 0) {
		return -1;
	}

	/* Read result */
	msg.addr = (TM_ADDR >> 1) + slot;
	msg.flags = I2C_M_RD;
	msg.len = 3;
	msg.buf = buf;
	msg_rdwr.msgs = &msg;
	msg_rdwr.nmsgs = 1;
	if ((i = ioctl(tm_i2c_fd, I2C_RDWR, &msg_rdwr)) < 0) {
		return -1;
	}

	//hexdump(buf, 10);
	ret = (tm->data_msb << 8) + tm->data_lsb;
	if (tm->cmd == cmd) return ret;
	return 0;
}

int tm_i2c_detect(unsigned char slot) {
	if (slot < 0 || slot > BITFURY_MAXBANKS - 1) return -1;
#ifdef BITFURY_MUX_OE
	return tm_i2c_req(slot, TM_GET_CORE0, 0);
#elif(BITFURY_MAXBANKS > 1)
	// Init GPIO as output on detect and SET it
	gpio_to_out(bf_bank_gpio[slot]);
	tm_i2c_set_oe(slot);
#endif
	return 0;
}

void tm_i2c_set_oe(unsigned char slot) {
	if (slot < 0 || slot > BITFURY_MAXBANKS - 1) return;
#ifdef BITFURY_MUX_OE
	tm_i2c_req(slot, TM_SET_OE, 0);
#elif(BITFURY_MAXBANKS > 1)
#ifdef BF_OE_ACTIVE_LOW
	gpio_clr(bf_bank_gpio[slot]);
#else
	gpio_set(bf_bank_gpio[slot]);
#endif	// BF_OE_ACTIVE_LOW
#endif
	return;
}

void tm_i2c_clear_oe(unsigned char slot) {
	if (slot < 0 || slot > BITFURY_MAXBANKS - 1) return;
#ifdef BITFURY_MUX_OE
	tm_i2c_req(slot, TM_SET_OE, 1);
#elif(BITFURY_MAXBANKS > 1)
#ifdef BF_OE_ACTIVE_LOW
	gpio_set(bf_bank_gpio[slot]);
#else
	gpio_clr(bf_bank_gpio[slot]);
#endif	// BF_OE_ACTIVE_LOW
#endif
	return;
}

#if 0 // unused

float tm_i2c_getcore0(unsigned char slot) {
	if (slot < 0 || slot > BITFURY_MAXBANKS - 1) return 0;
#ifdef BITFURY_MUX_OE
	return tm_i2c_Data2Core(tm_i2c_req(slot, TM_GET_CORE0, 0));
#endif
	return 0;
}

float tm_i2c_getcore1(unsigned char slot) {
	if (slot < 0 || slot > BITFURY_MAXBANKS - 1) return 0;
#ifdef BITFURY_MUX_OE
	return tm_i2c_Data2Core(tm_i2c_req(slot, TM_GET_CORE1, 0));
#endif
	return 0;
}

float tm_i2c_gettemp(unsigned char slot) {
	if (slot < 0 || slot > BITFURY_MAXBANKS - 1) return 0;
#ifdef BITFURY_MUX_OE
	return tm_i2c_Data2Temp(tm_i2c_req(slot, TM_GET_TEMP, 0));
#endif
	return 0;
}

float tm_i2c_Data2Temp(unsigned int ans) {
	float t = ans;
	return (t / 1023.0 * 3.3 * 2-2.73) * 100.0;
}

float tm_i2c_Data2Core(unsigned int ans) {
	float t = ans;
	return t / 1023.0 * 3.3;
}

#endif
