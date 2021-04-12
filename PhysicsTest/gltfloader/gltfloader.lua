------------------------------------------------------------------------------------------------------------

local tinsert = table.insert

------------------------------------------------------------------------------------------------------------

local geom = require("gltfloader.geometry-utils")
local imageutils = require("gltfloader.image-utils")

------------------------------------------------------------------------------------------------------------

local gltfloader = {
	curr_factory 	= nil,
	temp_meshes 	= {},
}

------------------------------------------------------------------------------------------------------------

local function loadgltf( fname )

	--print(fname)
	-- Check for gltf - only support this at the moment. 
	local valid = string.match(fname, ".*%.gltf$")
	assert(valid)

	local basepath = fname:match("(.*/)")

	-- Note: This can be replaced with io.open if needed.
	local gltfdata, error = sys.load_resource(fname)	
	-- local gltfobj = jsonloader.parse( gltfdata )
	local gltfobj = json.decode( gltfdata )
	gltfobj.basepath = basepath
	--pprint(gltfobj)

	-- load buffers 
	for k,v in pairs(gltfobj.buffers) do 

		if(v.uri) then 
			-- local fh = io.open(basepath..v.uri, "rb")
			local data, error = sys.load_resource(basepath..v.uri)	
			if(data) then 
				v.data = data
			else 
				print("Error: Cannot load gltf binary file ["..basepath..v.uri.."]") 
			end
		end 
	end 
	
	return gltfobj
end 

------------------------------------------------------------------------------------------------------------

function gltfloader:setpool( meshpoolpath, nummeshes )

	-- Make a list of valid meshes
	for i=1, nummeshes do 
		local mpath = meshpoolpath..string.format("%03d", i)..".mesh"
		tinsert(gltfloader.temp_meshes, { path=mpath, used=false, go=nil } )
	end
end 

------------------------------------------------------------------------------------------------------------
-- Get a free temp mesh, nil if none available
function gltfloader:getfreetemp()
	for k,v in pairs(gltfloader.temp_meshes) do 
		if(v.used == false) then 
			v.used = true 
			return v 
		end 
	end 
	return nil
end

------------------------------------------------------------------------------------------------------------

