#include "arena.h"
#include "defines.h"

#include "server.h"

#include <curl/curl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static inline U32 WriteFunc(void *data, U32 size, U32 nmemb, void *p_client) {
    U32     write_size = size * nmemb;
    Memory *mem = (Memory *)p_client;

    char *ptr = realloc(mem->response, mem->size + write_size + 1);
    if (ptr == NULL) {
        return 0;
    }

    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), data, write_size);
    mem->size += write_size;
    mem->response[mem->size] = 0;

    return write_size;
}

void ServerInitialise(ServerData *server_data, const char *uri) {
    server_data->curl = curl_easy_init();
    server_data->uri = uri;
}

void ServerDestroy(ServerData *server_data) {
    curl_easy_cleanup(server_data->curl);
}

static inline void FormatUrl(const char *uri, const char *endpoint, char *url) {
    char buf[512];

    snprintf(buf, 512, "%s/%s", uri, endpoint);

    strcpy(url, buf);
}

void ServerGet(ServerData *server_data, char *endpoint, char *response) {
    CURLcode res;

    char url[512];
    FormatUrl(server_data->uri, endpoint, url);

    if (server_data->curl) {
        Memory mem;
        mem.response = malloc(1);
        mem.size = 0;

        curl_easy_setopt(server_data->curl, CURLOPT_URL, url);
        curl_easy_setopt(server_data->curl, CURLOPT_WRITEFUNCTION, WriteFunc);
        curl_easy_setopt(server_data->curl, CURLOPT_WRITEDATA, (void *)&mem);

        res = curl_easy_perform(server_data->curl);

        if (res != CURLE_OK) {
            printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            strcpy(response, mem.response);
        }

        free(mem.response);
    }
}

typedef struct GetRequestArgs {
    ServerData *server_data;
    char       *endpoint;
    char       *response;
    void       *user_data;

    void (*callback)(void *user_data, char *response);
} GetRequestArgs;

static inline void *GetRequestThread(void *data) {
    GetRequestArgs *args = (GetRequestArgs *)data;

    ServerGet(args->server_data, args->endpoint, args->response);

    if (args->callback) {
        args->callback(args->user_data, args->response);
    }

    return NULL;
}

void ServerGetAsync(ServerData *server_data,
                    char       *endpoint,
                    void       *user_data,
                    void (*callback)(void *user_data, char *response),
                    MemoryArena *arena) {
    GetRequestArgs *args = ArenaPushStruct(arena, GetRequestArgs);

    args->server_data = server_data;
    args->endpoint = endpoint;
    args->response = ArenaPushArray(arena, 1024, char);
    args->user_data = user_data;
    args->callback = callback;

    pthread_create(&server_data->thread, NULL, GetRequestThread, args);
}

void ServerPost(ServerData *server_data,
                const char *endpoint,
                const char *data,
                char       *response) {
    CURLcode res;

    char url[512];
    FormatUrl(server_data->uri, endpoint, url);

    if (server_data->curl) {
        Memory mem;
        mem.response = malloc(1);
        mem.size = 0;

        struct curl_slist *headers = NULL;

        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(server_data->curl, CURLOPT_URL, url);
        curl_easy_setopt(server_data->curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(server_data->curl, CURLOPT_POSTFIELDS, data);
        curl_easy_setopt(server_data->curl, CURLOPT_WRITEFUNCTION, WriteFunc);
        curl_easy_setopt(server_data->curl, CURLOPT_WRITEDATA, (void *)&mem);

        res = curl_easy_perform(server_data->curl);

        if (res != CURLE_OK) {
            printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            strcpy(response, mem.response);
        }

        free(mem.response);
    }
}

typedef struct PostRequestArgs {
    ServerData *server_data;
    char       *endpoint;
    const char *data;
    char       *response;
    void       *user_data;

    void (*callback)(void *user_data, char *response);
} PostRequestArgs;

void *PostRequestThread(void *data) {
    PostRequestArgs *args = (PostRequestArgs *)data;

    ServerPost(args->server_data, args->endpoint, args->data, args->response);

    if (args->callback) {
        args->callback(args->user_data, args->response);
    }

    return NULL;
}

void ServerPostAsync(ServerData *server_data,
                     char       *endpoint,
                     const char *data,
                     void       *user_data,
                     void (*callback)(void *user_data, char *response),
                     MemoryArena *arena) {
    PostRequestArgs *args = ArenaPushStruct(arena, PostRequestArgs);

    args->server_data = server_data;
    args->endpoint = endpoint;
    args->data = data;
    args->response = ArenaPushArray(arena, 1024, char);
    args->user_data = user_data;
    args->callback = callback;

    pthread_create(&server_data->thread, NULL, PostRequestThread, args);
}

void ServerWait(ServerData *server_data) {
    pthread_join(server_data->thread, NULL);
}
