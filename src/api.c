#include "api.h"

#include "animation.h"
#include "arena.h"
#include "defines.h"
#include "filesystem.h"
#include "handmademath.h"
#include "hashmap.h"
#include "lmath.h"
#include "lua.h"
#include "procedures.h"
#include "renderer.h"
#include "signals.h"
#include "state.h"

#include <assert.h>
#include <lauxlib.h>
#include <lualib.h>

#include <math.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "parameter.h"

State *p_state;

#define CheckArgument(L, type, index, function)                                \
    if (lua_type(L, index) != type) {                                          \
        ApiErrorFunction(L, function,                                          \
                         "received non-" #type " argument " #index);           \
        return 0;                                                              \
    }

static Color
PopColor(lua_State *L);

static void
PushColor(lua_State *L, Color c);

static HMM_Vec2
PopVec2(lua_State *L);

static void
PushVec2(lua_State *L, HMM_Vec2 vec);

static void
PushApi(ApiData *api);

static ApiInterface
PopApi(ApiData *api);

static int
RegisterCallback(lua_State *L);

#define CallCallback(L, callback, args)                                        \
    if (!CallCallback_((L), (callback), (args))) {                             \
        printf("Error calling callback in %s: %s\n", #callback,                \
               lua_tostring(L, -1));                                           \
    }

static B8
CallCallback_(lua_State *L, int *callback, U32 args);

static void
FreeCallback(lua_State *L, int *callback);

static void
DumpStack(lua_State *L) {
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
API_METHODS_ANIMATION
API_METHODS_RENDERER
API_METHODS_PROCS
API_METHODS_PARAMS
#undef X

static void
SetPath(lua_State *L, const char *path) {
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

static U64
ApiShaderHash(const void *item, U64 seed0, U64 seed1) {
    ApiShader *pshader = (ApiShader *)item;

    char buf[512];

    snprintf(buf, 512, "%s%s", pshader->vertex, pshader->fragment);

    return hashmap_sip(buf, strlen(buf), seed0, seed1);
}

static I32
ApiShaderCompare(const void *a, const void *b, void *udata) {
    (void)(udata);

    ApiShader *pa = (ApiShader *)a;
    ApiShader *pb = (ApiShader *)b;

    char bufa[512];
    snprintf(bufa, 512, "%s%s", pa->vertex, pa->fragment);

    char bufb[512];
    snprintf(bufb, 512, "%s%s", pb->vertex, pb->fragment);

    return strcmp(bufa, bufb);
}

static void
ApiShaderFree(void *el) {
    UnloadShader(((ApiShader *)el)->shader);
}

void
ApiInitialise(const char *api_fp, void *state, ApiData *api) {
    p_state = (State *)state;

    api->lua = luaL_newstate();
    luaL_openlibs(api->lua);

    api->shaders = hashmap_new(sizeof(ApiShader), 0, 0, 0, ApiShaderHash,
                               ApiShaderCompare, ApiShaderFree, NULL);

    PushApi(api);

    // Set the path to lua
    {
        char cwd[512];
        getcwd(cwd, 512);

        char home[512];
        FSGetHomeDirectory(home);

        char buffer[512];
        snprintf(buffer, 512, "%s/lua/?.lua;%s/.config/apollo/?.lua", cwd,
                 home);

        SetPath(api->lua, buffer);
    }

    if (luaL_dofile(api->lua, api_fp)) {
        printf("Failed to load API: %s\n", lua_tostring(api->lua, -1));
    }

    api->data = PopApi(api);
}

void
ApiPreUpdate(ApiData *api, void *state) {
    p_state = (State *)state;

    for (U32 i = 0; i < api->pre_update_count; ++i) {
        if (api->pre_update[i] != -1) {
            CallCallback(api->lua, &api->pre_update[i], 0);
        }
    }
}

void
ApiUpdate(ApiData *api, void *state) {
    p_state = (State *)state;

    for (U32 i = 0; i < api->on_update_count; ++i) {
        if (api->on_update[i] != -1) {
            CallCallback(api->lua, &api->on_update[i], 0);
        }
    }
}

void
ApiPreRender(ApiData *api, void *state) {
    p_state = (State *)state;

    for (U32 i = 0; i < api->pre_render_count; ++i) {
        if (api->pre_render[i] != -1) {
            CallCallback(api->lua, &api->pre_render[i], 0);
        }
    }
}

void
ApiRender(ApiData *api, void *state) {
    p_state = (State *)state;

    for (U32 i = 0; i < api->on_render_count; ++i) {
        if (api->on_render[i] != -1) {
            CallCallback(api->lua, &api->on_render[i], 0);
        }
    }
}

void
ApiDestroy(ApiData *api) {
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

    hashmap_free(p_state->api_data->shaders);

    lua_close(api->lua);
}

void
PushApi(ApiData *api) {
    lua_newtable(api->lua);
    {
        lua_pushstring(api->lua, "opt");
        lua_newtable(api->lua);
        {
            lua_pushstring(api->lua, "bg_color");
            PushColor(api->lua, api->data.opt.bg_color);
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

#define NSPACE(nspace, methods)                                                \
    lua_pushstring(api->lua, #nspace);                                         \
    lua_newtable(api->lua);                                                    \
    methods lua_settable(api->lua, -3);
            NSPACE(animation, API_METHODS_ANIMATION)
            NSPACE(renderer, API_METHODS_RENDERER)
            NSPACE(proc, API_METHODS_PROCS)
            NSPACE(param, API_METHODS_PARAMS)
#undef NSPACE

#undef X
        }
        lua_settable(api->lua, -3);
    }
    lua_setglobal(api->lua, "lynx");
}

Color
PopColor(lua_State *L) {
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
    lua_pop(L, 2);

    return c;
}

void
PushColor(lua_State *L, Color c) {
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

ApiInterface
PopApi(ApiData *api) {
    ApiInterface data;

    lua_getglobal(api->lua, "lynx");
    {
        lua_pushstring(api->lua, "opt");
        lua_gettable(api->lua, -2);
        {
            lua_pushstring(api->lua, "bg_color");
            lua_gettable(api->lua, -2);
            data.opt.bg_color = PopColor(api->lua);
        }
        lua_pop(api->lua, 1);
    }
    lua_pop(api->lua, 1);

    return data;
}

int
RegisterCallback(lua_State *L) {
    return luaL_ref(L, LUA_REGISTRYINDEX);
}

B8
CallCallback_(lua_State *L, ApiCallback *callback, U32 args) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, *callback);

    lua_pushvalue(L, -1);

    // Replicate while moving function behind arguments.
    for (U32 i = 0; i < args; ++i) {
        lua_pushvalue(L, -args - 2);
    }

    if (lua_pcall(L, args, 0, 0) != 0) {
        return false;
    }

    *callback = luaL_ref(L, LUA_REGISTRYINDEX);

    lua_pop(L, args);

    return true;
}

void
FreeCallback(lua_State *L, ApiCallback *callback) {
    luaL_unref(L, LUA_REGISTRYINDEX, *callback);

    *callback = 0;
}

static int
L_GetMusicTimePlayed(lua_State *L) {
    F32 time_played = p_state->condition == StateCondition_RECORDING
                          ? GetTime() - p_state->record_start
                          : GetMusicTimePlayed(p_state->music);

    lua_pushnumber(L, time_played);

    return 1;
}

static int
L_AddParameter(lua_State *L) {
    CheckArgument(L, LUA_TSTRING, 1, add_param);
    CheckArgument(L, LUA_TNUMBER, 2, add_param);
    CheckArgument(L, LUA_TNUMBER, 3, add_param);
    CheckArgument(L, LUA_TNUMBER, 4, add_param);

    const char *name = lua_tostring(L, 1);
    F32         value = lua_tonumber(L, 2);
    F32         min = lua_tonumber(L, 3);
    F32         max = lua_tonumber(L, 4);

    char *mem = ArenaPushString(&p_state->arena, name);

    Parameter param = {
        .name = mem,
        .value = value,
        .min = min,
        .max = max,
    };

    ParameterSet(p_state->parameters, &param);

    lua_pushlightuserdata(L, ParameterGet(p_state->parameters, name));

    return 1;
}

static int
L_SetParameter(lua_State *L) {
    CheckArgument(L, LUA_TSTRING, 1, set_param);
    CheckArgument(L, LUA_TNUMBER, 2, set_param);

    const char *name = lua_tostring(L, 1);
    F32         value = lua_tonumber(L, 2);

    ParameterSetValue(p_state->parameters, name, value);

    return 0;
}

static int
L_GetParameter(lua_State *L) {
    CheckArgument(L, LUA_TLIGHTUSERDATA, 1, get_param);

    Parameter *param = lua_touserdata(L, 1);

    lua_pushnumber(L, ParameterGetValue(p_state->parameters, param->name));

    return 1;
}

static int
L_PreUpdate(lua_State *L) {
    CheckArgument(L, LUA_TFUNCTION, 1, pre_update);

    p_state->api_data->pre_update[p_state->api_data->pre_update_count++] =
        RegisterCallback(L);

    return 0;
}

static int
L_OnUpdate(lua_State *L) {
    CheckArgument(L, LUA_TFUNCTION, 1, on_update);

    p_state->api_data->on_update[p_state->api_data->on_update_count++] =
        RegisterCallback(L);

    return 0;
}

static int
L_OnRender(lua_State *L) {
    CheckArgument(L, LUA_TFUNCTION, 1, on_render);

    p_state->api_data->on_render[p_state->api_data->on_render_count++] =
        RegisterCallback(L);

    return 0;
}

static int
L_PreRender(lua_State *L) {
    CheckArgument(L, LUA_TFUNCTION, 1, pre_render);

    p_state->api_data->pre_render[p_state->api_data->pre_render_count++] =
        RegisterCallback(L);

    return 0;
}

int
L_SetBgColor(lua_State *L) {
    CheckArgument(L, LUA_TTABLE, 1, set_bg_color);

    p_state->api_data->data.opt.bg_color = PopColor(L);

    return 0;
}

int
L_GetBgColor(lua_State *L) {
    PushColor(L, p_state->api_data->data.opt.bg_color);

    return 1;
}

static void
PushArray(lua_State *L, F32 *array, U32 count) {
    lua_newtable(L);
    for (U32 i = 0; i < count; ++i) {
        lua_pushnumber(L, i + 1);
        lua_pushnumber(L, array[i]);
        lua_settable(L, -3);
    }
}

static void
PopArray(lua_State *L, F32 *out, U32 count) {
    F32 array[count];

    for (U32 i = 0; i < count; ++i) {
        lua_geti(L, -1, i + 1);
        array[i] = lua_tonumber(L, -1);
        lua_pop(L, 1);
    }

    memcpy(out, array, sizeof(F32) * count);
}

static int
L_GetSamples(lua_State *L) {
    PushArray(L, p_state->samples, SAMPLE_COUNT);

    return 1;
}

static int
L_SmoothSignal(lua_State *L) {
    CheckArgument(L, LUA_TTABLE, 1, smooth_signal);

    U32 length = luaL_len(L, 1);

    F32 in[length];

    PopArray(L, in, length);

    U32 out_length;
    SignalsSmoothConvolve(NULL, length, NULL, p_state->filter_count, NULL,
                          &out_length);

    F32 out[out_length];
    SignalsSmoothConvolve(in, length, p_state->filter, p_state->filter_count,
                          out, &out_length);

    PushArray(L, out, out_length);

    return 1;
}

static int
L_GetScreenSize(lua_State *L) {
    PushVec2(L, p_state->screen_size);

    return 1;
}

void
ApiError(lua_State *L, const char *msg) {
    luaL_traceback(L, L, NULL, 1);
    printf("Error calling lua: %s\n%s\n", msg, lua_tostring(L, -1));
    lua_pop(L, 1);
    return;
}

static HMM_Vec2
PopVec2(lua_State *L) {
    HMM_Vec2 out = {};

    if (!lua_istable(L, -1)) {
        ApiError(L, "tried parsing non-table vector");
        return out;
    }

    lua_geti(L, -1, 1);
    lua_geti(L, -2, 2);

    out = HMM_V2(lua_tonumber(L, -2), lua_tonumber(L, -1));

    lua_pop(L, 3);

    return out;
}

static void
PushVec2(lua_State *L, HMM_Vec2 vec) {
    lua_newtable(L);
    {
        lua_pushstring(L, "x");
        lua_pushnumber(L, vec.X);
        lua_settable(L, -3);

        lua_pushstring(L, "y");
        lua_pushnumber(L, vec.Y);
        lua_settable(L, -3);
    }
}

// TODO(satvik): Make this user-proof
static int
L_DrawLinedPoly(lua_State *L) {
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
    }

    RendererDrawLinedPoly(p_state->renderer_data, vertices, vertex_count,
                          indices, index_count, color);

    return 0;
}

static int
L_BindShader(lua_State *L) {
    const char *fragment_shader;
    const char *vertex_shader;

    if (!lua_isstring(p_state->api_data->lua, 1)) {
        if (!lua_isnumber(L, 2)) {
            ApiErrorFunction(p_state->api_data->lua, bind_shader,
                             "received non-string and non-zero vertex shader");
            return 0;
        }

        vertex_shader = NULL;
    } else {
        vertex_shader = lua_tostring(p_state->api_data->lua, 1);
    }

    if (!lua_isstring(p_state->api_data->lua, 2)) {
        if (!lua_isnumber(L, 1)) {
            ApiErrorFunction(
                p_state->api_data->lua, bind_shader,
                "received non-string and non-zero fragment shader");
            return 0;
        }

        fragment_shader = NULL;
    } else {
        fragment_shader = lua_tostring(p_state->api_data->lua, 2);
    }

    ApiShader seek = {};

    strcpy(seek.fragment, fragment_shader);
    strcpy(seek.vertex, vertex_shader);

    ApiShader *shad =
        (ApiShader *)hashmap_get(p_state->api_data->shaders, &seek);

    if (shad == NULL) {
        seek.shader = LoadShader(vertex_shader, fragment_shader);
        hashmap_set(p_state->api_data->shaders, &seek);
    }

    shad = (ApiShader *)hashmap_get(p_state->api_data->shaders, &seek);

    assert(shad != NULL);

    BeginShaderMode(shad->shader);

    return 0;
}

static int
L_UnbindShader(lua_State *L) {
    (void)L;

    EndShaderMode();

    return 0;
}

static int
L_DrawCenteredText(lua_State *L) {
    CheckArgument(L, LUA_TSTRING, 1, draw_centered_text);
    CheckArgument(L, LUA_TTABLE, 2, draw_centered_text);
    CheckArgument(L, LUA_TNUMBER, 3, draw_centered_text);

    const char *text = lua_tostring(L, 1);
    U32         size = lua_tonumber(L, 3);

    lua_pop(L, 1);

    HMM_Vec2 pos = PopVec2(L);

    RendererDrawTextCenter(FontClosestToSize(p_state->font, size), text, pos,
                           (Color){255, 255, 255, 255});

    return 0;
}

typedef struct AnimationUpdateData {
    ApiCallback callback;
    lua_State  *lua;
} AnimationUpdateData;

static void
ApiAnimationUpdate(Animation *anim, void *user_data, F64 dt) {
    AnimationUpdateData *data = (AnimationUpdateData *)user_data;

    lua_pushlightuserdata(data->lua, anim);
    lua_pushnumber(data->lua, dt);

    CallCallback(data->lua, &data->callback, 2);
}

static int
L_AnimationGetElapsed(lua_State *L) {
    CheckArgument(L, LUA_TLIGHTUSERDATA, 1, animation_get_elapsed);

    Animation *anim = lua_touserdata(L, 1);

    if (anim) {
        lua_pushnumber(L, anim->elapsed);
    } else {
        ApiError(L, "tried getting elapsed time of non-existent animation");
        lua_pushnumber(L, 0);
    }

    return 1;
}

static int
L_AnimationGetVal(lua_State *L) {
    CheckArgument(L, LUA_TLIGHTUSERDATA, 1, animation_get_elapsed);

    Animation *anim = lua_touserdata(L, 1);

    if (anim) {
        lua_pushnumber(L, anim->val);
    } else {
        ApiError(L, "tried getting value of non-existent animation");
        lua_pushnumber(L, 0);
    }

    return 1;
}

static int
L_AnimationSetVal(lua_State *L) {
    CheckArgument(L, LUA_TLIGHTUSERDATA, 1, animation_set_val);
    CheckArgument(L, LUA_TNUMBER, 2, animation_set_val);

    Animation *anim = lua_touserdata(L, 1);
    F32        val = lua_tonumber(L, 2);

    if (anim) {
        anim->val = val;
    } else {
        ApiError(L, "tried setting value of non-existent animation");
    }

    return 0;
}

static int
L_AnimationLoad(lua_State *L) {
    CheckArgument(L, LUA_TLIGHTUSERDATA, 1, animation_load);
    CheckArgument(L, LUA_TNUMBER, 2, animation_load);

    Animation *anim = lua_touserdata(L, 1);
    F32        def = lua_tonumber(L, 2);

    if (AnimationsExists(p_state->animations, anim->name)) {
        lua_pushnumber(L, AnimationsLoad(p_state->animations, anim->name));
    } else {
        lua_pushnumber(L, def);
    }

    return 1;
}

static int
L_AnimationSetFinished(lua_State *L) {
    CheckArgument(L, LUA_TLIGHTUSERDATA, 1, animation_set_finished);

    Animation *anim = lua_touserdata(L, 1);

    if (anim) {
        anim->finished = true;
    } else {
        ApiError(L, "tried setting finished of non-existent animation");
    }

    return 0;
}

static int
L_AddAnimation(lua_State *L) {
    CheckArgument(L, LUA_TSTRING, 1, animations_add);
    CheckArgument(L, LUA_TFUNCTION, 2, animations_add);

    const char *name = lua_tostring(L, 1);

    lua_pushvalue(L, 2);
    CheckArgument(L, LUA_TFUNCTION, -1, animations_add);
    ApiCallback callback = RegisterCallback(L);

    AnimationUpdateData data = {
        .callback = callback,
        .lua = L,
    };

    Animation *anim = AnimationsAdd(p_state->animations, name, &data,
                                    ApiAnimationUpdate, &p_state->arena);

    lua_pushlightuserdata(L, anim);

    return 1;
}

typedef struct ProcedureCallbackData {
    ApiCallback callback;
    lua_State  *L;
} ProcedureCallbackData;

static void
ProcedureCallbackWrapper(void *data) {
    ProcedureCallbackData *callback_data = (ProcedureCallbackData *)data;

    CallCallback(callback_data->L, &callback_data->callback, 0);
}

static int
L_AddProcedure(lua_State *L) {
    CheckArgument(L, LUA_TSTRING, 1, add_procedure);
    CheckArgument(L, LUA_TFUNCTION, 2, add_procedure);

    const char *name = lua_tostring(L, 1);

    lua_pushvalue(L, 2);

    CheckArgument(L, LUA_TFUNCTION, -1, add_procedure);
    ApiCallback callback = RegisterCallback(L);

    ProcedureCallbackData data = {
        .callback = callback,
        .L = L,
    };

    Procedure *proc = ProcedureAdd(p_state->procedures, name, &data,
                                   ProcedureCallbackWrapper, &p_state->arena);

    lua_pushlightuserdata(L, proc);

    return 1;
}

static int
L_CallProcedure(lua_State *L) {
    CheckArgument(L, LUA_TLIGHTUSERDATA, 1, call_procedure);

    const char *name = ((Procedure *)lua_touserdata(L, 1))->name;

    ProcedureCall(p_state->procedures, name);

    return 0;
}
