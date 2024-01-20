#include "api.h"

#include "lua.h"
#include "renderer.h"
#include "state.h"

#include <assert.h>
#include <lauxlib.h>
#include <lualib.h>

#include <raylib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "parameter.h"

State *api_state;

static Color PopColor(lua_State *L);
static void PushApi(Api *api);
static ApiData PopApi(Api *api);

static int RegisterCallback(lua_State *L);
static void CallCallback(lua_State *L, int *callback);
static void FreeCallback(lua_State *L, int *callback);

static int L_GetMusicTimePlayed(lua_State *L);
static int L_AddParameter(lua_State *L);
static int L_SetParameter(lua_State *L);
static int L_GetParameter(lua_State *L);
static int L_OnUpdate(lua_State *L);
static int L_OnRender(lua_State *L);
static int L_SetBgColor(lua_State *L);
static int L_GetBgColor(lua_State *L);

static int L_DrawLinedPoly(lua_State *L);

static void SetPath(lua_State *L, const char *path) {
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path");

    const char *current_path = lua_tostring(L, -1);

    char buffer[512];
    snprintf(buffer, 512, "%s;%s", current_path, path);

    lua_pop(L, 1);

    lua_pushstring(L, buffer);
    lua_setfield(L, -2, "path");
    lua_pop(L, 1);
}

void ApiCreate(const char *api_fp, void *state, Api *api) {
    api_state = (State *)state;

    api->lua = luaL_newstate();
    luaL_openlibs(api->lua);

    memset(api->on_update, -1, sizeof(ApiCallback) * MAX_API_CALLBACKS);
    memset(api->on_render, -1, sizeof(ApiCallback) * MAX_API_CALLBACKS);

    PushApi(api);

    // Set the path to lua
    {
        char cwd[512];
        getcwd(cwd, 512);

        char buffer[512];
        snprintf(buffer, 512, "%s/lua/?.lua", cwd);

        SetPath(api->lua, buffer);
    }

    if (luaL_dofile(api->lua, api_fp)) {
        printf("Failed to load API: %s\n", lua_tostring(api->lua, -1));
    }

    api->data = PopApi(api);
}

void ApiUpdate(Api *api, void *state) {
    api_state = (State *)state;

    for (U32 i = 0; i < api->on_update_count; ++i) {
        if (api->on_update[i] != -1) {
            CallCallback(api->lua, &api->on_update[i]);
        }
    }
}

void ApiRender(Api *api, void *state) {
    api_state = (State *)state;

    for (U32 i = 0; i < api->on_render_count; ++i) {
        if (api->on_render[i] != -1) {
            CallCallback(api->lua, &api->on_render[i]);
        }
    }
}

void ApiDestroy(Api *api) {
    for (U32 i = 0; i < api->on_update_count; ++i) {
        if (api->on_update[i] != -1) {
            FreeCallback(api->lua, &api->on_update[i]);
        }
    }

    for (U32 i = 0; i < api->on_render_count; ++i) {
        if (api->on_render[i] != -1) {
            FreeCallback(api->lua, &api->on_render[i]);
        }
    }

    lua_close(api->lua);
}

void PushApi(Api *api) {
    lua_newtable(api->lua);
    {
        lua_pushstring(api->lua, "opt");
        lua_newtable(api->lua);
        {
            lua_pushstring(api->lua, "bg_color");
            lua_newtable(api->lua);
            {
                lua_pushnumber(api->lua, 1);
                lua_pushnumber(api->lua, 0);
                lua_settable(api->lua, -3);

                lua_pushnumber(api->lua, 2);
                lua_pushnumber(api->lua, 0);
                lua_settable(api->lua, -3);

                lua_pushnumber(api->lua, 3);
                lua_pushnumber(api->lua, 0);
                lua_settable(api->lua, -3);

                lua_pushnumber(api->lua, 4);
                lua_pushnumber(api->lua, 0);
                lua_settable(api->lua, -3);
            }
            lua_settable(api->lua, -3);
        }
        lua_settable(api->lua, -3);

        lua_pushstring(api->lua, "api");
        lua_newtable(api->lua);
        {
            lua_pushstring(api->lua, "get_music_time_played");
            lua_pushcfunction(api->lua, L_GetMusicTimePlayed);
            lua_settable(api->lua, -3);

            lua_pushstring(api->lua, "add_param");
            lua_pushcfunction(api->lua, L_AddParameter);
            lua_settable(api->lua, -3);

            lua_pushstring(api->lua, "get_param");
            lua_pushcfunction(api->lua, L_GetParameter);
            lua_settable(api->lua, -3);

            lua_pushstring(api->lua, "set_param");
            lua_pushcfunction(api->lua, L_SetParameter);
            lua_settable(api->lua, -3);

            lua_pushstring(api->lua, "on_update");
            lua_pushcfunction(api->lua, L_OnUpdate);
            lua_settable(api->lua, -3);

            lua_pushstring(api->lua, "on_render");
            lua_pushcfunction(api->lua, L_OnRender);
            lua_settable(api->lua, -3);

            lua_pushstring(api->lua, "set_bg_color");
            lua_pushcfunction(api->lua, L_SetBgColor);
            lua_settable(api->lua, -3);

            lua_pushstring(api->lua, "get_bg_color");
            lua_pushcfunction(api->lua, L_GetBgColor);
            lua_settable(api->lua, -3);

            lua_pushstring(api->lua, "draw_lined_poly");
            lua_pushcfunction(api->lua, L_DrawLinedPoly);
            lua_settable(api->lua, -3);
        }
        lua_settable(api->lua, -3);
    }
    lua_setglobal(api->lua, "lynx");
}

