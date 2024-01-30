#include "defines.h"
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

static inline U32 WriteFunc(void *data, U32 size, U32 nmemb, void *p_client) {
    U32 write_size = size * nmemb;
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

void ServerGet(char *url, char *response) {
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();

    if (curl) {
        Memory mem;
        mem.response = malloc(1);
        mem.size = 0;

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&mem);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            strcpy(response, mem.response);
        }

        free(mem.response);
        curl_easy_cleanup(curl);
    }
}
