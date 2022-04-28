------------------------------------------------------------------------------------------------------------

local tinsert = table.insert

------------------------------------------------------------------------------------------------------------

local mpool 		= require("voxloader.meshpool")

local geom 			= require("voxloader.geometry-utils")
local imageutils 	= require("voxloader.image-utils")

local blob 			= require("utils.Blob")

-- Create a custom type that can parse 2D or 3D vectors
blob.types.vector = function(dimensions)
    -- The vector has one double value per dimension
    return string.rep("d", dimensions)
end

------------------------------------------------------------------------------------------------------------

local voxloader = {
	curr_factory 	= nil,
	temp_meshes 	= {},
}

------------------------------------------------------------------------------------------------------------

local function readString( vdata )

	local strsz = vdata:unpack("i4")
	local fmt = "c"..tostring(strsz)
	local str = vdata:unpack(fmt)
	return str
end 

------------------------------------------------------------------------------------------------------------

local function readDict( vdata )

	local cnt = vdata:unpack("i4")
	local dict = {}
	for i = 1, cnt do 
		local key = readString( vdata ) 
		local value = readString( vdata )
		tinsert( dict, { key = key, value = value })
	end 
	return dict
end 

------------------------------------------------------------------------------------------------------------

local function doChunkPack( vdata )
	local nummodels = vdata:unpack("i4")
	print("[Number of Models] ", nummodels)
end

------------------------------------------------------------------------------------------------------------

local function doChunkMain( vdata )
end
------------------------------------------------------------------------------------------------------------

local function doChunkSize( vdata )
	local x = vdata:unpack("i4")
	local y = vdata:unpack("i4")
	local z = vdata:unpack("i4")  -- gravity direction
	print("[ SIZE ] ",x,y,z)
	return {x,y,z}
end
------------------------------------------------------------------------------------------------------------

local function doChunkXYZI( vdata )
	local numvoxels = vdata:unpack("i4")
	print("[ VOXELS ] ", numvoxels)
	local voxels = {}
	for i=1, numvoxels do
		local x,y,z,i = vdata:unpack("BBBB")
		tinsert(voxels, {x,y,z,i})
		--pprint("[ XYZI ] ", z,y,z,i)
	end 
	return voxels
end
------------------------------------------------------------------------------------------------------------

local function doChunkRGBA( vdata )
	local colorpalette = {}
	tinsert(colorpalette, {0,0,0,0})
	for i=1, 255 do 
		local r,g,b,a = vdata:unpack("BBBB")
		tinsert(colorpalette, {r,g,b,a})
	end 
	-- pprint("[ COLOR PAL ] ",colorpalette)
	return colorpalette 
end

------------------------------------------------------------------------------------------------------------

local function doChunknTRN( vdata )
	local nodeid = vdata:unpack("i4")
	local attribs = readDict( vdata )
	local childid = vdata:unpack("i4")
	local reserved = vdata:unpack("i4")
	local layerid = vdata:unpack("i4")
	local numframes = vdata:unpack("i4")
	print("[ nTRN Num Frames ] ", numframes)
	local frames = {}
	for i = 1, numframes do
		local dict = readDict(vdata)
		tinsert(frames, { dict = dict })
	end 
	return frames
end

------------------------------------------------------------------------------------------------------------

local function doChunknGRP( vdata )
	local nodeid = vdata:unpack("i4")
	local attribs = readDict( vdata )
	local childnodes = vdata:unpack("i4")
	local children = {}
	for i= 1, childnodes do 
		tinsert(children, { id = vdata:unpack("i4") } )
	end 
	return children
end

------------------------------------------------------------------------------------------------------------

local function doChunknSHP( vdata )
	local nodeid = vdata:unpack("i4")
	local attribs = readDict( vdata )
	local nummodels = vdata:unpack("i4")

	local models = {}
	for i = 1, nummodels do 
		local modelid = vdata:unpack("i4")
		local att = readDict(vdata)
		tinsert( models,  { id = modelid, attr = att } )
	end
	return models
end

------------------------------------------------------------------------------------------------------------

