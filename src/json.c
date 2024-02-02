#include <jsmn.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "defines.h"
#include "json.h"

static B8 JsonEq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return true;
    }
    return false;
}

void JsonObjectGetKey(const char *json, const char *key, char *value) {
    jsmn_parser parser;
    jsmntok_t tokens[128];

    jsmn_init(&parser);
    I32 r = jsmn_parse(&parser, json, strlen(json), tokens, 128);
    if (r < 0) {
        printf("Failed to parse JSON: %d\n", r);
        return;
    }

    if (r < 1 || tokens[0].type != JSMN_OBJECT) {
        printf("Object expected\n");
        return;
    }

    for (I32 i = 1; i < r; ++i) {
        if (JsonEq(json, &tokens[i], key)) {
            char buf[256];
            snprintf(buf, 256, "%.*s\n",
                     tokens[i + 1].end - tokens[i + 1].start,
                     json + tokens[i + 1].start);
            strcpy(value, buf);
            break;
        }
    }
}
