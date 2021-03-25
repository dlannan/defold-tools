
// myextension.cpp
// Extension lib defines
#define LIB_NAME "TinyGLTFExtension"
#define MODULE_NAME "tinygltf_extension"

// include the Defold SDK
#include <dmsdk/sdk.h>

//
// TODO(syoyo): Print extensions and extras for each glTF object.
//
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

#include <cstdio>
#include <fstream>
#include <iostream>

static std::string GetFilePathExtension(const std::string &FileName) {
  if (FileName.find_last_of(".") != std::string::npos)
    return FileName.substr(FileName.find_last_of(".") + 1);
  return "";
}

// -------------------------------------------------------------------------------------
static std::string PrintMode(int mode) {
  if (mode == TINYGLTF_MODE_POINTS) {
    return "POINTS";
  } else if (mode == TINYGLTF_MODE_LINE) {
    return "LINE";
  } else if (mode == TINYGLTF_MODE_LINE_LOOP) {
    return "LINE_LOOP";
  } else if (mode == TINYGLTF_MODE_TRIANGLES) {
    return "TRIANGLES";
  } else if (mode == TINYGLTF_MODE_TRIANGLE_FAN) {
    return "TRIANGLE_FAN";
  } else if (mode == TINYGLTF_MODE_TRIANGLE_STRIP) {
    return "TRIANGLE_STRIP";
  }
  return "**UNKNOWN**";
}

// -------------------------------------------------------------------------------------
static std::string PrintTarget(int target) {
  if (target == 34962) {
    return "GL_ARRAY_BUFFER";
  } else if (target == 34963) {
    return "GL_ELEMENT_ARRAY_BUFFER";
  } else {
    return "**UNKNOWN**";
  }
}

// -------------------------------------------------------------------------------------
static std::string PrintType(int ty) {
  if (ty == TINYGLTF_TYPE_SCALAR) {
    return "SCALAR";
  } else if (ty == TINYGLTF_TYPE_VECTOR) {
    return "VECTOR";
  } else if (ty == TINYGLTF_TYPE_VEC2) {
    return "VEC2";
  } else if (ty == TINYGLTF_TYPE_VEC3) {
    return "VEC3";
  } else if (ty == TINYGLTF_TYPE_VEC4) {
    return "VEC4";
  } else if (ty == TINYGLTF_TYPE_MATRIX) {
    return "MATRIX";
  } else if (ty == TINYGLTF_TYPE_MAT2) {
    return "MAT2";
  } else if (ty == TINYGLTF_TYPE_MAT3) {
    return "MAT3";
  } else if (ty == TINYGLTF_TYPE_MAT4) {
    return "MAT4";
  }
  return "**UNKNOWN**";
}

// -------------------------------------------------------------------------------------
static std::string PrintComponentType(int ty) {
  if (ty == TINYGLTF_COMPONENT_TYPE_BYTE) {
    return "BYTE";
  } else if (ty == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
    return "UNSIGNED_BYTE";
  } else if (ty == TINYGLTF_COMPONENT_TYPE_SHORT) {
    return "SHORT";
  } else if (ty == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
    return "UNSIGNED_SHORT";
  } else if (ty == TINYGLTF_COMPONENT_TYPE_INT) {
    return "INT";
  } else if (ty == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
    return "UNSIGNED_INT";
  } else if (ty == TINYGLTF_COMPONENT_TYPE_FLOAT) {
    return "FLOAT";
  } else if (ty == TINYGLTF_COMPONENT_TYPE_DOUBLE) {
    return "DOUBLE";
  }

  return "**UNKNOWN**";
}

// -------------------------------------------------------------------------------------
#if 0
static std::string PrintParameterType(int ty) {
  if (ty == TINYGLTF_PARAMETER_TYPE_BYTE) {
    return "BYTE";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE) {
    return "UNSIGNED_BYTE";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_SHORT) {
    return "SHORT";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT) {
    return "UNSIGNED_SHORT";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_INT) {
    return "INT";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT) {
    return "UNSIGNED_INT";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_FLOAT) {
    return "FLOAT";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_FLOAT_VEC2) {
    return "FLOAT_VEC2";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_FLOAT_VEC3) {
    return "FLOAT_VEC3";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_FLOAT_VEC4) {
    return "FLOAT_VEC4";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_INT_VEC2) {
    return "INT_VEC2";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_INT_VEC3) {
    return "INT_VEC3";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_INT_VEC4) {
    return "INT_VEC4";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_BOOL) {
    return "BOOL";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_BOOL_VEC2) {
    return "BOOL_VEC2";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_BOOL_VEC3) {
    return "BOOL_VEC3";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_BOOL_VEC4) {
    return "BOOL_VEC4";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_FLOAT_MAT2) {
    return "FLOAT_MAT2";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_FLOAT_MAT3) {
    return "FLOAT_MAT3";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_FLOAT_MAT4) {
    return "FLOAT_MAT4";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_SAMPLER_2D) {
    return "SAMPLER_2D";
  }

  return "**UNKNOWN**";
}
#endif