local function doChunkLAYR( vdata )
	local nodeid = vdata:unpack("i4")
	local attribs = readDict( vdata )
	local reserved = vdata:unpack("i4")
	return { id = nodeid, attribs = attribs }
end

------------------------------------------------------------------------------------------------------------

local function processChunks(vdata, dlen, voxobj)

	-- voxobj is a close relation to gltfobj.
	-- The main difference is that we generate a quad buffer of cubes for each 
	-- voxel within the mesh buffer.
	voxobj.sizes 	= {}
	voxobj.xyzi 	= {}
	voxobj.palette 	= nil 
	voxobj.groups 	= {}
	voxobj.transforms = {}
	voxobj.shapes 	= {} 
	voxobj.layers 	= {} 


	-- Keep reading until we have it all.
	while( dlen - vdata.pos > 4  ) do 

		-- Read chunk first:
		local chunkid = vdata:bytes(4)
		print(chunkid)
		local bcontent = vdata:unpack("i4")
		local bchildren = vdata:unpack("i4")
		print(bcontent, bchildren)
	
		if(chunkid == "PACK") then 
			doChunkPack(vdata)
		elseif(chunkid == "MAIN") then 
			doChunkMain(vdata)
		elseif(chunkid == "SIZE") then 
			tinsert( voxobj.sizes, doChunkSize(vdata) )
		elseif(chunkid == "XYZI") then 
			tinsert( voxobj.xyzi, doChunkXYZI(vdata) )
		elseif(chunkid == "RGBA") then 
			voxobj.palette = doChunkRGBA(vdata)
		elseif(chunkid == "nTRN") then 
			tinsert( voxobj.transforms, doChunknTRN(vdata) )
		elseif(chunkid == "nGRP") then 
			tinsert( voxobj.groups, doChunknGRP(vdata) )
		elseif(chunkid == "nSHP") then 
			tinsert( voxobj.shapes, doChunknSHP(vdata) )
		elseif(chunkid == "MATL") then 
			vdata.bytes(bcontent)   -- skip this chunk
		elseif(chunkid == "LAYR") then 
			tinsert( voxobj.layers, doChunkLAYR(vdata) )
		elseif(chunkid == "rOBJ") then 
			vdata.bytes(bcontent)   -- skip this chunk
		elseif(chunkid == "rCAM") then 
			vdata.bytes(bcontent)   -- skip this chunk
		elseif(chunkid == "NOTE") then 
			vdata.bytes(bcontent)   -- skip this chunk
		else 
			print("[ EXITED ] ")
			break
		end
		print("[ DLEN ] ", vdata.pos, dlen)
	end
end

------------------------------------------------------------------------------------------------------------

local function parsevox( voxdata )

	-- Iterate through the file. Stepping and converting bytes.
	-- print(blob.backend)
	local vdata = blob.new(voxdata)
	assert(vdata:bytes(4) == "VOX ")
	local ver = vdata:unpack("i4")
	local dlen = #voxdata 
	pprint(dlen, ver)

	local voxobj = {}
	processChunks(vdata, dlen, voxobj)

	return voxobj
end

------------------------------------------------------------------------------------------------------------

local function loadvox( fname )

	--print(fname)
	-- Check for gltf - only support this at the moment. 
	print(fname)
	local valid = string.match(fname, ".*%.vox$")
	assert(valid)

	local basepath = fname:match("(.*/)")

	-- Note: This can be replaced with io.open if needed.
	local voxdata, error = sys.load_resource(fname)	
	local voxobj = parsevox( voxdata )
	voxobj.basepath = basepath
	-- pprint(voxobj)

	return voxobj
end 

------------------------------------------------------------------------------------------------------------

function voxloader:setpool( meshpoolpath, nummeshes )

	-- Make a list of valid meshes
	for i=1, nummeshes do 
		local mpath = meshpoolpath..string.format("%03d", i)..".mesh"
		tinsert(voxloader.temp_meshes, { path=mpath, used=false, go=nil } )
	end
end 

