/**
 * @file conf.h  Configuration reader/writer API
 *
 * Copyright (C) 2022 Christian Spielberger
 */
#ifndef FCONF_H
#define FCONF_H

#include <stdint.h>


struct fconf {
	char *ttydev;
	int baudrate;
	int stopbits;
	int slaveid;
	char *mqtthost;
	int   mqttport;
	char *mqttuser;
	char *mqttpass;
	char *capath;
	char *cafile;
	char *publish[10];
	int reset_hh;
};


int read_fconfig(struct fconf *conf, const char *fname);
int write_conf_file(struct fconf *conf, const char *fname);
void free_fconfig(struct fconf *conf);

#endif
