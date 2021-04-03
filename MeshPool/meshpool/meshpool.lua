
local tinsert = table.insert

-- --------------------------------------------------------------------------------------------------------
-- Add gltfloader to load meshes into pool temps
-- local gltf = require("meshpool.gltfloader")
local fpgen = require("meshpool.filepool-gen")

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
	-- helmet = { name = "helmet", go = "/temp/temp001", fpath = "/assets/models/DamagedHelmet/glTF/DamagedHelmet.gltf", priority = 0 },
	-- car = { name = "helmet", go = "/temp/temp001", fpath = "/assets/models/ALPINIST/ALPINIST_HI_Body.gltf", priority = 0 },
}

-- --------------------------------------------------------------------------------------------------------
-- A reverse mnapping from goname->meshpoolfiles (might be handy later)
meshpool.mapped = {
}

-- --------------------------------------------------------------------------------------------------------

function init( count, regenerate )

	-- The make pool files is not needed for every run. But it keeps all the objects "clean"
	-- Should add a return check for success
	if(regenerate) then fpgen.makecollection("assets/temp.collection", count) end
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

	-- Add gltfloader to load files!
	-- gltf:load(filepath, goname, "temp")
	go.set_rotation(rot, goname)
	go.set_position(pos, goname)

	meshpool.files[name] = { name = name, goname = goname, fpath = filepath, priority = 0 } 
	meshpool.mapped[goname] = meshpool.files[name]
	return goname 
end 

-- --------------------------------------------------------------------------------------------------------
-- TODO: Add object manipulation routines and shader controls. Also updaters, message handlers and controllers.


-- --------------------------------------------------------------------------------------------------------

meshpool.init		= init 
meshpool.addmesh	= addmesh

return meshpool

-- --------------------------------------------------------------------------------------------------------
