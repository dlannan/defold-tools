// myextension.cpp
// Extension lib defines
#define LIB_NAME "NewtonExtension"
#define MODULE_NAME "newtonextension"

// include the Defold SDK
#include <dmsdk/sdk.h>
#include <stdlib.h>
#include <Newton.h>
#include <vector>

static NewtonWorld* world = NULL;

static std::vector<NewtonBody* >bodies;
static std::vector<NewtonCollision*> colls;
static std::vector<NewtonMesh* >meshes;

// Define a custom data structure to store a body ID.
struct UserData {
    int bodyID=0;
};

enum ShapeType {

    Shape_Plane           = 1,
    Shape_Cube            ,
    Shape_Sphere          ,
    Shape_Cone            , 
    Shape_Capsule         , 
    Shape_Cylinder        ,
    Shape_ChamferCylinder ,
    Shape_ConvexHull      
}; 

static int SetTableVector( lua_State *L, dFloat *data, const char *name )
{
    lua_pushstring(L, name); 
    lua_newtable(L);

    lua_pushstring(L, "x"); 
    lua_pushnumber(L, data[0]);
    lua_settable(L, -3);
    lua_pushstring(L, "y"); 
    lua_pushnumber(L, data[1]);
    lua_settable(L, -3);
    lua_pushstring(L, "z"); 
    lua_pushnumber(L, data[2]);
    lua_settable(L, -3);
    lua_pushstring(L, "w"); 
    lua_pushnumber(L, data[3]);
    lua_settable(L, -3);

    lua_settable(L, -3);
   
    return 0;
}

static void AddTableIndices( lua_State *L, int count, int *indices )
{
    lua_newtable(L);
    int * idxptr = (int *)indices;
    for (int i=1; i<=count; ++i) {
        lua_pushnumber(L, i); 
        lua_pushnumber(L, *idxptr++);
        lua_rawset(L, -3);
    }
}

static void AddTableVertices( lua_State *L, int count, const double *vertices )
{
    lua_newtable(L);
    double * vertptr = (double *)vertices;
    int idx = 1;
    for(int ctr = 0; ctr < count; ++ctr) {
        if(ctr % 4 == 3) { 
            vertptr++;
        } else {
            lua_pushnumber(L, idx++); 
            lua_pushnumber(L, *vertptr++);
            lua_rawset(L, -3);
        }
    }
}


static void AddTableUVs( lua_State *L, int count, const double *uvs )
{
    lua_newtable(L);
    double * uvsptr = (double *)uvs;
    for (int i=1; i<=count; ++i) {
        lua_pushnumber(L, i); 
        lua_pushnumber(L, *uvsptr++);
        lua_rawset(L, -3);
    }
}

static void AddTableNormals( lua_State *L, int count, const double *normals )
{
    lua_newtable(L);
    double * normptr = (double *)normals;
    for (int i=1; i<=count; ++i) {
        lua_pushnumber(L, i); 
        lua_pushnumber(L, *normptr++);
        lua_rawset(L, -3);
    }
}



void cb_applyForce(const NewtonBody* const body, dFloat timestep, int threadIndex)
{
    // Fetch user data and body position.
    //UserData *mydata = (UserData*)NewtonBodyGetUserData(body);
    //dFloat pos[4];
    //NewtonBodyGetPosition(body, pos);

    // Apply force.
    dFloat force[3] = {0, -9.8, 0};
    NewtonBodySetForce(body, force);

    // Print info to terminal.
    //printf("BodyID=%d, Sleep=%d, %.2f, %.2f, %.2f\n",
    //mydata->bodyID, NewtonBodyGetSleepState(body), pos[0], pos[1], pos[2]);
}

static int addCollisionSphere( lua_State * L ) {

    double radii = lua_tonumber(L, 1);
    // Collision shapes: sphere (our ball), and large box (our ground plane).
    NewtonCollision* cs_object = NewtonCreateSphere(world, radii, Shape_Sphere, NULL);
    colls.push_back( cs_object );
    lua_pushnumber(L, colls.size()-1);
    return 1;
}

