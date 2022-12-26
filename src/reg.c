/**
 * @file reg.c  SDM72D register reading and formatted printing
 *
 * Copyright (C) 2022 Christian Spielberger
 */
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <modbus.h>

#include "util.h"
#include "conf.h"
#ifdef USE_MQTT
#include "mqtt.h"
#endif
#include "ieee754.h"
#include "reg.h"

enum {
	VALSIZE    = 32,
	VALPOS     = 64,
};


struct sdm_register {
	uint16_t address;     /* modbus address                              */
	const char *desc;     /* description                                 */
	const char *unit;     /* unit of the value                           */
	bool input;           /* input register flag                         */
};


#ifdef SDM630
struct sdm_register registers[] = {
{0x06,  "Phase 1 current", "A", true},
{0x08,  "Phase 2 current", "A", true},
{0x0a,  "Phase 3 current", "A", true},
{0x0c,  "Phase 1 power", "W", true},
{0x0e,  "Phase 2 power", "W", true},
{0x10,  "Phase 3 power", "W", true},
{0x12,  "Phase 1 apparent power", "VA", true},
{0x14,  "Phase 2 apparent power", "VA", true},
{0x16,  "Phase 3 apparent power", "VA", true},
};
#else
struct sdm_register registers[] = {
{0x34,  "Total system power", "W", true},
{0x0156,  "Total energy", "kWh", true},
{0x0180,  "Resetable total energy", "kWh", true},

{0x14,  "Modbus address", NULL, false},
{0x18,  "Password", NULL, false},
{0x1c,  "Baud rate index", NULL, false},
};
#endif

typedef void (print_reg_h)(struct sdm_register *reg, uint16_t *val, void *arg);


#ifdef USE_MQTT
static bool use_mqtt(const struct fconf *conf)
{
	if (!conf)
		return false;

	if (conf->mqtthost)
		return true;

	return false;
}
#endif


static const char *print_subline(const char *txt)
{
	size_t sz = VALPOS - 3;
	const char *p = txt + sz;

	for (; p > txt && p[0]!=' '; --p)
		--sz;

	printf("%.*s\n", (int) sz, txt);
	sz = strlen(p);
	if (sz > 1024) {
		printf("ERR - description to long\n");
		return NULL;
	}

	if (sz > VALPOS - 3)
		return print_subline(p);
	else
		return p;
}


static void convert_float(char *txt,
			const struct sdm_register *reg, uint16_t *val)
{
	uint32_t v0 = val[0];
	uint32_t v;

	v = (v0 << 16) + val[1];

	sprintf(txt, "%f", ieee754_to_float(v));
}


static void print_reg(struct sdm_register *reg, uint16_t *val, void *arg)
{
	char fill[VALPOS];
	char vtxt[VALSIZE * 2 + 1];
	const char *p;
	size_t sz;
	(void)arg;

	convert_float(vtxt, reg, val);
	if (reg->unit) {
		strcat(vtxt, " ");
		strcat(vtxt, reg->unit);
	}

	printf("%04u : ", reg->address);
	p = reg->desc;
	sz = strlen(p);
	if (sz > VALPOS - 3) {
		p = print_subline(reg->desc);
		sz = strlen(p);
		printf("       ");
	}

	memset(fill, 0, sizeof(fill));
	memset(fill, ' ', VALPOS - sz);
	printf("%s%s%s\n", p, fill, vtxt);
}


static int print_register_range(modbus_t *ctx, uint32_t begin, uint32_t end)
{
	size_t n = ARRAY_SIZE(registers);
	uint32_t addr = UINT16_MAX;
	uint16_t val[VALSIZE];
	bool found = false;
	int err = 0;

	for (size_t i = 0; i < n; ++i) {
		struct sdm_register *reg = &registers[i];
		int mret = 0;

		if (!found && reg->address < begin)
			continue;

		if (reg->address >= end)
			break;

		found = true;
		if (reg->address != addr) {
			addr = reg->address;
			memset(val, 0, sizeof(val));

			if (reg->input) {
				mret = modbus_read_input_registers(
						ctx, addr, 2, val);
			}
			else {
				mret = modbus_read_registers(
						ctx, addr, 2, val);
			}

			if (ctx && mret == -1) {
				perror("ERR - modbus read error\n");
				err = EPROTO;
				break;
			}
		}

		print_reg(reg, val, NULL);
	}

	return err;
}


int print_all_registers(modbus_t *ctx)
{
	return print_register_range(ctx, 0, UINT16_MAX);
}


static int print_register_priv(modbus_t *ctx, int addr,
			       print_reg_h *ph, void *arg)
{
	size_t n = ARRAY_SIZE(registers);
	uint16_t val[VALSIZE];
	int mret;
	int err = 0;

	for (size_t i = 0; i < n; ++i) {
		struct sdm_register *reg = &registers[i];
		if (reg->address != addr)
			continue;

		mret = modbus_read_registers(ctx, addr, 2, val);
		if (ctx && mret == -1) {
			perror("ERR - modbus read error\n");
			err = EPROTO;
			break;
		}

		ph(reg, val, arg);
	}

	return err;
}


int print_register(modbus_t *ctx, int addr)
{
	return print_register_priv(ctx, addr, print_reg, NULL);
}


#ifdef USE_MQTT
static void publish_reg_handler(struct sdm_register *reg, uint16_t *val,
				void *arg)
{
	const char *topic = arg;
	char vtxt[VALSIZE * 2 + 1];

	convert_float(vtxt, reg, val);

	fmqtt_publish(topic, vtxt);
}
#endif


int publish_registers(modbus_t *ctx, struct fconf *conf)
{
	int err = 0;

	for (size_t i = 0; i < ARRAY_SIZE(conf->publish); ++i) {
		int addr;
		char topic[41];
		int n;

		if (!conf->publish[i])
			break;

		n = sscanf(conf->publish[i], "0x%02x %40s", &addr, topic);
		if (n != 2) {
			printf("ERR - could not parse publish%u \"%s\" \n",
			       (uint8_t) i, conf->publish[i]);
			err = EINVAL;
			break;
		}

		err = print_register_priv(ctx, addr,
#ifdef USE_MQTT
			  (use_mqtt(conf) ? publish_reg_handler : print_reg),
			  topic
#else
			  print_reg, NULL
#endif
			  );
		if (err) {
			printf("ERR - Could'nt read register %d\n", addr);
			break;
		}
	}

	return err;
}
