#pragma once

#include "arena.h"
#include "defines.h"
#include "thread.h"

#include <curl/curl.h>

typedef struct ServerData {
    CURL       *curl;
    const char *uri;
    Thread     *thread;
} ServerData;

typedef struct Memory {
    char *response;
    U32   size;
} Memory;

void ServerInitialise(ServerData  *server_data,
                      const char  *uri,
                      MemoryArena *arena);
void ServerDestroy(ServerData *server_data);
void ServerGet(ServerData *server_data, char *endpoint, char *response);

void ServerGetAsync(ServerData *server_data,
                    char       *endpoint,
                    void       *user_data,
                    void (*callback)(void *user_data, char *response),
                    MemoryArena *arena);

void ServerPost(ServerData *server_data,
                const char *endpoint,
                const char *data,
                char       *response);

void ServerPostAsync(ServerData *server_data,
                     char       *endpoint,
                     const char *data,
                     void       *user_data,
                     void (*callback)(void *user_data, char *response),
                     MemoryArena *arena);

void ServerWait(ServerData *server_data);