// -------------------------------------------------------------------------------------
static std::string PrintWrapMode(int mode) {
  if (mode == TINYGLTF_TEXTURE_WRAP_REPEAT) {
    return "REPEAT";
  } else if (mode == TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE) {
    return "CLAMP_TO_EDGE";
  } else if (mode == TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT) {
    return "MIRRORED_REPEAT";
  }

  return "**UNKNOWN**";
}

// -------------------------------------------------------------------------------------
static std::string PrintFilterMode(int mode) {
  if (mode == TINYGLTF_TEXTURE_FILTER_NEAREST) {
    return "NEAREST";
  } else if (mode == TINYGLTF_TEXTURE_FILTER_LINEAR) {
    return "LINEAR";
  } else if (mode == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST) {
    return "NEAREST_MIPMAP_NEAREST";
  } else if (mode == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR) {
    return "NEAREST_MIPMAP_LINEAR";
  } else if (mode == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST) {
    return "LINEAR_MIPMAP_NEAREST";
  } else if (mode == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR) {
    return "LINEAR_MIPMAP_LINEAR";
  }
  return "**UNKNOWN**";
}

// -------------------------------------------------------------------------------------
static void tableNew(lua_State* L, const char * name) {
  lua_pushstring( L, name );   
  lua_newtable( L );            
}

// -------------------------------------------------------------------------------------
static void tableNewInt(lua_State* L, int idx) {
  lua_pushnumber( L, idx );   
  lua_newtable( L );            
}

// -------------------------------------------------------------------------------------
static void tableClose(lua_State* L) {
  lua_settable( L, -3 );
}

// -------------------------------------------------------------------------------------
static void tableKVStr(lua_State* L, const char * key, const char * value) {
  lua_pushstring( L, key );
  lua_pushstring( L, value );  
  lua_settable( L, -3 );
}

// -------------------------------------------------------------------------------------
static void tableKVInt(lua_State* L, const char * key, int value) {
  lua_pushstring( L, key );
  lua_pushnumber( L, value );  
  lua_settable( L, -3 );
}

// -------------------------------------------------------------------------------------
static void tableKIntVInt(lua_State* L, int key, int value) {
  lua_pushnumber( L, key );
  lua_pushnumber( L, value );  
  lua_settable( L, -3 );
}

// -------------------------------------------------------------------------------------
static void tableKVDouble(lua_State* L, const char * key, double value) {
  lua_pushstring( L, key );
  lua_pushnumber( L, value );  
  lua_settable( L, -3 );
}

// -------------------------------------------------------------------------------------
static void tableKIntVDouble(lua_State* L, int key, double value) {
  lua_pushnumber( L, key );
  lua_pushnumber( L, value );  
  lua_settable( L, -3 );
}

// -------------------------------------------------------------------------------------
static void parseScenes( lua_State *L, tinygltf::Model &model) 
{
  tableNew(L, "scenes");

  for (size_t i = 0; i < model.scenes.size(); i++) {

    tableNewInt(L, i+1);            
    tableKVStr( L, "name", model.scenes[i].name.c_str() );

    tableNew(L, "nodes");
    for( int j = 0; j < model.scenes[i].nodes.size(); j++ ) {
      tableKIntVInt( L, j+1, model.scenes[i].nodes[j]+1 );
    }
    tableClose(L); // nodes
    
    tableClose(L); // scene index
  }  

  tableClose(L); // scenes
}

// -------------------------------------------------------------------------------------

static void parseMeshes( lua_State *L, tinygltf::Model &model)
{
  tableNew(L, "meshes");

  for (size_t i = 0; i < model.meshes.size(); i++) {

    tableNewInt(L, i+1);            
    tableKVStr( L, "name", model.meshes[i].name.c_str() );

    tableNew(L, "primitives");

    for( int j = 0; j < model.meshes[i].primitives.size(); j++ ) {

      tableNewInt(L, j+1);

      tableKVInt( L, "material", model.meshes[i].primitives[j].material );
      tableKVInt( L, "indices", model.meshes[i].primitives[j].indices + 1); // This points to an accessor!
      tableKVInt( L, "mode", model.meshes[i].primitives[j].mode );

      tableNew(L, "attribs");

      const std::map<std::string, int> &m = model.meshes[i].primitives[j].attributes;
      std::map<std::string, int>::const_iterator it(m.begin());
      std::map<std::string, int>::const_iterator itEnd(m.end());
      for (; it != itEnd; it++) {
        tableKVInt( L, it->first.c_str(), it->second + 1);
      }      

      tableClose(L); // attribs

      tableClose(L); // primitive index
    }

    tableClose(L); // primitives

    tableClose(L); // meshes index
  }

  tableClose(L); // meshes
}