static int addCollisionPlane( lua_State * L ) {

    double width = lua_tonumber(L, 1);
    double depth = lua_tonumber(L, 2);
    // Collision shapes: sphere (our ball), and large box (our ground plane).
    NewtonCollision* cs_object = NewtonCreateBox(world, width, 0.1, depth, Shape_Plane, NULL);
    colls.push_back( cs_object );
    lua_pushnumber(L, colls.size() - 1);
    return 1;
}

static int addCollisionCube( lua_State * L ) {

    double sx = lua_tonumber(L, 1);
    double sy = lua_tonumber(L, 2);
    double sz = lua_tonumber(L, 3);
    NewtonCollision* cs_object = NewtonCreateBox(world, sx, sy, sz, Shape_Cube, NULL);
    colls.push_back( cs_object );
    lua_pushnumber(L, colls.size() - 1);
    return 1;
}

static int addCollisionCone( lua_State * L ) {

    double radius = lua_tonumber(L, 1);
    double height = lua_tonumber(L, 2);
    NewtonCollision* cs_object = NewtonCreateCone(world, radius, height, Shape_Cone, NULL);
    colls.push_back( cs_object );
    lua_pushnumber(L, colls.size() - 1);
    return 1;
}

static int addCollisionCapsule( lua_State * L ) {

    double r0 = lua_tonumber(L, 1);
    double r1 = lua_tonumber(L, 2);
    double height = lua_tonumber(L, 3);
    NewtonCollision* cs_object = NewtonCreateCapsule(world, r0, r1, height, Shape_Capsule, NULL);
    colls.push_back( cs_object );
    lua_pushnumber(L, colls.size() - 1);
    return 1;
}

static int addCollisionCylinder( lua_State * L ) {

    double r0 = lua_tonumber(L, 1);
    double r1 = lua_tonumber(L, 2);
    double height = lua_tonumber(L, 3);
    NewtonCollision* cs_object = NewtonCreateCylinder(world, r0, r1, height, Shape_Cylinder, NULL);
    colls.push_back( cs_object );
    lua_pushnumber(L, colls.size() - 1);
    return 1;
}

static int addCollisionChamferCylinder( lua_State * L ) {

    double radius = lua_tonumber(L, 1);
    double height = lua_tonumber(L, 2);
    NewtonCollision* cs_object = NewtonCreateChamferCylinder(world, radius, height,Shape_ChamferCylinder, NULL);
    colls.push_back( cs_object );
    lua_pushnumber(L, colls.size() - 1);
    return 1;
}

static int addCollisionConvexHull( lua_State * L ) {

    double count = lua_tonumber(L, 1);
    int stride = lua_tonumber(L, 2);
    double tolerance = lua_tonumber(L, 3);
    const float *vertCloud = (float *)lua_topointer(L, 4);
    NewtonCollision* cs_object = NewtonCreateConvexHull(world, count, vertCloud, stride, tolerance, Shape_ConvexHull, NULL);
    colls.push_back( cs_object );
    lua_pushnumber(L, colls.size() - 1);
    return 1;
}

static int worldRayCast( lua_State *L ) {

    const dFloat *p0= (dFloat *)lua_topointer(L, 1);
    const dFloat *p1 = (dFloat *)lua_topointer(L, 2);
    NewtonWorldRayFilterCallback filter_cb = *(NewtonWorldRayFilterCallback *)lua_topointer(L, 3);
    void * const userData = (void * const)lua_topointer(L, 4);
    NewtonWorldRayPrefilterCallback prefilter_cb = *(NewtonWorldRayPrefilterCallback *)lua_topointer(L, 5);
        
    NewtonWorldRayCast( world, p0, p1, filter_cb, userData, prefilter_cb, 0);
    return 0;
}