Color PopColor(lua_State *L) {
    Color c;

    lua_geti(L, -1, 1);
    c.r = lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_geti(L, -1, 2);
    c.g = lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_geti(L, -1, 3);
    c.b = lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_geti(L, -1, 4);
    c.a = lua_tonumber(L, -1);
    lua_pop(L, 1);

    return c;
}

void PushColor(lua_State *L, Color c) {
    lua_newtable(L);
    {
        lua_pushnumber(L, 1);
        lua_pushnumber(L, c.r);
        lua_settable(L, -3);

        lua_pushnumber(L, 2);
        lua_pushnumber(L, c.g);
        lua_settable(L, -3);

        lua_pushnumber(L, 3);
        lua_pushnumber(L, c.b);
        lua_settable(L, -3);

        lua_pushnumber(L, 4);
        lua_pushnumber(L, c.a);
        lua_settable(L, -3);
    }
}

ApiData PopApi(Api *api) {
    ApiData data;

    lua_getglobal(api->lua, "lynx");
    {
        lua_pushstring(api->lua, "opt");
        lua_gettable(api->lua, -2);
        {
            lua_pushstring(api->lua, "bg_color");
            lua_gettable(api->lua, -2);
            { data.opt.bg_color = PopColor(api->lua); }
            lua_pop(api->lua, 1);
        }
        lua_pop(api->lua, 1);
    }
    lua_pop(api->lua, 1);

    return data;
}

int RegisterCallback(lua_State *L) { return luaL_ref(L, LUA_REGISTRYINDEX); }

void CallCallback(lua_State *L, ApiCallback *callback) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, *callback);

    lua_pushvalue(L, 1);

    if (lua_pcall(L, 0, 0, 0) != 0) {
        printf("Failed to call callback: %s\n", lua_tostring(L, -1));
    }

    *callback = luaL_ref(L, LUA_REGISTRYINDEX);
}

void FreeCallback(lua_State *L, ApiCallback *callback) {
    luaL_unref(L, LUA_REGISTRYINDEX, *callback);

    *callback = 0;
}

static int L_GetMusicTimePlayed(lua_State *L) {
    F32 time_played = api_state->condition == StateCondition_RECORDING
                          ? GetTime() - api_state->record_start
                          : GetMusicTimePlayed(api_state->music);

    lua_pushnumber(L, time_played);

    return 1;
}

static int L_AddParameter(lua_State *L) {
    const char *name = lua_tostring(L, 1);
    F32 value = lua_tonumber(L, 2);
    F32 min = lua_tonumber(L, 3);
    F32 max = lua_tonumber(L, 4);

    Parameter param = {
        .name = name,
        .value = value,
        .min = min,
        .max = max,
    };

    ParameterSet(api_state->parameters, &param);

    return 0;
}

static int L_SetParameter(lua_State *L) {
    const char *name = lua_tostring(L, 1);
    F32 value = lua_tonumber(L, 2);

    ParameterSetValue(api_state->parameters, name, value);

    return 0;
}

static int L_GetParameter(lua_State *L) {
    const char *name = lua_tostring(L, 1);

    lua_pushnumber(L, ParameterGetValue(api_state->parameters, name));

    return 1;
}

static int L_OnUpdate(lua_State *L) {
    api_state->api->on_update[api_state->api->on_update_count++] =
        RegisterCallback(L);

    return 0;
}

static int L_OnRender(lua_State *L) {
    api_state->api->on_render[api_state->api->on_render_count++] =
        RegisterCallback(L);

    return 0;
}

int L_SetBgColor(lua_State *L) {
    api_state->api->data.opt.bg_color = PopColor(L);

    return 0;
}

int L_GetBgColor(lua_State *L) {
    PushColor(L, api_state->api->data.opt.bg_color);

    return 1;
}

// TODO(satvik): Make this user-proof
static int L_DrawLinedPoly(lua_State *L) {
    U32 n = lua_gettop(L);

    assert(n == 3);

    U32 vertex_count = luaL_len(L, 1);
    HMM_Vec2 vertices[vertex_count];

    U32 index_count = luaL_len(L, 2);
    HMM_Vec2 indices[index_count];

    assert(vertex_count == index_count);

    F32 color[4];
    for (U32 i = 0; i < 4; ++i) {
        lua_geti(L, -1, i + 1);
        color[i] = lua_tonumber(L, -1);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    // Indices are at the top of the stack
    for (U32 i = 0; i < index_count; ++i) {
        lua_geti(L, -1, i + 1);
        {
            lua_geti(L, -1, 1);
            lua_geti(L, -2, 2);

            HMM_Vec2 index = HMM_V2(lua_tonumber(L, -2), lua_tonumber(L, -1));

            lua_pop(L, 2);

            indices[i] = index;
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    // Vertices are now at the top of the stack
    for (U32 i = 0; i < vertex_count; ++i) {
        lua_geti(L, -1, i + 1);
        {
            lua_geti(L, -1, 1);
            lua_geti(L, -2, 2);

            HMM_Vec2 vertex = HMM_V2(lua_tonumber(L, -2), lua_tonumber(L, -1));

            lua_pop(L, 2);

            vertices[i] = vertex;
        }
        lua_pop(L, 1);
    }

    RendererDrawLinedPoly(api_state->renderer, vertices, vertex_count, indices,
                          index_count,
                          (Color){color[0], color[1], color[2], color[3]});

    return 0;
}

