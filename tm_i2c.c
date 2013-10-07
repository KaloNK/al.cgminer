#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "tm_i2c.h"

#include "bitfury-config.h"

int tm_i2c_init() {
	if ((tm_i2c_fd = open("/dev/i2c-1", O_RDWR)) < 0)
		return 1;
	else
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
#ifdef BITFURY_MUX_OE
	if (slot < 0 || slot > BITFURY_MAXBANKS - 1) return 0;
	return tm_i2c_req(slot, TM_GET_CORE0, 0);
#else
	return 0;
#endif
}

void tm_i2c_set_oe(unsigned char slot) {
#ifdef BITFURY_MUX_OE
	if (slot < 0 || slot > BITFURY_MAXBANKS - 1) return;
	tm_i2c_req(slot, TM_SET_OE, 0);
#else
	return;
#endif
}

void tm_i2c_clear_oe(unsigned char slot) {
#ifdef BITFURY_MUX_OE
	if (slot < 0 || slot > BITFURY_MAXBANKS - 1) return;
	tm_i2c_req(slot, TM_SET_OE, 1);
#else
	return;
#endif
}

#if 0 // unused

float tm_i2c_getcore0(unsigned char slot) {
#ifdef BITFURY_MUX_OE
	if (slot < 0 || slot > BITFURY_MAXBANKS - 1) return 0;
	return tm_i2c_Data2Core(tm_i2c_req(slot, TM_GET_CORE0, 0));
#else
	return 0;
#endif
}

float tm_i2c_getcore1(unsigned char slot) {
#ifdef BITFURY_MUX_OE
	if (slot < 0 || slot > BITFURY_MAXBANKS - 1) return 0;
	return tm_i2c_Data2Core(tm_i2c_req(slot, TM_GET_CORE1, 0));
#else
	return 0;
#endif
}

float tm_i2c_gettemp(unsigned char slot) {
#ifdef BITFURY_MUX_OE
	if (slot < 0 || slot > BITFURY_MAXBANKS - 1) return 0;
	return tm_i2c_Data2Temp(tm_i2c_req(slot, TM_GET_TEMP, 0));
#else
	return 0;
#endif
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
