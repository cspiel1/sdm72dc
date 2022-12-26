/**
 * @file reg.h  SDM72D register API
 *
 * Copyright (C) 2022 Christian Spielberger
 */
#ifndef FREG_H
#define FREG_H

typedef struct _modbus modbus_t;
struct fconf;

int print_all_registers(modbus_t *ctx);
int print_register(modbus_t *ctx, int addr);
int publish_registers(modbus_t *ctx, struct fconf *conf);
int reset_energie(modbus_t *ctx);
int check_reset(modbus_t *ctx, const struct fconf *conf);

#endif
