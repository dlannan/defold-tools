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

// Define a custom data structure to store a body ID.
struct UserData {
    int bodyID=0;
};


void cb_applyForce(const NewtonBody* const body, dFloat timestep, int threadIndex)
{
    // Fetch user data and body position.
    UserData *mydata = (UserData*)NewtonBodyGetUserData(body);
    dFloat pos[4];
    NewtonBodyGetPosition(body, pos);

    // Apply force.
    dFloat force[3] = {0, -1.0, 0};
    NewtonBodySetForce(body, force);

    // Print info to terminal.
    printf("BodyID=%d, Sleep=%d, %.2f, %.2f, %.2f\n",
    mydata->bodyID, NewtonBodyGetSleepState(body), pos[0], pos[1], pos[2]);
}


static int addCollisionSphere( lua_State * L ) {

    double radii = lua_tonumber(L, 1);
    // Collision shapes: sphere (our ball), and large box (our ground plane).
    NewtonCollision* cs_sphere = NewtonCreateSphere(world, radii, 0, NULL);
    colls.push_back( cs_sphere );
    lua_pushnumber(L, colls.size()-1);
    return 1;
}

static int addCollisionPlane( lua_State * L ) {

    double width = lua_tonumber(L, 1);
    double depth = lua_tonumber(L, 2);
    // Collision shapes: sphere (our ball), and large box (our ground plane).
    NewtonCollision* cs_ground = NewtonCreateBox(world, width, 0.1, depth, 0, NULL);
    colls.push_back( cs_ground );
    lua_pushnumber(L, colls.size() - 1);
    return 1;
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

    tm[12] = x; tm[13] = y; tm[14] = z;
    NewtonBody *body = NewtonCreateDynamicBody(world, colls[idx], tm);
    bodies.push_back(body);
    NewtonBodySetForceAndTorqueCallback(body, cb_applyForce);

    UserData *myData = new UserData[2];
    myData[0].bodyID = bodies.size()-1;
    NewtonBodySetUserData(body, (void *)&myData[0]);
    
    lua_pushnumber(L, bodies.size() - 1);
    return 1;
}

static int addBodies(lua_State *L) {
    // Neutral transform matrix.
    float	tm[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    // Collision shapes: sphere (our ball), and large box (our ground plane).
    NewtonCollision* cs_sphere = NewtonCreateSphere(world, 1, 0, NULL);
    NewtonCollision* cs_ground = NewtonCreateBox(world, 100, 0.1, 100, 0, NULL);
    colls.push_back(cs_sphere);
    colls.push_back(cs_ground);

    // Create the bodies and assign them the collision shapes. Note that
    // we need to modify initial transform for the ball to place it a y=2.0
    NewtonBody* ground = NewtonCreateDynamicBody(world, cs_ground, tm);
    tm[13] = 2.0;
    NewtonBody* sphere = NewtonCreateDynamicBody(world, cs_sphere, tm);

    bodies.push_back(ground);
    bodies.push_back(sphere);
    
    // Assign non-zero mass to sphere to make it dynamic.
    NewtonBodySetMassMatrix(sphere, 1.0f, 1, 1, 1);

    // Install the callbacks to track the body positions.
    NewtonBodySetForceAndTorqueCallback(sphere, cb_applyForce);
    NewtonBodySetForceAndTorqueCallback(ground, cb_applyForce);

    // Attach our custom data structure to the bodies.
    UserData *myData = new UserData[2];
    myData[0].bodyID = 0;
    myData[1].bodyID = 1;
    NewtonBodySetUserData(sphere, (void *)&myData[0]);
    NewtonBodySetUserData(ground, (void *)&myData[1]);
    return 0;
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

static int Update( lua_State *L )
{
    double timestep = luaL_checknumber(L, 1);
    NewtonUpdate(world, (float)timestep);
    return 0;
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
    {"addbodies", addBodies},
    {"addcollisionplane", addCollisionPlane },
    {"addcollisionsphere", addCollisionSphere },
    {"addbody", addBody },
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