function gltfloader:makeNodeMeshes( gltfobj, goname, parent, n )

	-- Each node can have a mesh reference. If so, get the mesh data and make one, set its parent to the
	--  parent node mesh
	local thisnode = gltfobj.nodes[n] 
	if(thisnode.mesh) then 

		-- Temp.. 
		local gltf = {}
		
		-- Get indices from accessor 
		local thismesh = gltfobj.meshes[thisnode.mesh + 1]
		local prims = thismesh.primitives	
		
		if(prims == nil) then print("No Primitives?"); return end 
			
		local prim = prims[1]

		-- If it has a material, load it, and set the material 
		if(prim.material) then 

			local materialid = prim.material
			local mat = gltfobj.materials[ materialid + 1 ]
			--pprint(mat)
			local pbrmetallicrough = mat.pbrMetallicRoughness 
			if (pbrmetallicrough) then 
				if(pbrmetallicrough.baseColorTexture) then 
					local bcolor = pbrmetallicrough.baseColorTexture.index
					local res = gltfobj.images[bcolor + 1]
					gltfloader:loadimages( gltfobj, goname, bcolor + 1, 0 )
				end 

				if(pbrmetallicrough.metallicRoughnessTexture) then 
					local bcolor = pbrmetallicrough.metallicRoughnessTexture.index
					local res = gltfobj.images[bcolor + 1]
					gltfloader:loadimages( gltfobj, goname, bcolor + 1, 1 )
				end
			end
			local pbremissive = mat.emissiveTexture
			if(pbremissive) then 
				local bcolor = pbremissive.index
				local res = gltfobj.images[bcolor + 1]
				gltfloader:loadimages( gltfobj, goname, bcolor + 1, 2 )
			end
			local pbrnormal = mat.normalTexture
			if(pbrnormal) then  
				local bcolor = pbrnormal.index
				local res = gltfobj.images[bcolor + 1]
				gltfloader:loadimages( gltfobj, goname, bcolor + 1, 3 )
			end
		end 
		
		local acc_idx = prim.indices
		local accessor = gltfobj.accessors[acc_idx + 1]

		local bv = gltfobj.bufferViews[accessor.bufferView + 1]
		local byteoff = accessor.byteOffset -- Not sure what to do with this just yet 
		local buffer = gltfobj.buffers[bv.buffer+1]

		gltf.indices = {}
		-- Indices specific - this is default dataset for gltf (I think)
		geomextension.setbufferintsfromtable(bv.byteOffset, bv.byteLength, buffer.data, gltf.indices)

		-- Get position accessor
		local aidx = gltfobj.accessors[prim.attributes["POSITION"] + 1]
		bv = gltfobj.bufferViews[aidx.bufferView + 1]
		buffer = gltfobj.buffers[bv.buffer + 1]
		
		-- Get positions (or verts) 
		gltf.verts = {}
		-- getBufferData( gltf.verts, bv, buffer )
		geomextension.setbufferfloatsfromtable(bv.byteOffset or 0, bv.byteLength, buffer.data, gltf.verts)
		
		-- Get uvs accessor
		aidx = gltfobj.accessors[prim.attributes["TEXCOORD_0"] + 1]
		bv = gltfobj.bufferViews[aidx.bufferView + 1]
		buffer = gltfobj.buffers[bv.buffer + 1]

		-- Get positions (or verts) 
		gltf.uvs = {}
		-- getBufferData( gltf.uvs, bv, buffer )
		geomextension.setbufferfloatsfromtable(bv.byteOffset or 0, bv.byteLength or 0, buffer.data, gltf.uvs)
				
		-- Get normals accessor
		aidx = gltfobj.accessors[prim.attributes["NORMAL"] + 1]
		bv = gltfobj.bufferViews[aidx.bufferView + 1]
		buffer = gltfobj.buffers[bv.buffer + 1]

		-- Get positions (or verts) 
		gltf.normals = {}
		-- getBufferData( gltf.normals, bv, buffer )
		geomextension.setbufferfloatsfromtable(bv.byteOffset, bv.byteLength, buffer.data, gltf.normals)
		
		-- 	local indices	= { 0, 1, 2, 0, 2, 3 }
		-- 	local verts		= { -sx + offx, 0.0, sy + offy, sx + offx, 0.0, sy + offy, sx + offx, 0.0, -sy + offy, -sx + offx, 0.0, -sy + offy }
		-- 	local uvs		= { 0.0, 0.0, uvMult, 0.0, uvMult, uvMult, 0.0, uvMult }
		-- 	local normals	= { 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0 }
		geom:makeMesh( goname, gltf.indices, gltf.verts, gltf.uvs, gltf.normals )
		--print("Geometry: ", goname)

	-- No mesh.. try children
	else

		if(thisnode.children) then 
			for k,v in pairs(thisnode.children) do 
				local child = gltfobj.nodes[v + 1]
				if(child.mesh) then 
					self:makeNodeMeshes( gltfobj, goname, pobj, v + 1)
				end
			end 
		end
	end
	
end	

------------------------------------------------------------------------------------------------------------
-- Load images: This is horribly slow at the moment. Will improve.

function gltfloader:loadimages( gltfobj, goname, idx, tid )

	tid = tid or 0
	-- Load in any images 
	--for k,v in pairs(gltfobj.images) do 
	local v = gltfobj.images[idx]
	imageutils.loadimage(goname, gltfobj.basepath..v.uri, tid )
	--end
end


------------------------------------------------------------------------------------------------------------
-- goname is needed as the parent or as the single mesh (if its known)
-- fname is the filename for the gltf model
-- ofactory is the url for the factory for creating meshes. 
--     At the moment, it needs quite a specific setup. This will change.

-- NOTE: All gltf files can only load a single mesh at the moment. Looking into how to generate a mesh 
--       hierarchy. Might be a complicated problem. Considering single buffered meshes with redundant verts.

function gltfloader:load( fname, pobj, meshname )

	-- Parent mesh
	local goname = msg.url(nil, pobj, meshname)	
	local goscript = pobj.."#script"
	
	-- local gltfobj = tinygltf_extension.loadmodel( fname )
	local gltfobj = loadgltf( fname )
		
	local gltfmesh 	= geom:New(goname)
	geom:New(goname, 1.0)
	tinsert(geom.meshes, goname)

	--gltfloader:loadimages( gltfobj, pobj, goname )	
	
	-- Go throught the scenes (we will only really care about the first one initially)
	for k,v in pairs(gltfobj.scenes) do
		if k > 1 then break end

		-- Go through the scenes nodes - this is recursive. nodes->children->children..
		for ni, n in pairs(v.nodes) do
			self:makeNodeMeshes( gltfobj, goname, pobj, n + 1)
		end 
	end
end


------------------------------------------------------------------------------------------------------------

return gltfloader

------------------------------------------------------------------------------------------------------------
