------------------------------------------------------------------------------------------------------------

local tinsert = table.insert
local ffi 	= package.preload.ffi()

ffi.cdef[[

	union floatData {
		struct data {
			unsigned char a;
			unsigned char b;
			unsigned char c;
			unsigned char d;
		} data;
		float f;
	};
]]

------------------------------------------------------------------------------------------------------------

local geom = require("assets.gotemplate.scripted_geom")

------------------------------------------------------------------------------------------------------------

local gltfloader = {
	curr_factory 	= nil,
}

------------------------------------------------------------------------------------------------------------
local tfloat = ffi.new("union floatData")

local function getBufferData( data, bv, buffer )
	bc = 1
	while( bc < bv.byteLength ) do 
		local bcl = bc + bv.byteOffset
		tfloat.data.a = buffer.data[bcl]
		tfloat.data.b = buffer.data[bcl+1]
		tfloat.data.c = buffer.data[bcl+2]
		tfloat.data.d = buffer.data[bcl+3]
		tinsert( data, tonumber(tfloat.f) )
		bc = bc + 4  -- This needs to be checked for type float etc
	end
end

------------------------------------------------------------------------------------------------------------

function gltfloader:makeNodeMeshes( gltfobj, goname, parent, nidx, n )

	-- Each node can have a mesh reference. If so, get the mesh data and make one, set its parent to the
	--  parent node mesh
	local thisnode = gltfobj.nodes[n] 
	if(thisnode.mesh) then 

		-- Temp.. 
		local gltf = {}
		
		-- Get indices from accessor 
		local prims = gltfobj.meshes[thisnode.mesh].primitives
		if(prims == nil) then print("No Primitives?"); return end 
		local prim = prims[1]
		local acc_idx = prim.indices
		local accessor = gltfobj.accessors[acc_idx]

		local bv = gltfobj.bufferviews[accessor.bufferView]
		local byteoff = accessor.byteOffset -- Not sure what to do with this just yet 

		local buffer = gltfobj.buffers[bv.buffer]

		-- Indices specific - this is default dataset for gltf (I think)
		gltf.indices = {}
		local bc = 1
		while( bc < bv.byteLength ) do 
			local bcl = bc + bv.byteOffset
			local index = bit.bor(bit.lshift(buffer.data[bcl+1], 8), buffer.data[bcl])
			tinsert( gltf.indices, index )
			bc = bc + 2
		end
				
		-- Get position accessor
		bv = gltfobj.bufferviews[prim.attribs["POSITION"]]
		buffer = gltfobj.buffers[bv.buffer]
		
		-- Get positions (or verts) 
		gltf.verts = {}
		getBufferData( gltf.verts, bv, buffer )

		-- Get uvs accessor
		bv = gltfobj.bufferviews[prim.attribs["TEXCOORD_0"]]
		buffer = gltfobj.buffers[bv.buffer]

		-- Get positions (or verts) 
		gltf.uvs = {}
		getBufferData( gltf.uvs, bv, buffer )
		
		-- Get normals accessor
		bv = gltfobj.bufferviews[prim.attribs["NORMAL"]]
		buffer = gltfobj.buffers[bv.buffer]

		-- Get positions (or verts) 
		gltf.normals = {}
		getBufferData( gltf.normals, bv, buffer )
		
		-- 	local indices	= { 0, 1, 2, 0, 2, 3 }
		-- 	local verts		= { -sx + offx, 0.0, sy + offy, sx + offx, 0.0, sy + offy, sx + offx, 0.0, -sy + offy, -sx + offx, 0.0, -sy + offy }
		-- 	local uvs		= { 0.0, 0.0, uvMult, 0.0, uvMult, uvMult, 0.0, uvMult }
		-- 	local normals	= { 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0 }
		geom:makeMesh( goname, gltf.indices, gltf.verts, gltf.uvs, gltf.normals )
		print("Geometry: ", goname)
	end
	
end	

------------------------------------------------------------------------------------------------------------
-- goname is needed as the parent or as the single mesh (if its known)
-- fname is the filename for the gltf model
-- ofactory is the url for the factory for creating meshes. 
--     At the moment, it needs quite a specific setup. This will change.

function gltfloader:load( fname, ofactory, meshname )

	-- Parent mesh
	local pobj = factory.create(ofactory)
	local goname = msg.url(nil, pobj, meshname)
	
	local gltfobj = tinygltf_extension.loadmodel( fname )
	-- pprint(gltfobj)
		
	local gltfmesh 	= geom:New(goname)
	geom:New(goname, 1.0)
	tinsert(geom.meshes, goname)

	-- Go throught the scenes (we will only really care about the first one initially)
	for k,v in pairs(gltfobj.scenes) do
		if k > 1 then break end

		-- Go through the scenes nodes - this is recursive. nodes->children->children..
		for ni, n in pairs(v.nodes) do
			self:makeNodeMeshes( gltfobj, goname, pobj, ni, n)
		end 
	end
	return pobj
end


------------------------------------------------------------------------------------------------------------

return gltfloader

------------------------------------------------------------------------------------------------------------
