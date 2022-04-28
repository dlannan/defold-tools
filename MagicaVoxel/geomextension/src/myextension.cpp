// myextension.cpp
// Extension lib defines
#define LIB_NAME "GeomExtension"
#define MODULE_NAME "geomextension"

// include the Defold SDK
#include <dmsdk/sdk.h>
#include <stdlib.h>

#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>

// #include "lua.h"
// #include "lauxlib.h"

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

/*
** {======================================================
** Library for packing/unpacking structures.
** $Id: struct.c,v 1.8 2018/05/16 11:00:23 roberto Exp $
** See Copyright Notice at the end of this file
** =======================================================
*/
/*
** Valid formats:
** > - big endian
** < - little endian
** ![num] - alignment
** x - pading
** b/B - signed/unsigned byte
** h/H - signed/unsigned short
** l/L - signed/unsigned long
** T   - size_t
** i/In - signed/unsigned integer with size 'n' (default is size of int)
** cn - sequence of 'n' chars (from/to a string); when packing, n==0 means
        the whole string; when unpacking, n==0 means use the previous
        read number as the string length
** s - zero-terminated string
** f - float
** d - double
** ' ' - ignored
*/

/* basic integer type */
#if !defined(STRUCT_INT)
#define STRUCT_INT	long
#endif

typedef STRUCT_INT Inttype;

/* corresponding unsigned version */
typedef unsigned STRUCT_INT Uinttype;


/* maximum size (in bytes) for integral types */
#define MAXINTSIZE	32

/* is 'x' a power of 2? */
#define isp2(x)		((x) > 0 && ((x) & ((x) - 1)) == 0)

/* dummy structure to get alignment requirements */
struct cD {
  char c;
  double d;
};


#define PADDING		(sizeof(struct cD) - sizeof(double))
#define MAXALIGN  	(PADDING > sizeof(int) ? PADDING : sizeof(int))


/* endian options */
#define BIG	0
#define LITTLE	1


static union {
  int dummy;
  char endian;
} const native = {1};


typedef struct Header {
  int endian;
  int align;
} Header;


static int getnum (const char **fmt, int df) {
  if (!isdigit(**fmt))  /* no number? */
    return df;  /* return default value */
  else {
    int a = 0;
    do {
      a = a*10 + *((*fmt)++) - '0';
    } while (isdigit(**fmt));
    return a;
  }
}


#define defaultoptions(h)	((h)->endian = native.endian, (h)->align = 1)



static size_t optsize (lua_State *L, char opt, const char **fmt) {
  switch (opt) {
    case 'B': case 'b': return sizeof(char);
    case 'H': case 'h': return sizeof(short);
    case 'L': case 'l': return sizeof(long);
    case 'T': return sizeof(size_t);
    case 'f':  return sizeof(float);
    case 'd':  return sizeof(double);
    case 'x': return 1;
    case 'c': return getnum(fmt, 1);
    case 'i': case 'I': {
      int sz = getnum(fmt, sizeof(int));
      if (sz > MAXINTSIZE)
        luaL_error(L, "integral size %d is larger than limit of %d",
                       sz, MAXINTSIZE);
      return sz;
    }
    default: return 0;  /* other cases do not need alignment */
  }
}


/*
** return number of bytes needed to align an element of size 'size'
** at current position 'len'
*/
static int gettoalign (size_t len, Header *h, int opt, size_t size) {
  if (size == 0 || opt == 'c') return 0;
  if (size > (size_t)h->align)
    size = h->align;  /* respect max. alignment */
  return (size - (len & (size - 1))) & (size - 1);
}