------------------------------------------------------------------------------------------------------------
-- Get a free temp mesh, nil if none available
function voxloader:getfreetemp()
	for k,v in pairs(voxloader.temp_meshes) do 
		if(v.used == false) then 
			v.used = true 
			return v 
		end 
	end 
	return nil
end

------------------------------------------------------------------------------------------------------------

function voxloader:makeNodeMeshes( voxobj, goname, parent, n )

	-- Each node can have a mesh reference. If so, get the mesh data and make one, set its parent to the
	--  parent node mesh
	local gomeshname  = parent.."/node"..string.format("%04d", n)
	local gochild = mpool.gettemp( gomeshname )
	local gochildname = gochild.."#temp"
	
	local thisnode = voxobj.nodes[n] 
	-- print("Name:", thisnode.name, parent)	
	if(thisnode.mesh) then 
		
		-- Temp.. 
		local gltf = {}
		
		-- Get indices from accessor 
		local thismesh = voxobj.meshes[thisnode.mesh + 1]
		local prims = thismesh.primitives	
		
		if(prims == nil) then print("No Primitives?"); return end 
			
		local prim = prims[1]

		-- If it has a material, load it, and set the material 
		if(prim.material) then 

			local materialid = prim.material
			local mat = voxobj.materials[ materialid + 1 ]
			--pprint(mat)
			local pbrmetallicrough = mat.pbrMetallicRoughness 
			if (pbrmetallicrough) then 
				if(pbrmetallicrough.baseColorTexture) then 
					local bcolor = pbrmetallicrough.baseColorTexture.index
					local res = voxobj.images[bcolor + 1]
					voxloader:loadimages( voxobj, gochildname, bcolor + 1, 0 )
				end 

				if(pbrmetallicrough.metallicRoughnessTexture) then 
					local bcolor = pbrmetallicrough.metallicRoughnessTexture.index
					local res = voxobj.images[bcolor + 1]
					voxloader:loadimages( voxobj, gochildname, bcolor + 1, 1 )
				end
			end
			local pbremissive = mat.emissiveTexture
			if(pbremissive) then 
				local bcolor = pbremissive.index
				local res = voxobj.images[bcolor + 1]
				voxloader:loadimages( voxobj, gochildname, bcolor + 1, 2 )
			end
			local pbrnormal = mat.normalTexture
			if(pbrnormal) then  
				local bcolor = pbrnormal.index
				local res = voxobj.images[bcolor + 1]
				voxloader:loadimages( voxobj, gochildname, bcolor + 1, 3 )
			end
		end 
		
		local acc_idx = prim.indices
		local accessor = voxobj.accessors[acc_idx + 1]

		local bv = voxobj.bufferViews[accessor.bufferView + 1]
		local byteoff = accessor.byteOffset -- Not sure what to do with this just yet 
		local buffer = voxobj.buffers[bv.buffer+1]

		gltf.indices = {}
		-- Indices specific - this is default dataset for gltf (I think)
		geomextension.setbufferintsfromtable(bv.byteOffset, bv.byteLength, buffer.data, gltf.indices)

		-- Get position accessor
		gltf.verts = {}
		local posidx = prim.attributes["POSITION"]
		if(posidx) then 
			local aidx = voxobj.accessors[posidx + 1]
			bv = voxobj.bufferViews[aidx.bufferView + 1]
			buffer = voxobj.buffers[bv.buffer + 1]
			-- Get positions (or verts) 
			geomextension.setbufferfloatsfromtable(bv.byteOffset or 0, bv.byteLength or 0, buffer.data, gltf.verts)
		end
		
		-- Get uvs accessor
		gltf.uvs = {}
		local texidx = prim.attributes["TEXCOORD_0"]
		if(texidx) then 
			local aidx = voxobj.accessors[texidx + 1]
			bv = voxobj.bufferViews[aidx.bufferView + 1]
			buffer = voxobj.buffers[bv.buffer + 1]
			geomextension.setbufferfloatsfromtable(bv.byteOffset or 0, bv.byteLength or 0, buffer.data, gltf.uvs)
		end 

		-- Get normals accessor
		gltf.normals = {}
		local normidx = prim.attributes["NORMAL"]
		if(normidx) then 
			local aidx = voxobj.accessors[normidx + 1]
			bv = voxobj.bufferViews[aidx.bufferView + 1]
			buffer = voxobj.buffers[bv.buffer + 1]
			geomextension.setbufferfloatsfromtable(bv.byteOffset or 0, bv.byteLength or 0, buffer.data, gltf.normals)
		end 
		
		-- 	local indices	= { 0, 1, 2, 0, 2, 3 }
		-- 	local verts		= { -sx + offx, 0.0, sy + offy, sx + offx, 0.0, sy + offy, sx + offx, 0.0, -sy + offy, -sx + offx, 0.0, -sy + offy }
		-- 	local uvs		= { 0.0, 0.0, uvMult, 0.0, uvMult, uvMult, 0.0, uvMult }
		-- 	local normals	= { 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0 }
		geom:makeMesh( gochildname, gltf.indices, gltf.verts, gltf.uvs, gltf.normals )

	-- No mesh.. try children
	end 

	local trans = thisnode["translation"]
	if(trans) then go.set_position(vmath.vector3(trans[1], trans[2], trans[3]), gochildname) end
	-- Parent this mesh to the incoming node 
	go.set_parent(go.get_id(gochild), parent)
	
	if(thisnode.children) then 
		for k,v in pairs(thisnode.children) do 
			local child = voxobj.nodes[v + 1]
			self:makeNodeMeshes( voxobj, gochild, gochildname, v + 1)
		end 
	end
