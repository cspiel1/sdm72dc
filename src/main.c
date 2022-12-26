/**
 * @file main.c  sdm72dc main program
 *
 * Copyright (C) 2022 Christian Spielberger
 */
#include <stdio.h>
#include <unistd.h>
#include <modbus.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <asm/ioctls.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>

#include "util.h"
#include "conf.h"
#include "reg.h"
#include "mqtt.h"

static bool run = false;

static void signal_handler(int sig)
{
	run = false;
}


static void print_usage(const char *pname)
{
	printf("Usage:\n%s [options] [address]\n"
		"Options:\n"
		"-d, --daemon\n"
		"       Start daemon that polls input registers and "
			"sends to MQTT broker.\n"
		"       If broker is not configured the values are printed to "
			"stdout.\n"
		"\n"
		"-n, --nomodbus\n"
		"       For debugging/valgrind. No modbus connection is "
			"opened. \n"
		"\n"
		"-D, --dev device\n"
		"-i, --id slave\n"
		"-r, --reset Reset total energie counter\n"
		, pname);
}


int main(int argc, char *argv[])
{
	int ret = 0;
	modbus_t *ctx = NULL;
	int addr = -1;
	struct fconf conf;
	char fname[255];
	const char *dev = NULL;
	int slave = 0;
	bool nomodbus = false;
	int err;
	bool reset = false;

	memset(&conf, 0, sizeof(conf));
	sprintf(fname, "%s/.config/sdm72dc", getenv("HOME"));
	err = read_fconfig(&conf, fname);
	if (err)
		return err;

	while (1) {
		int c;
		static struct option long_options[] = {
			{"nomodbus",  0, 0, 'n'},
			{"daemon",    0, 0, 'd'},
			{"dev",       1, 0, 'D'},
			{"id",        1, 0, 'i'},
			{"reset",     0, 0, 'r'},
			{0,           0, 0,   0}
		};

		c = getopt_long(argc, argv, "ndD:i:r", long_options, NULL);
		if (c == -1)
			break;

		switch (c) {

			case 'd':
				run = true;
				break;

			case 'n':
				nomodbus = true;
				break;

			case 'r':
				reset = true;
				break;

			case 'i':
				slave = atoi(optarg);
				break;

			case 'D':
				dev = optarg;
				break;
			default:
				print_usage(argv[0]);
				return EINVAL;
		}
	}

	if (optind < argc) {
		if (strchr(argv[optind], 'x'))
			sscanf(argv[optind], "0x%02x", &addr);
		else
			addr = atoi(argv[optind]);
	}

	if (!dev)
		dev = conf.ttydev;

	if (!slave)
		slave = conf.slaveid;

	printf("Connecting to %s modbus slave %d stopbits %d\n",
	       dev, slave, conf.stopbits);

	if (!nomodbus) {
		ctx = modbus_new_rtu(dev, conf.baudrate, 'N', 8, conf.stopbits);
		if (ctx == NULL) {
			perror("Unable to create the libmodbus context\n");
			goto out;
		}

		/*        ret = modbus_set_debug(ctx, TRUE);*/

		ret = modbus_rtu_set_serial_mode(ctx, MODBUS_RTU_RS232);
		if(ret < 0){
			perror("modbus_rtu_set_serial_mode error\n");
			goto out;
		}

		ret = modbus_connect(ctx);
		if(ret < 0){
			perror("modbus_connect error\n");
			goto out;
		}

		ret = modbus_set_slave(ctx, slave);
		if(ret < 0) {
			perror("modbus_set_slave error\n");
			goto out;
		}

		if (reset)
			(void)reset_energie(ctx);
	}

	(void)signal(SIGINT, signal_handler);
	(void)signal(SIGALRM, signal_handler);
	(void)signal(SIGTERM, signal_handler);

	if (run) {
		bool mqtt = conf.mqtthost != NULL;
		uint64_t ms = tmr_jiffies();

		if (mqtt) {
			err = fmqtt_init(&conf);
			if (err)
				goto out;
		}

		while (run) {
			if (mqtt)
				err = fmqtt_loop();

			if (err)
				break;

			if (tmr_jiffies() > ms) {
				ms += 5000;

				/* overrun? */
				if (ms < tmr_jiffies())
					ms = tmr_jiffies() + 5000;

				err = publish_registers(ctx, &conf);

				check_reset(ctx, &conf);
			}

			if (err)
				break;

			if (usleep(10000)==-1)
				break;
		}

		run = false;
		if (mqtt)
			fmqtt_close();
	}
	else if (addr >= 0) {
		err = print_register(ctx, addr);
	}
	else {
		err = print_all_registers(ctx);
	}

out:
	modbus_close(ctx);
	modbus_free(ctx);

	free_fconfig(&conf);

	if (!err && ret < 0)
		err = EIO;

	return err;
}