/*
** options to control endianess and alignment
*/
static void controloptions (lua_State *L, int opt, const char **fmt,
                            Header *h) {
  switch (opt) {
    case  ' ': return;  /* ignore white spaces */
    case '>': h->endian = BIG; return;
    case '<': h->endian = LITTLE; return;
    case '!': {
      int a = getnum(fmt, MAXALIGN);
      if (!isp2(a))
        luaL_error(L, "alignment %d is not a power of 2", a);
      h->align = a;
      return;
    }
    default: {
      const char *msg = lua_pushfstring(L, "invalid format option '%c'", opt);
      luaL_argerror(L, 1, msg);
    }
  }
}


static void putinteger (lua_State *L, luaL_Buffer *b, int arg, int endian,
                        int size) {
  lua_Number n = luaL_checknumber(L, arg);
  Uinttype value;
  char buff[MAXINTSIZE];
  if (n < 0)
    value = (Uinttype)(Inttype)n;
  else
    value = (Uinttype)n;
  if (endian == LITTLE) {
    int i;
    for (i = 0; i < size; i++) {
      buff[i] = (value & 0xff);
      value >>= 8;
    }
  }
  else {
    int i;
    for (i = size - 1; i >= 0; i--) {
      buff[i] = (value & 0xff);
      value >>= 8;
    }
  }
  luaL_addlstring(b, buff, size);
}


static void correctbytes (char *b, int size, int endian) {
  if (endian != native.endian) {
    int i = 0;
    while (i < --size) {
      char temp = b[i];
      b[i++] = b[size];
      b[size] = temp;
    }
  }
}


static int b_pack (lua_State *L) {
  luaL_Buffer b;
  const char *fmt = luaL_checkstring(L, 1);
  Header h;
  int arg = 2;
  size_t totalsize = 0;
  defaultoptions(&h);
  lua_pushnil(L);  /* mark to separate arguments from string buffer */
  luaL_buffinit(L, &b);
  while (*fmt != '\0') {
    int opt = *fmt++;
    size_t size = optsize(L, opt, &fmt);
    int toalign = gettoalign(totalsize, &h, opt, size);
    totalsize += toalign;
    while (toalign-- > 0) luaL_addchar(&b, '\0');
    switch (opt) {
      case 'b': case 'B': case 'h': case 'H':
      case 'l': case 'L': case 'T': case 'i': case 'I': {  /* integer types */
        putinteger(L, &b, arg++, h.endian, size);
        break;
      }
      case 'x': {
        luaL_addchar(&b, '\0');
        break;
      }
      case 'f': {
        float f = (float)luaL_checknumber(L, arg++);
        correctbytes((char *)&f, size, h.endian);
        luaL_addlstring(&b, (char *)&f, size);
        break;
      }
      case 'd': {
        double d = luaL_checknumber(L, arg++);
        correctbytes((char *)&d, size, h.endian);
        luaL_addlstring(&b, (char *)&d, size);
        break;
      }
      case 'c': case 's': {
        size_t l;
        const char *s = luaL_checklstring(L, arg++, &l);
        if (size == 0) size = l;
        luaL_argcheck(L, l >= (size_t)size, arg, "string too short");
        luaL_addlstring(&b, s, size);
        if (opt == 's') {
          luaL_addchar(&b, '\0');  /* add zero at the end */
          size++;
        }
        break;
      }
      default: controloptions(L, opt, &fmt, &h);
    }
    totalsize += size;
  }
  luaL_pushresult(&b);
  return 1;
}


static lua_Number getinteger (const char *buff, int endian,
                        int issigned, int size) {
  Uinttype l = 0;
  int i;
  if (endian == BIG) {
    for (i = 0; i < size; i++) {
      l <<= 8;
      l |= (Uinttype)(unsigned char)buff[i];
    }
  }
  else {
    for (i = size - 1; i >= 0; i--) {
      l <<= 8;
      l |= (Uinttype)(unsigned char)buff[i];
    }
  }
  if (!issigned)
    return (lua_Number)l;
  else {  /* signed format */
    Uinttype mask = (Uinttype)(~((Uinttype)0)) << (size*8 - 1);
    if (l & mask)  /* negative value? */
      l |= mask;  /* signal extension */
    return (lua_Number)(Inttype)l;
  }
}


