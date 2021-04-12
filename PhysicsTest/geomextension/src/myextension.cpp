// myextension.cpp
// Extension lib defines
#define LIB_NAME "GeomExtension"
#define MODULE_NAME "geomextension"

// include the Defold SDK
#include <dmsdk/sdk.h>
#include <stdlib.h>

static int SEED = 0;

static int hash[] = {208,34,231,213,32,248,233,56,161,78,24,140,71,48,140,254,245,255,247,247,40,
    185,248,251,245,28,124,204,204,76,36,1,107,28,234,163,202,224,245,128,167,204,
    9,92,217,54,239,174,173,102,193,189,190,121,100,108,167,44,43,77,180,204,8,81,
    70,223,11,38,24,254,210,210,177,32,81,195,243,125,8,169,112,32,97,53,195,13,
    203,9,47,104,125,117,114,124,165,203,181,235,193,206,70,180,174,0,167,181,41,
    164,30,116,127,198,245,146,87,224,149,206,57,4,192,210,65,210,129,240,178,105,
    228,108,245,148,140,40,35,195,38,58,65,207,215,253,65,85,208,76,62,3,237,55,89,
    232,50,217,64,244,157,199,121,252,90,17,212,203,149,152,140,187,234,177,73,174,
    193,100,192,143,97,53,145,135,19,103,13,90,135,151,199,91,239,247,33,39,145,
    101,120,99,3,186,86,99,41,237,203,111,79,220,135,158,42,30,154,120,67,87,167,
    135,176,183,191,253,115,184,21,233,58,129,233,142,39,128,211,118,137,139,255,
    114,20,218,113,154,27,127,246,250,1,8,198,250,209,92,222,173,21,88,102,219
};

int noise2(int x, int y)
{
    int tmp = hash[(y + SEED) % 256];
    return hash[(tmp + x) % 256];
}

float lin_inter(float x, float y, float s)
{
    return x + s * (y-x);
}

float smooth_inter(float x, float y, float s)
{
    return lin_inter(x, y, s * s * (3-2*s));
}

float noise2d(float x, float y)
{
    int x_int = x;
    int y_int = y;
    float x_frac = x - x_int;
    float y_frac = y - y_int;
    int s = noise2(x_int, y_int);
    int t = noise2(x_int+1, y_int);
    int u = noise2(x_int, y_int+1);
    int v = noise2(x_int+1, y_int+1);
    float low = smooth_inter(s, t, x_frac);
    float high = smooth_inter(u, v, x_frac);
    return smooth_inter(low, high, y_frac);
}

float perlin2d(float x, float y, float freq, int depth)
{
    float xa = x*freq;
    float ya = y*freq;
    float amp = 1.0;
    float fin = 0;
    float div = 0.0;

    int i;
    for(i=0; i<depth; i++)
    {
        div += 256 * amp;
        fin += noise2d(xa, ya) * amp;
        amp /= 2;
        xa *= 2;
        ya *= 2;
    }

    return fin/div;
}

static int PerlinNoise( lua_State *L )
{
    DM_LUA_STACK_CHECK(L, 1);
    float x = luaL_checknumber(L, 1);
    float y = luaL_checknumber(L, 2);
    float freq = luaL_checknumber(L, 3);
    int depth = luaL_checknumber(L, 4);
    
    lua_pushnumber(L, perlin2d( x, y, freq, depth ));
    return 1;
}

static int SetBufferIntsFromTable(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    size_t offset = luaL_checknumber(L, 1);
    size_t length = luaL_checknumber(L, 2);
    const unsigned char *data = (unsigned char *)luaL_checkstring(L, 3);
    luaL_checktype(L, 4, LUA_TTABLE);

    
    // Now we have the data, cast it to the union and write back out.
    int idx = 1;
    for( int i=0; i<length; i+=sizeof(unsigned short))
    {
        unsigned int val = ((unsigned int)data[i+1+offset] << 8) | ((unsigned int)data[i+offset]);
        //printf("%d\n", val);
        lua_pushnumber(L, val);  /* value */
        lua_rawseti(L, 4, idx++);  /* set table at key `i' */
    }

    return 0;
}

static int SetBufferFloatsFromTable(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    size_t offset = luaL_checknumber(L, 1);
    size_t length = luaL_checknumber(L, 2);
    const char *data = luaL_checkstring(L, 3);
    luaL_checktype(L, 4, LUA_TTABLE);
    
    // Now we have the data, cast it to the union and write back out.
    int idx = 1;
    for( int i=0; i<length; i+= sizeof(float))
    {
        float val = *(float *)(data + i + offset);
        //printf("%f\n", val);
        lua_pushnumber(L, val);  /* value */
        lua_rawseti(L, 4, idx++);  /* set table at key `i' */
    }

    return 0;
}

static void GetTableNumbersInt( lua_State * L, int tblidx, int *data )
{
    // Iterate indices and set float buffer with correct lookups
    lua_pushnil(L);
    size_t idx = 0;
    // Build a number array matching the buffer. They are all assumed to be type float (for the time being)
    while( lua_next( L, tblidx ) != 0) {
        data[idx++] = (int)lua_tonumber( L, -1 );
        lua_pop( L, 1 );
    }
}

static void GetTableNumbersFloat( lua_State * L, int tblidx, float *data )
{
    // Iterate indices and set float buffer with correct lookups
    lua_pushnil(L);
    size_t idx = 0;
    // Build a number array matching the buffer. They are all assumed to be type float (for the time being)
    while( lua_next( L, tblidx ) != 0) {
        data[idx++] = lua_tonumber( L, -1 );
        lua_pop( L, 1 );
    }
}