static int addBody( lua_State *L ) {

    // Neutral transform matrix.
    float	tm[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    int idx = lua_tonumber(L, 1);
    double x = lua_tonumber(L, 2);
    double y = lua_tonumber(L, 3);
    double z = lua_tonumber(L, 4);
    double mass = lua_tonumber(L, 5);

    tm[12] = x; tm[13] = y; tm[14] = z;
    NewtonBody *body = NewtonCreateDynamicBody(world, colls[idx], tm);
    bodies.push_back(body);
    NewtonBodySetForceAndTorqueCallback(body, cb_applyForce);

    // Assign non-zero mass to sphere to make it dynamic.
    NewtonBodySetMassMatrix(body, mass, 1, 1, 1);
    
    UserData *myData = new UserData[2];
    myData[0].bodyID = bodies.size()-1;
    NewtonBodySetUserData(body, (void *)&myData[0]);
    
    lua_pushnumber(L, bodies.size() - 1);
    return 1;
}

static int Create( lua_State *L )
{
    // Print the library version.
    printf("Hello, this is Newton version %d\n", NewtonWorldGetVersion());
    // Create the Newton world.
    world = NewtonCreate();
    NewtonInvalidateCache(world);
    return 0;
}

static int createMeshFromCollision( lua_State *L )
{
    int collindex = lua_tonumber(L, 1);
    if(collindex < 0 || collindex > colls.size()-1) {
        lua_pushnil(L);
        return 1;
    }
    const NewtonCollision *collision = colls[collindex];

    NewtonMesh *mesh = NewtonMeshCreateFromCollision( collision );
    if(mesh) {
        meshes.push_back(mesh);
        lua_pushnumber(L, meshes.size() - 1);
        // save the polygon array
        int faceCount = NewtonMeshGetTotalFaceCount (mesh); 
        int indexCount = NewtonMeshGetTotalIndexCount (mesh); 
        int pointCount = NewtonMeshGetPointCount (mesh);
        int vertexStride = NewtonMeshGetVertexStrideInByte(mesh) / sizeof (dFloat);

        int* faceArray = new int [faceCount];
        void** indexArray = new void* [indexCount];
        int* materialIndexArray = new int [faceCount];
        int* remapedIndexArray = new int [indexCount];
        const int *vertexIndexList = NewtonMeshGetIndexToVertexMap(mesh);

        NewtonMeshGetFaces (mesh, faceArray, materialIndexArray, indexArray); 
        NewtonMeshCalculateVertexNormals( mesh, 1.05f );

        for (int i = 0; i < indexCount; i ++) {
    //		void* face = indexArray[i];
            int index = NewtonMeshGetVertexIndex (mesh, indexArray[i]);
            remapedIndexArray[i] = index;
        }
        
        AddTableIndices(L, indexCount, remapedIndexArray);
        
        int vcount = NewtonMeshGetVertexCount(mesh);
        AddTableVertices(L, vcount * 4, NewtonMeshGetVertexArray(mesh));

        double *uvs = NULL;
        if (NewtonMeshHasUV0Channel(mesh)) {
            NewtonMeshGetUV0Channel(mesh, 2 * sizeof (dFloat), (dFloat*)uvs);
            AddTableUVs( L, pointCount*2, uvs );
        } else {
            printf("No UVS in mesh!!\n");
            lua_newtable(L);
        }
    
        double* normals = NULL;
        if (NewtonMeshHasNormalChannel(mesh)) {
            NewtonMeshGetNormalChannel(mesh, 3 * sizeof (dFloat), (dFloat*)normals);
            AddTableNormals( L, vcount * 3, normals );
        } else {
            printf("No Normals in mesh!!\n");
            lua_newtable(L);
        }

        return 5;
    }
    else {
        lua_pushnil(L);
        return 1;
    }
}

static int Update( lua_State *L )
{
    double timestep = luaL_checknumber(L, 1);
    NewtonUpdate(world, (float)timestep);

    lua_newtable(L);
    
    for(size_t i = 0; i<bodies.size(); i++)
    {        
        NewtonBody *body = bodies[i];

        // After update, build the table and set all the pos and quats.
        dFloat rot[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        NewtonBodyGetRotation(body, rot);

        dFloat pos[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        NewtonBodyGetPosition(body, pos);

        lua_pushnumber(L, i+1); 
        lua_newtable(L);
        
        SetTableVector(L, pos, "pos");
        SetTableVector(L, rot, "rot");
        lua_settable(L, -3);
    }

    return 1;
}

static int Close( lua_State *L )
{
    // Clean up.
    for(size_t i=0; i<colls.size(); i++)
        NewtonDestroyCollision(colls[i]);
    colls.clear();
    NewtonDestroy(world);
    return 0;
}


// Functions exposed to Lua
static const luaL_reg Module_methods[] =
{
    {"create", Create}, 
    {"update", Update}, 
    {"close", Close},
    
    {"addcollisionplane", addCollisionPlane },
    {"addcollisioncube", addCollisionCube },
    {"addcollisionsphere", addCollisionSphere },
    {"addcollisioncone", addCollisionCone },
    {"addcollisioncapsule", addCollisionCapsule },
    {"addcollisioncylinder", addCollisionCylinder },
    {"addcollisionchamfercylinder", addCollisionChamferCylinder },
    {"addcollisionconvexhull", addCollisionConvexHull },

    {"addbody", addBody },

    {"createmeshfromcollision", createMeshFromCollision },
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

dmExtension::Result AppInitializeNewtonExtension(dmExtension::AppParams* params)
{
    dmLogInfo("AppInitializeNewtonExtension\n");
    return dmExtension::RESULT_OK;
}

dmExtension::Result InitializeNewtonExtension(dmExtension::Params* params)
{
    // Init Lua
    LuaInit(params->m_L);
    dmLogInfo("Registered %s Extension\n", MODULE_NAME);
    return dmExtension::RESULT_OK;
}

dmExtension::Result AppFinalizeNewtonExtension(dmExtension::AppParams* params)
{
    dmLogInfo("AppFinalizeNewtonExtension\n");
    return dmExtension::RESULT_OK;
}

dmExtension::Result FinalizeNewtonExtension(dmExtension::Params* params)
{
    dmLogInfo("FinalizeNewtonExtension\n");
    for(size_t i=0; i<meshes.size(); i++)
        NewtonMeshDestroy(meshes[i]);
    meshes.clear();
    for(size_t i=0; i<colls.size(); i++)
        NewtonDestroyCollision(colls[i]);
    colls.clear();
    NewtonDestroy(world);    
    return dmExtension::RESULT_OK;
}

dmExtension::Result OnUpdateNewtonExtension(dmExtension::Params* params)
{
    // dmLogInfo("OnUpdateNewtonExtension\n");
    return dmExtension::RESULT_OK;
}

void OnEventNewtonExtension(dmExtension::Params* params, const dmExtension::Event* event)
{
    switch(event->m_Event)
    {
        case dmExtension::EVENT_ID_ACTIVATEAPP:
            dmLogInfo("OnEventNewtonExtension - EVENT_ID_ACTIVATEAPP\n");
            break;
        case dmExtension::EVENT_ID_DEACTIVATEAPP:
            dmLogInfo("OnEventNewtonExtension - EVENT_ID_DEACTIVATEAPP\n");
            break;
        case dmExtension::EVENT_ID_ICONIFYAPP:
            dmLogInfo("OnEventNewtonExtension - EVENT_ID_ICONIFYAPP\n");
            break;
        case dmExtension::EVENT_ID_DEICONIFYAPP:
            dmLogInfo("OnEventNewtonExtension - EVENT_ID_DEICONIFYAPP\n");
            break;
        default:
            dmLogWarning("OnEventNewtonExtension - Unknown event id\n");
            break;
    }
}

// Defold SDK uses a macro for setting up extension entry points:
//
// DM_DECLARE_EXTENSION(symbol, name, app_init, app_final, init, update, on_event, final)

// NewtonExtension is the C++ symbol that holds all relevant extension data.
// It must match the name field in the `ext.manifest`
DM_DECLARE_EXTENSION(NewtonExtension, LIB_NAME, AppInitializeNewtonExtension, AppFinalizeNewtonExtension, InitializeNewtonExtension, OnUpdateNewtonExtension, OnEventNewtonExtension, FinalizeNewtonExtension)