static int b_unpack (lua_State *L) {
  Header h;
  const char *fmt = luaL_checkstring(L, 1);
  size_t ld;
  const char *data = luaL_checklstring(L, 2, &ld);
  size_t pos = (size_t)luaL_optinteger(L, 3, 1) - 1;
  int n = 0;  /* number of results */
  luaL_argcheck(L, pos <= ld, 3, "initial position out of string");
  defaultoptions(&h);
  while (*fmt) {
    int opt = *fmt++;
    size_t size = optsize(L, opt, &fmt);
    pos += gettoalign(pos, &h, opt, size);
    luaL_argcheck(L, size <= ld - pos, 2, "data string too short");
    /* stack space for item + next position */
    luaL_checkstack(L, 2, "too many results");
    switch (opt) {
      case 'b': case 'B': case 'h': case 'H':
      case 'l': case 'L': case 'T': case 'i':  case 'I': {  /* integer types */
        int issigned = islower(opt);
        lua_Number res = getinteger(data+pos, h.endian, issigned, size);
        lua_pushnumber(L, res); n++;
        break;
      }
      case 'x': {
        break;
      }
      case 'f': {
        float f;
        memcpy(&f, data+pos, size);
        correctbytes((char *)&f, sizeof(f), h.endian);
        lua_pushnumber(L, f); n++;
        break;
      }
      case 'd': {
        double d;
        memcpy(&d, data+pos, size);
        correctbytes((char *)&d, sizeof(d), h.endian);
        lua_pushnumber(L, d); n++;
        break;
      }
      case 'c': {
        if (size == 0) {
          if (n == 0 || !lua_isnumber(L, -1))
            luaL_error(L, "format 'c0' needs a previous size");
          size = lua_tonumber(L, -1);
          lua_pop(L, 1); n--;
          luaL_argcheck(L, size <= ld - pos, 2, "data string too short");
        }
        lua_pushlstring(L, data+pos, size); n++;
        break;
      }
      case 's': {
        const char *e = (const char *)memchr(data+pos, '\0', ld - pos);
        if (e == NULL)
          luaL_error(L, "unfinished string in data");
        size = (e - (data+pos)) + 1;
        lua_pushlstring(L, data+pos, size - 1); n++;
        break;
      }
      default: controloptions(L, opt, &fmt, &h);
    }
    pos += size;
  }
  lua_pushinteger(L, pos + 1);  /* next position */
  return n + 1;
}


static int b_size (lua_State *L) {
  Header h;
  const char *fmt = luaL_checkstring(L, 1);
  size_t pos = 0;
  defaultoptions(&h);
  while (*fmt) {
    int opt = *fmt++;
    size_t size = optsize(L, opt, &fmt);
    pos += gettoalign(pos, &h, opt, size);
    if (opt == 's')
      luaL_argerror(L, 1, "option 's' has no fixed size");
    else if (opt == 'c' && size == 0)
      luaL_argerror(L, 1, "option 'c0' has no fixed size");
    if (!isalnum(opt))
      controloptions(L, opt, &fmt, &h);
    pos += size;
  }
  lua_pushinteger(L, pos);
  return 1;
}

/* }====================================================== */

// Functions exposed to Lua
static const luaL_reg Module_methods[] =
{
    {"setbufferbytes", SetBufferBytes},
    {"setbufferbytesfromtable", SetBufferBytesFromTable},
    {"setbufferfloatsfromtable", SetBufferFloatsFromTable},
    {"setbufferintsfromtable", SetBufferIntsFromTable},
    {"perlinnoise", PerlinNoise},    // TODO move this to utils extension
    {"pack",         b_pack},
    {"unpack",       b_unpack},
    {"size",         b_size},

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


/******************************************************************************
* Copyright (C) 2010-2018 Lua.org, PUC-Rio.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/
