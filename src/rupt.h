#pragma once

#include <stdint.h>
#include <stdio.h>

#define DEBUG_LOG if (1) printf

typedef void (*rupt_process_message)(char *message, unsigned long length);

int rupt_init_server(const char *, uint16_t, rupt_process_message);
int rupt_init_client(const char *, uint16_t);
int rupt_send_message(const char *, unsigned long size);