end	

------------------------------------------------------------------------------------------------------------
-- Load images: This is horribly slow at the moment. Will improve.

function voxloader:loadimages( voxobj, goname, idx, tid )

	tid = tid or 0
	-- Load in any images 
	--for k,v in pairs(voxobj.images) do 
	local v = voxobj.images[idx]
	imageutils.loadimage(goname, voxobj.basepath..v.uri, tid )
	--end
end


------------------------------------------------------------------------------------------------------------
-- goname is needed as the parent or as the single mesh (if its known)
-- fname is the filename for the gltf model
-- ofactory is the url for the factory for creating meshes. 
--     At the moment, it needs quite a specific setup. This will change.

-- NOTE: All gltf files can only load a single mesh at the moment. Looking into how to generate a mesh 
--       hierarchy. Might be a complicated problem. Considering single buffered meshes with redundant verts.

function voxloader:load( fname, pobj, meshname )

	-- Note: Meshname is important for idenifying a mesh you want to be able to modify
	if(pobj == nil) then 
		pobj = mpool.gettemp( meshname )
	end 
	
	-- Parent mesh
	local goname = msg.url(nil, pobj, meshname)	
	local goscript = pobj.."#script"
	
	-- local voxobj = tinygltf_extension.loadmodel( fname )
	local voxobj = loadvox( fname )
		
	local gltfmesh 	= geom:New(goname)
	geom:New(goname, 1.0)
	tinsert(geom.meshes, goname)

	--voxloader:loadimages( voxobj, pobj, goname )	
	
	-- Go through the scenes (we will only care about the first one initially)
	for k,v in pairs(voxobj.scenes) do
		if k > 1 then break end

		-- Go through the scenes nodes - this is recursive. nodes->children->children..
		local childtag = nil
		for ni, n in pairs(v.nodes) do
			self:makeNodeMeshes( voxobj, goname, pobj, n + 1)
		end 
	end

	return pobj
end

-- --------------------------------------------------------------------------------------------------------
-- Uses a tempobject within the pool, assigns a goname to the mapping (must be unique). 
-- Initial pos vec3 or initial rotation quat can be provided. Otherwise will be default
function voxloader:addmesh( filepath, name, initpos, initrot )

	local pos = initpos or vmath.vector3(0, 0, 0)
	local rot = initrot or vmath.quat_rotation_y(0.0)
	local goname = mpool.gettemp( name, filepath )

	voxloader:load(filepath, goname, name)
	go.set_rotation(rot, goname)
	go.set_position(pos, goname)

	return goname 
end 

------------------------------------------------------------------------------------------------------------

return voxloader

------------------------------------------------------------------------------------------------------------
