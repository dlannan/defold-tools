
local tinsert = table.insert

-- --------------------------------------------------------------------------------------------------------

local gltf = require("gltfloader.gltfloader")
local fpgen = require("gltfloader.filepool-gen")

-- --------------------------------------------------------------------------------------------------------
-- MeshPool 
-- Concept: Aim is to provide a "Defold like" interface to the meshpool game objects within the pool.
--          The objects all have generic names and settings, but the user shall be able to provide their
--          own names and use the "simpler" interfaces here to manipulate a meshpool go. 
--          Note: This is not ideal, it adds another "layer" of abstraction which is something I personally
--                dont like doing. So there might likey be an native extension in the future to make this all more
--                Defold friendly.

local meshpool = {
	maxindex 		= 0,
	currentindex 	= 0, 
}


-- The files mapped into the meshpool. Each file is mapped to a temp go/mesh. 
-- The meshpool should always be created with "maximum" amount of gameobjects to be used. 
-- A priority system will be added to allow gameobjects to "take over" less used gameobjects.

meshpool.files = {
	-- helmet = { name = "helmet", goname = "/temp/temp001", fpath = "/assets/models/DamagedHelmet/glTF/DamagedHelmet.gltf", priority = 0 },
	-- car = { name = "helmet", goname = "/temp/temp001", fpath = "/assets/models/ALPINIST/ALPINIST_HI_Body.gltf", priority = 0 },

	-- 	"/assets/models/Cube/Cube.gltf",
	-- 	"/assets/models/Suzanne/glTF/Suzanne.gltf",
	-- 	"/assets/models/Lantern/glTF/Lantern.gltf",
}

-- --------------------------------------------------------------------------------------------------------
-- A reverse mnapping from goname->meshpoolfiles (might be handy later)
meshpool.mapped = {
}

-- --------------------------------------------------------------------------------------------------------

function init( count, regenerate )

	-- THis is here because you shouldnt need to change it often - can move this out if needed.
	fpgen.init( "assets/gotemplate/meshpool/temp", "assets/images/", "assets/shaders/" )

	-- Need to find a nice way to do this.
	--os.execute( "rm -rf /home/dlannan/store1/Development/defold/games/treaure-hunt/assets/gotemplate/meshpool/*" )
	
	-- The make pool files is not needed for every run. But it keeps all the objects "clean"
	-- Should add a return check for success
	if(regenerate) then fpgen.makecollection("assets/gotemplate/temp.collection", count) end
	meshpool.maxindex = count
	meshpool.currentindex = #meshpool.files + 1
	
	-- If there are files already set (as above) then add automatically 
	for k, v in pairs(meshpool.files) do 
		addmesh( v.fpath, v.name )
	end
end

-- --------------------------------------------------------------------------------------------------------
-- Uses a tempobject within the pool, assigns a goname to the mapping (must be unique). 
-- Initial pos vec3 or initial rotation quat can be provided. Otherwise will be default
function addmesh( filepath, name, initpos, initrot )

	if(meshpool.currentindex > meshpool.maxindex) then print("No More Meshes!"); return nil end
	-- Is this new or current meshfile
	local ismesh = meshpool.files[name]
	
	local pos = initpos or vmath.vector3(0, 0, 0)
	local rot = initrot or vmath.quat_rotation_y(0.0)
	
	local goname = "/temp/temp"..string.format("%03d", meshpool.currentindex)
	if(ismesh == nil) then meshpool.currentindex = meshpool.currentindex + 1 end 

	gltf:load(filepath, goname, "temp")
	go.set_rotation(rot, goname)
	go.set_position(pos, goname)

	meshpool.files[name] = { name = name, goname = goname, fpath = filepath, priority = 0 } 
	meshpool.mapped[goname] = meshpool.files[name]
	return goname 
end 

-- --------------------------------------------------------------------------------------------------------
-- TODO: Add object manipulation routines and shader controls. Also updaters, message handlers and controllers.

function updateall( updatefunc ) 

	for k, v in pairs(meshpool.files) do  
		updatefunc( v )
	end 
end

-- --------------------------------------------------------------------------------------------------------
-- Allocates a temp go slot for external mesh use. Returns goname
function gettemp( name )

	if(meshpool.currentindex > meshpool.maxindex) then print("No More Meshes!"); return nil end
	local goname = "/temp/temp"..string.format("%03d", meshpool.currentindex)
	meshpool.currentindex = meshpool.currentindex + 1

	meshpool.files[name] = { name = name, goname = goname, fpath = "", priority = 0 } 
	meshpool.mapped[goname] = meshpool.files[name]
	return goname 
end

-- --------------------------------------------------------------------------------------------------------

meshpool.init		= init 
meshpool.addmesh	= addmesh
meshpool.updateall	= updateall
meshpool.gettemp	= gettemp

return meshpool

-- --------------------------------------------------------------------------------------------------------