// -------------------------------------------------------------------------------------

static void parseAccessors( lua_State *L, tinygltf::Model &model) 
{
  tableNew(L, "accessors");

  for (size_t i = 0; i < model.accessors.size(); i++) {

    tableNewInt(L, i+1);

    const tinygltf::Accessor &accessor = model.accessors[i];

    tableKVStr(L, "name", accessor.name.c_str());
    tableKVInt(L, "bufferView", accessor.bufferView + 1);
    tableKVInt(L, "byteOffset", accessor.byteOffset);

    tableKVInt(L, "componentType", accessor.componentType);
    tableKVInt(L, "count", accessor.count);
    tableKVInt(L, "type", accessor.type);    

    if (!accessor.minValues.empty()) {
      tableNew(L, "min");
      for (size_t k = 0; k < accessor.minValues.size(); k++) {
        tableKIntVInt( L, k+1, accessor.minValues[k] );
      }
      tableClose(L); // min
    }
    if (!accessor.maxValues.empty()) {
      tableNew(L, "max");
      for (size_t k = 0; k < accessor.maxValues.size(); k++) {
        tableKIntVInt( L, k+1, accessor.minValues[k] );
      }
      tableClose(L); // max
    }

    if (accessor.sparse.isSparse) {

      tableNew(L, "sparse");      
      tableKVInt(L, "count", accessor.sparse.count);

      tableNew(L, "indices");
      tableKVInt(L, "bufferView", accessor.sparse.indices.bufferView + 1);
      tableKVInt(L, "byteOffset", accessor.sparse.indices.byteOffset);
      tableKVInt(L, "componentType", accessor.sparse.indices.componentType);

      tableClose(L); // indices
      tableNew(L, "values");
      tableKVInt(L, "bufferView", accessor.sparse.values.bufferView + 1);
      tableKVInt(L, "byteOffset", accessor.sparse.values.byteOffset);
      tableClose(L); // values

      tableClose(L); // sparse
    }

    tableClose(L); // accessor index
  }

  tableClose(L); // accessors
}

// -------------------------------------------------------------------------------------

static void parseBufferViews( lua_State *L, tinygltf::Model &model)
{
  tableNew( L, "bufferviews" );   

  for (size_t i = 0; i < model.bufferViews.size(); i++) {

    tableNewInt( L, i + 1 );   

    const tinygltf::BufferView &bufferView = model.bufferViews[i];

    tableKVStr( L, "name", bufferView.name.c_str() );  
    tableKVInt( L, "buffer", bufferView.buffer + 1 );  
    tableKVInt( L, "byteLength", bufferView.byteLength );  
    tableKVInt( L, "byteOffset", bufferView.byteOffset ); 
    tableKVInt( L, "byteStride", bufferView.byteStride );  

    tableClose(L); // bufferView index
  }
  tableClose(L); // bufferviews  
}

// -------------------------------------------------------------------------------------

static void parseBuffers( lua_State *L, tinygltf::Model &model) 
{
  tableNew( L, "buffers" );   

  for (size_t i = 0; i < model.buffers.size(); i++) {
    const tinygltf::Buffer &buffer = model.buffers[i];

    tableNewInt( L, i + 1 );   

    tableKVStr( L, "name", buffer.name.c_str() );  
    tableKVInt( L, "byteLength", buffer.data.size() );  
    tableNew(L, "data");

    // This is an unsigned char buffer anyway. Post process in lua + ffi or data conv
    for (size_t j = 0; j < buffer.data.size(); j++) {
      tableKIntVInt( L, j+1, buffer.data[j] );
    }
    tableClose(L); // buffer data
    
    tableClose(L); // buffers index
  }
  tableClose(L); // buffers
}

// -------------------------------------------------------------------------------------

