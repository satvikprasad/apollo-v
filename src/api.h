#pragma once

#include "defines.h"
#include "lua.h"
#include "raylib.h"

typedef I32 ApiCallback;

typedef struct ApiData {
    struct {
        Color bg_color;
    } opt;
} ApiData;

#define MAX_API_CALLBACKS 256
typedef struct Api {
    lua_State *lua;
    ApiData data;

    ApiCallback on_update[MAX_API_CALLBACKS];
    U32 on_update_count;

    ApiCallback on_render[MAX_API_CALLBACKS];
    U32 on_render_count;
} Api;

void ApiCreate(const char *api_fp, void *state, Api *api);
void ApiUpdate(Api *api, void *state);
void ApiRender(Api *api, void *state);
void ApiDestroy(Api *api);