static int SetBufferBytesFromTable(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    dmScript::LuaHBuffer *buffer = dmScript::CheckBuffer(L, 1);
    const char *streamname = luaL_checkstring(L, 2);
    luaL_checktype(L, 3, LUA_TTABLE);
    luaL_checktype(L, 4, LUA_TTABLE);

    float* bytes = 0x0;
    uint32_t count = 0;
    uint32_t components = 0;
    uint32_t stride = 0;
    dmBuffer::Result r = dmBuffer::GetStream(buffer->m_Buffer, dmHashString64(streamname), (void**)&bytes, &count, &components, &stride);

    if(components == 0 || count == 0) return 0;

    // This is very rudimentary.. will make nice later (maybe)    
    size_t indiceslen = lua_objlen(L, 3);
    int * idata = (int *)calloc(indiceslen, sizeof(int));    
    GetTableNumbersInt(L, 3, idata);

    size_t floatslen = lua_objlen(L, 4);
    float *floatdata = (float *)calloc(floatslen, sizeof(float));    
    GetTableNumbersFloat(L, 4, floatdata);

    if (r == dmBuffer::RESULT_OK) {
        for (int i = 0; i < count; ++i)
        {
            for (int c = 0; c < components; ++c)
            {
                bytes[c] = floatdata[idata[i] * components + c];
            }
            bytes += stride;
        }
    } else {
        // handle error
    }
    
    free(floatdata);
    free(idata);
    r = dmBuffer::ValidateBuffer(buffer->m_Buffer);
    return 0;
}


static int SetBufferBytes(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    dmScript::LuaHBuffer *buffer = dmScript::CheckBuffer(L, 1);
    const char *streamname = luaL_checkstring(L, 2);
    const char *bufferstring = luaL_checkstring(L, 3);
    
    uint8_t* bytes = 0x0;
    uint32_t size = 0;
    uint32_t count = 0;
    uint32_t components = 0;
    uint32_t stride = 0;
    dmBuffer::Result r = dmBuffer::GetStream(buffer->m_Buffer, dmHashString64(streamname), (void**)&bytes, &count, &components, &stride);

    size_t idx = 0;
    if (r == dmBuffer::RESULT_OK) {
        for (int i = 0; i < count; ++i)
        {
            for (int c = 0; c < components; ++c)
            {
                bytes[c] = bufferstring[idx++];
            }
            bytes += stride;
        }
    } else {
        // handle error
    }
        
    r = dmBuffer::ValidateBuffer(buffer->m_Buffer);
    return 0;
}

// Functions exposed to Lua
static const luaL_reg Module_methods[] =
{
    {"setbufferbytes", SetBufferBytes},
    {"setbufferbytesfromtable", SetBufferBytesFromTable},
    {"setbufferfloatsfromtable", SetBufferFloatsFromTable},
    {"setbufferintsfromtable", SetBufferIntsFromTable},
    {"perlinnoise", PerlinNoise},    // TODO move this to utils extension
    {0, 0}
};

static void LuaInit(lua_State* L)
{
    int top = lua_gettop(L);

    // Register lua names
    luaL_register(L, MODULE_NAME, Module_methods);

    lua_pop(L, 1);
    assert(top == lua_gettop(L));
}

dmExtension::Result AppInitializeGeomExtension(dmExtension::AppParams* params)
{
    dmLogInfo("AppInitializeGeomExtension\n");
    return dmExtension::RESULT_OK;
}

dmExtension::Result InitializeGeomExtension(dmExtension::Params* params)
{
    // Init Lua
    LuaInit(params->m_L);
    dmLogInfo("Registered %s Extension\n", MODULE_NAME);
    return dmExtension::RESULT_OK;
}

dmExtension::Result AppFinalizeGeomExtension(dmExtension::AppParams* params)
{
    dmLogInfo("AppFinalizeGeomExtension\n");
    return dmExtension::RESULT_OK;
}

dmExtension::Result FinalizeGeomExtension(dmExtension::Params* params)
{
    dmLogInfo("FinalizeGeomExtension\n");
    return dmExtension::RESULT_OK;
}

dmExtension::Result OnUpdateGeomExtension(dmExtension::Params* params)
{
    // dmLogInfo("OnUpdateGeomExtension\n");
    return dmExtension::RESULT_OK;
}

void OnEventGeomExtension(dmExtension::Params* params, const dmExtension::Event* event)
{
    switch(event->m_Event)
    {
        case dmExtension::EVENT_ID_ACTIVATEAPP:
            dmLogInfo("OnEventGeomExtension - EVENT_ID_ACTIVATEAPP\n");
            break;
        case dmExtension::EVENT_ID_DEACTIVATEAPP:
            dmLogInfo("OnEventGeomExtension - EVENT_ID_DEACTIVATEAPP\n");
            break;
        case dmExtension::EVENT_ID_ICONIFYAPP:
            dmLogInfo("OnEventGeomExtension - EVENT_ID_ICONIFYAPP\n");
            break;
        case dmExtension::EVENT_ID_DEICONIFYAPP:
            dmLogInfo("OnEventGeomExtension - EVENT_ID_DEICONIFYAPP\n");
            break;
        default:
            dmLogWarning("OnEventGeomExtension - Unknown event id\n");
            break;
    }
}

// Defold SDK uses a macro for setting up extension entry points:
//
// DM_DECLARE_EXTENSION(symbol, name, app_init, app_final, init, update, on_event, final)

// GeomExtension is the C++ symbol that holds all relevant extension data.
// It must match the name field in the `ext.manifest`
DM_DECLARE_EXTENSION(GeomExtension, LIB_NAME, AppInitializeGeomExtension, AppFinalizeGeomExtension, InitializeGeomExtension, OnUpdateGeomExtension, OnEventGeomExtension, FinalizeGeomExtension)
