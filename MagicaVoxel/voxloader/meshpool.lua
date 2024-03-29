
local tinsert = table.insert

-- --------------------------------------------------------------------------------------------------------

local fpgen = require("voxloader.filepool-gen")

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
}

-- --------------------------------------------------------------------------------------------------------
-- A reverse mnapping from goname->meshpoolfiles (might be handy later)
meshpool.mapped = {
}

-- --------------------------------------------------------------------------------------------------------

function init( count, regenerate, collfile, rootid )

	-- THis is here because you shouldnt need to change it often - can move this out if needed.
	fpgen.init( "assets/gotemplate/meshpool/temp", "assets/images/", "assets/shaders/" )

	-- Need to find a nice way to do this.
	--os.execute( "rm -rf assets/gotemplate/meshpool/*" )
	
	-- The make pool files is not needed for every run. But it keeps all the objects "clean"
	-- Should add a return check for success
	if(regenerate) then 
		
		print(collfile, rootid, count)
		local newfiles = fpgen.addtocollection(collfile, rootid, count) 
		-- Check if the rootgo has children (if not, add them, and stop execution)
		if(newfiles) then 
			print("---------------------------------------------------------------------")
			print("    Built new go files, updating main collection. Restart build.")
			print("---------------------------------------------------------------------")
			os.exit(1)
		end 
	end
	meshpool.maxindex = count
	meshpool.currentindex = 1
end

-- --------------------------------------------------------------------------------------------------------

function setroot( rootgo, maincollection )

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
function gettemp( name, filepath )

	if(meshpool.currentindex > meshpool.maxindex) then print("No More Meshes!"); return nil end
	-- TBD: This needs to be more flexible and easier to change
	local goname = "/temp"..string.format("%03d", meshpool.currentindex)
	meshpool.currentindex = meshpool.currentindex + 1

	meshpool.files[name] = { name = name, goname = goname, fpath = filepath or "", priority = 0 } 
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
