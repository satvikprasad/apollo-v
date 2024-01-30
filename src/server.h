#pragma once

#include "defines.h"

typedef struct Memory {
    char *response;
    U32 size;
} Memory;

void ServerGet(char *url, char *response);
