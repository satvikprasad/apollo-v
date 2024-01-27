#include "api.h"

#include "defines.h"
#include "handmademath.h"
#include "hashmap.h"
#include "lmath.h"
#include "lua.h"
#include "renderer.h"
#include "signals.h"
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

#define CheckArgument(L, type, index, function)                                \
    if (lua_type(L, index) != type) {                                          \
        ApiErrorFunction(L, function,                                          \
                         "received non-" #type " argument " #index);           \
        return 0;                                                              \
    }

static Color PopColor(lua_State *L);
static void PushApi(Api *api);
static ApiData PopApi(Api *api);

static int RegisterCallback(lua_State *L);
static void CallCallback(lua_State *L, int *callback);
static void FreeCallback(lua_State *L, int *callback);

static void DumpStack(lua_State *L) {
    int top = lua_gettop(L);
    for (int i = 1; i <= top; i++) {
        printf("%d\t%s\t", i, luaL_typename(L, i));
        switch (lua_type(L, i)) {
        case LUA_TNUMBER:
            printf("%g\n", lua_tonumber(L, i));
            break;
        case LUA_TSTRING:
            printf("%s\n", lua_tostring(L, i));
            break;
        case LUA_TBOOLEAN:
            printf("%s\n", (lua_toboolean(L, i) ? "true" : "false"));
            break;
        case LUA_TNIL:
            printf("%s\n", "nil");
            break;
        default:
            printf("%p\n", lua_topointer(L, i));
            break;
        }
    }
}

#define X(func, name) static int func(lua_State *L);
API_METHODS
#undef X

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

static U64 ApiShaderHash(const void *item, U64 seed0, U64 seed1) {
    ApiShader *pshader = (ApiShader *)item;

    char buf[512];

    snprintf(buf, 512, "%s%s", pshader->vertex, pshader->fragment);

    return hashmap_sip(buf, strlen(buf), seed0, seed1);
}

static I32 ApiShaderCompare(const void *a, const void *b, void *udata) {
    (void)(udata);

    ApiShader *pa = (ApiShader *)a;
    ApiShader *pb = (ApiShader *)b;

    char bufa[512];
    snprintf(bufa, 512, "%s%s", pa->vertex, pa->fragment);

    char bufb[512];
    snprintf(bufb, 512, "%s%s", pb->vertex, pb->fragment);

    return strcmp(bufa, bufb);
}

static void ApiShaderFree(void *el) { UnloadShader(((ApiShader *)el)->shader); }

void ApiCreate(const char *api_fp, void *state, Api *api) {
    api_state = (State *)state;

    api->lua = luaL_newstate();
    luaL_openlibs(api->lua);

    api->shaders = hashmap_new(sizeof(ApiShader), 0, 0, 0, ApiShaderHash,
                               ApiShaderCompare, ApiShaderFree, NULL);

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

void ApiPreUpdate(Api *api, void *state) {
    api_state = (State *)state;

    for (U32 i = 0; i < api->pre_update_count; ++i) {
        if (api->pre_update[i] != -1) {
            CallCallback(api->lua, &api->pre_update[i]);
        }
    }
}

void ApiUpdate(Api *api, void *state) {
    api_state = (State *)state;

    for (U32 i = 0; i < api->on_update_count; ++i) {
        if (api->on_update[i] != -1) {
            CallCallback(api->lua, &api->on_update[i]);
        }
    }
}

void ApiPreRender(Api *api, void *state) {
    api_state = (State *)state;

    for (U32 i = 0; i < api->pre_render_count; ++i) {
        if (api->pre_render[i] != -1) {
            CallCallback(api->lua, &api->pre_render[i]);
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

    for (U32 i = 0; i < api->pre_update_count; ++i) {
        if (api->pre_update[i] != -1) {
            FreeCallback(api->lua, &api->pre_update[i]);
        }
    }

    for (U32 i = 0; i < api->pre_render_count; ++i) {
        if (api->pre_render[i] != -1) {
            FreeCallback(api->lua, &api->pre_render[i]);
        }
    }

    hashmap_free(api_state->api->shaders);

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
#define X(func, name)                                                          \
    lua_pushstring(api->lua, #name);                                           \
    lua_pushcfunction(api->lua, (func));                                       \
    lua_settable(api->lua, -3);
            API_METHODS
#undef X

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

            lua_pushstring(api->lua, "pre_update");
            lua_pushcfunction(api->lua, L_PreUpdate);
            lua_settable(api->lua, -3);

            lua_pushstring(api->lua, "pre_render");
            lua_pushcfunction(api->lua, L_PreRender);
            lua_settable(api->lua, -3);

            lua_pushstring(api->lua, "set_bg_color");
            lua_pushcfunction(api->lua, L_SetBgColor);
            lua_settable(api->lua, -3);

            lua_pushstring(api->lua, "get_bg_color");
            lua_pushcfunction(api->lua, L_GetBgColor);
            lua_settable(api->lua, -3);

            lua_pushstring(api->lua, "get_screen_size");
            lua_pushcfunction(api->lua, L_GetScreenSize);
            lua_settable(api->lua, -3);

            lua_pushstring(api->lua, "get_samples");
            lua_pushcfunction(api->lua, L_GetSamples);
            lua_settable(api->lua, -3);

            lua_pushstring(api->lua, "smooth_signal");
            lua_pushcfunction(api->lua, L_SmoothSignal);
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
    if (!lua_istable(L, -1)) {
        ApiError(L, "tried parsing non-table color");
        return (Color){0};
    }

    if (luaL_len(L, -1) != 4) {
        ApiError(L, "tried parsing non-four sized color");
        return (Color){0};
    }

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
    CheckArgument(L, LUA_TSTRING, 1, add_param);
    CheckArgument(L, LUA_TNUMBER, 2, add_param);
    CheckArgument(L, LUA_TNUMBER, 3, add_param);
    CheckArgument(L, LUA_TNUMBER, 4, add_param);

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
    CheckArgument(L, LUA_TSTRING, 1, set_param);
    CheckArgument(L, LUA_TNUMBER, 2, set_param);

    const char *name = lua_tostring(L, 1);
    F32 value = lua_tonumber(L, 2);

    ParameterSetValue(api_state->parameters, name, value);

    return 0;
}

static int L_GetParameter(lua_State *L) {
    CheckArgument(L, LUA_TSTRING, 1, get_param);

    const char *name = lua_tostring(L, 1);

    lua_pushnumber(L, ParameterGetValue(api_state->parameters, name));

    return 1;
}

static int L_OnUpdate(lua_State *L) {
    CheckArgument(L, LUA_TFUNCTION, 1, on_update);

    api_state->api->pre_update[api_state->api->pre_update_count++] =
        RegisterCallback(L);

    return 0;
}

static int L_PreUpdate(lua_State *L) {
    CheckArgument(L, LUA_TFUNCTION, 1, pre_update);

    api_state->api->on_update[api_state->api->on_update_count++] =
        RegisterCallback(L);

    return 0;
}

static int L_OnRender(lua_State *L) {
    CheckArgument(L, LUA_TFUNCTION, 1, on_render);

    api_state->api->on_render[api_state->api->on_render_count++] =
        RegisterCallback(L);

    return 0;
}

static int L_PreRender(lua_State *L) {
    CheckArgument(L, LUA_TFUNCTION, 1, pre_render);

    api_state->api->pre_render[api_state->api->pre_render_count++] =
        RegisterCallback(L);

    return 0;
}

int L_SetBgColor(lua_State *L) {
    CheckArgument(L, LUA_TTABLE, 1, set_bg_color);

    api_state->api->data.opt.bg_color = PopColor(L);

    return 0;
}

int L_GetBgColor(lua_State *L) {
    PushColor(L, api_state->api->data.opt.bg_color);

    return 1;
}

static void PushArray(lua_State *L, F32 *array, U32 count) {
    lua_newtable(L);
    for (U32 i = 0; i < count; ++i) {
        lua_pushnumber(L, i + 1);
        lua_pushnumber(L, array[i]);
        lua_settable(L, -3);
    }
}

static void PopArray(lua_State *L, F32 *out, U32 count) {
    F32 array[count];

    for (U32 i = 0; i < count; ++i) {
        lua_geti(L, -1, i + 1);
        array[i] = lua_tonumber(L, -1);
        lua_pop(L, 1);
    }

    memcpy(out, array, sizeof(F32) * count);
}

static int L_GetSamples(lua_State *L) {
    PushArray(L, api_state->samples, SAMPLE_COUNT);

    return 1;
}

static int L_SmoothSignal(lua_State *L) {
    CheckArgument(L, LUA_TTABLE, 1, smooth_signal);

    U32 length = luaL_len(L, 1);

    F32 in[length];

    PopArray(L, in, length);

    U32 out_length;
    SignalsSmoothConvolve(NULL, length, NULL, api_state->filter_count, NULL,
                          &out_length);

    F32 out[out_length];
    SignalsSmoothConvolve(in, length, api_state->filter,
                          api_state->filter_count, out, &out_length);

    PushArray(L, out, out_length);

    return 1;
}

static int L_GetScreenSize(lua_State *L) {
    lua_newtable(L);
    {
        lua_pushnumber(L, 1);
        lua_pushnumber(L, api_state->screen_size.X);
        lua_settable(L, -3);

        lua_pushnumber(L, 2);
        lua_pushnumber(L, api_state->screen_size.Y);
        lua_settable(L, -3);
    }

    return 1;
}

void ApiError(lua_State *L, const char *msg) {
    luaL_traceback(L, L, NULL, 1);
    printf("Error calling lua: %s\n%s\n", msg, lua_tostring(L, -1));
    lua_pop(L, 1);
    return;
}

static HMM_Vec2 PopVec2(lua_State *L) {
    HMM_Vec2 out;

    if (!lua_istable(L, -1)) {
        ApiError(L, "tried parsing non-table vector");
        return out;
    }

    lua_geti(L, -1, 1);
    lua_geti(L, -2, 2);

    out = HMM_V2(lua_tonumber(L, -2), lua_tonumber(L, -1));

    lua_pop(L, 2);

    return out;
}

// TODO(satvik): Make this user-proof
static int L_DrawLinedPoly(lua_State *L) {
    U32 n = lua_gettop(L);

    assert(n == 3 || "lynx.api.draw_lined_poly requires 3 arguments");

    CheckArgument(L, LUA_TTABLE, 1, draw_lined_poly);
    CheckArgument(L, LUA_TTABLE, 2, draw_lined_poly);
    CheckArgument(L, LUA_TTABLE, 3, draw_lined_poly);

    U32 vertex_count = luaL_len(L, 1);

    U32 index_count = luaL_len(L, 2);

    assert(vertex_count == index_count || "lynx.api.draw_lined_poly received "
                                          "mismatched vertex and index counts");
    assert(luaL_len(L, 3) == 4 || "lynx.api.draw_lined_poly received "
                                  "incorrect color array size");

    Color color = PopColor(L);

    lua_pop(L, 1);

    // Indices are at the top of the stack
    HMM_Vec2 indices[index_count];
    for (U32 i = 0; i < index_count; ++i) {
        lua_geti(L, -1, i + 1);

        if (!lua_istable(L, -1)) {
            ApiErrorFunction(L, draw_lined_poly,
                             "received non-two sized vector index");
            return 0;
        }

        indices[i] = PopVec2(L);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    // Vertices are now at the top of the stack
    HMM_Vec2 vertices[vertex_count];
    for (U32 i = 0; i < vertex_count; ++i) {
        lua_geti(L, -1, i + 1);

        if (!lua_istable(L, -1)) {
            ApiErrorFunction(L, draw_lined_poly,
                             "received non-two sized vector vertex");
            return 0;
        }
        vertices[i] = PopVec2(L);
        lua_pop(L, 1);
    }

    RendererDrawLinedPoly(api_state->renderer, vertices, vertex_count, indices,
                          index_count, color);

    return 0;
}

static int L_BindShader(lua_State *L) {
    const char *fragment_shader;
    const char *vertex_shader;

    if (!lua_isstring(api_state->api->lua, 1)) {
        if (!lua_isnumber(L, 2)) {
            ApiErrorFunction(api_state->api->lua, bind_shader,
                             "received non-string and non-zero vertex shader");
            return 0;
        }

        vertex_shader = NULL;
    } else {
        vertex_shader = lua_tostring(api_state->api->lua, 1);
    }

    if (!lua_isstring(api_state->api->lua, 2)) {
        if (!lua_isnumber(L, 1)) {
            ApiErrorFunction(
                api_state->api->lua, bind_shader,
                "received non-string and non-zero fragment shader");
            return 0;
        }

        fragment_shader = NULL;
    } else {
        fragment_shader = lua_tostring(api_state->api->lua, 2);
    }

    ApiShader seek = {};

    strcpy(seek.fragment, fragment_shader);
    strcpy(seek.vertex, vertex_shader);

    ApiShader *shad = (ApiShader *)hashmap_get(api_state->api->shaders, &seek);

    if (shad == NULL) {
        seek.shader = LoadShader(vertex_shader, fragment_shader);
        hashmap_set(api_state->api->shaders, &seek);
    }

    shad = (ApiShader *)hashmap_get(api_state->api->shaders, &seek);

    assert(shad != NULL);

    BeginShaderMode(shad->shader);

    return 0;
}

static int L_UnbindShader(lua_State *L) {
    EndShaderMode();

    return 0;
}

static int L_DrawCenteredText(lua_State *L) {
    CheckArgument(L, LUA_TSTRING, 1, draw_centered_text);
    CheckArgument(L, LUA_TTABLE, 2, draw_centered_text);
    CheckArgument(L, LUA_TNUMBER, 3, draw_centered_text);

    const char *text = lua_tostring(L, 1);
    U32 size = lua_tonumber(L, 3);

    lua_pop(L, 1);

    HMM_Vec2 pos = PopVec2(L);

    RendererDrawTextCenter(FontClosestToSize(api_state->font, size), text, pos);

    return 0;
}