static void parseNodes( lua_State *L, tinygltf::Model &model) 
{
  tableNew(L, "nodes");
  for (size_t i = 0; i < model.nodes.size(); i++) {

    tableNewInt(L, i+1);
    const tinygltf::Node &node = model.nodes[i];

    tableKVStr(L, "name", node.name.c_str());
    tableKVInt(L, "camera", node.camera+1);
    tableKVInt(L, "mesh", node.mesh+1);
    if (!node.rotation.empty()) {
      tableNew(L, "rotation");
      tableKVDouble(L, "x", node.rotation[0]);
      tableKVDouble(L, "y", node.rotation[1]);
      tableKVDouble(L, "z", node.rotation[2]);
      tableClose(L); // rotation
    }

    if (!node.scale.empty()) {
      tableNew(L, "scale");
      tableKVDouble(L, "x", node.scale[0]);
      tableKVDouble(L, "y", node.scale[1]);
      tableKVDouble(L, "z", node.scale[2]);
      tableClose(L); // scale
    }
        
    if (!node.translation.empty()) {
      tableNew(L, "translation");
      tableKVDouble(L, "x", node.translation[0]);
      tableKVDouble(L, "y", node.translation[1]);
      tableKVDouble(L, "z", node.translation[2]);
      tableClose(L); // translation
    }

    if (!node.matrix.empty()) {
      tableNew(L, "matrix");
      for (size_t i = 0; i < node.matrix.size(); i++) {
        tableKIntVDouble(L, i+1, node.matrix[i]);
      }
      tableClose(L); // matrix
    }

    tableNew(L, "children");
    for (size_t i = 0; i < node.children.size(); i++) {
      tableKIntVInt(L, i+1, node.children[i] + 1);
    }
    tableClose(L);
    
    tableClose(L);  // nodes index
  }  
  tableClose(L); // nodes
}
  
// -------------------------------------------------------------------------------------

static int LoadModel(lua_State* L)
{
  // The number of expected items to be on the Lua stack
  // once this struct goes out of scope
  DM_LUA_STACK_CHECK(L, 1);

  char *fname = (char*)luaL_checkstring(L, 1);
  
  // Store original JSON string for `extras` and `extensions`
  bool store_original_json_for_extras_and_extensions = false;

  tinygltf::Model model;
  tinygltf::TinyGLTF gltf_ctx;
  std::string err;
  std::string warn;
  std::string input_filename(fname);
  std::string ext = GetFilePathExtension(input_filename);

  gltf_ctx.SetStoreOriginalJSONForExtrasAndExtensions(
      store_original_json_for_extras_and_extensions);

  bool ret = false;
  if (ext.compare("glb") == 0) {
    std::cout << "Reading binary glTF" << std::endl;
    // assume binary glTF.
    ret = gltf_ctx.LoadBinaryFromFile(&model, &err, &warn,
                                      input_filename);
  } else {
    std::cout << "Reading ASCII glTF" << std::endl;
    // assume ascii glTF.
    ret =
        gltf_ctx.LoadASCIIFromFile(&model, &err, &warn, input_filename.c_str());
  }

  if (!warn.empty()) {
    printf("Warn: %s\n", warn.c_str());
  }

  if (!err.empty()) {
    printf("Err: %s\n", err.c_str());
  }

  if (!ret) {
    printf("Failed to parse glTF\n");
    return 0;
  }

  // This is the main table being returned  
  lua_newtable( L );

  parseScenes( L, model );
  parseMeshes( L, model );  
  parseAccessors( L, model );
  parseBufferViews( L, model );
  parseBuffers( L, model );
  parseNodes( L, model );
  
  return 1;
}

// Functions exposed to Lua
static const luaL_reg Module_methods[] =
{
  {"loadmodel", LoadModel},
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

dmExtension::Result AppInitializeTinyGLTFExtension(dmExtension::AppParams* params)
{
  return dmExtension::RESULT_OK;
}

dmExtension::Result InitializeTinyGLTFExtension(dmExtension::Params* params)
{
  // Init Lua
  LuaInit(params->m_L);
  printf("Registered %s Extension\n", MODULE_NAME);
  return dmExtension::RESULT_OK;
}

dmExtension::Result AppFinalizeTinyGLTFExtension(dmExtension::AppParams* params)
{
  return dmExtension::RESULT_OK;
}

dmExtension::Result FinalizeTinyGLTFExtension(dmExtension::Params* params)
{
  return dmExtension::RESULT_OK;
}


// Defold SDK uses a macro for setting up extension entry points:
//
// DM_DECLARE_EXTENSION(symbol, name, app_init, app_final, init, update, on_event, final)

// TinyGLTFExtension is the C++ symbol that holds all relevant extension data.
// It must match the name field in the `ext.manifest`
DM_DECLARE_EXTENSION(TinyGLTFExtension, LIB_NAME, AppInitializeTinyGLTFExtension, AppFinalizeTinyGLTFExtension, InitializeTinyGLTFExtension, 0, 0, FinalizeTinyGLTFExtension)
