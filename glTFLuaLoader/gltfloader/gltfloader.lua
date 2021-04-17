------------------------------------------------------------------------------------------------------------

local tinsert = table.insert

------------------------------------------------------------------------------------------------------------

local mpool 		= require("gltfloader.meshpool")

local geom 			= require("gltfloader.geometry-utils")
local imageutils 	= require("gltfloader.image-utils")

local b64 			= require("utils.base64")

------------------------------------------------------------------------------------------------------------

local gltfloader = {
	curr_factory 	= nil,
	temp_meshes 	= {},
}

------------------------------------------------------------------------------------------------------------

local function loadgltf( fname )

	--print(fname)
	-- Check for gltf - only support this at the moment. 
	print(fname)
	local valid = string.match(fname, ".*%.gltf$")
	assert(valid)

	local basepath = fname:match("(.*/)")

	-- Note: This can be replaced with io.open if needed.
	local gltfdata, error = sys.load_resource(fname)	
	-- local gltfobj = jsonloader.parse( gltfdata )
	local gltfobj = json.decode( gltfdata )
	gltfobj.basepath = basepath
	-- pprint(gltfobj)

	-- load buffers 
	for k,v in pairs(gltfobj.buffers) do 

		if(v.uri) then 

			-- The uri _may_ be a base64 stream 
			local ss, se = string.find(v.uri, "octet%-stream;base64,")
			if(ss ~= nil) then 
				local byteData = string.sub(v.uri, se+1, -1)
				v.data = b64.decode(byteData)
				print("Data:", v.byteLength, #v.data) 
			-- Its likely a file
			else
				-- local fh = io.open(basepath..v.uri, "rb")
				local data, error = sys.load_resource(basepath..v.uri)	
				if(data) then 
					v.data = data
				else 
					print("Error: Cannot load gltf binary file ["..basepath..v.uri.."]") 
				end
			end 

			assert(v.byteLength == #v.data)
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
	local gomeshname  = parent.."/node"..string.format("%04d", n)
	local gochild = mpool.gettemp( gomeshname )
	local gochildname = gochild.."#temp"
	
	local thisnode = gltfobj.nodes[n] 
	print("Name:", thisnode.name, parent)	
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
					gltfloader:loadimages( gltfobj, gochildname, bcolor + 1, 0 )
				end 

				if(pbrmetallicrough.metallicRoughnessTexture) then 
					local bcolor = pbrmetallicrough.metallicRoughnessTexture.index
					local res = gltfobj.images[bcolor + 1]
					gltfloader:loadimages( gltfobj, gochildname, bcolor + 1, 1 )
				end
			end
			local pbremissive = mat.emissiveTexture
			if(pbremissive) then 
				local bcolor = pbremissive.index
				local res = gltfobj.images[bcolor + 1]
				gltfloader:loadimages( gltfobj, gochildname, bcolor + 1, 2 )
			end
			local pbrnormal = mat.normalTexture
			if(pbrnormal) then  
				local bcolor = pbrnormal.index
				local res = gltfobj.images[bcolor + 1]
				gltfloader:loadimages( gltfobj, gochildname, bcolor + 1, 3 )
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
		gltf.verts = {}
		local posidx = prim.attributes["POSITION"]
		if(posidx) then 
			local aidx = gltfobj.accessors[posidx + 1]
			bv = gltfobj.bufferViews[aidx.bufferView + 1]
			buffer = gltfobj.buffers[bv.buffer + 1]
			-- Get positions (or verts) 
			geomextension.setbufferfloatsfromtable(bv.byteOffset or 0, bv.byteLength or 0, buffer.data, gltf.verts)
		end
		
		-- Get uvs accessor
		gltf.uvs = {}
		local texidx = prim.attributes["TEXCOORD_0"]
		if(texidx) then 
			local aidx = gltfobj.accessors[texidx + 1]
			bv = gltfobj.bufferViews[aidx.bufferView + 1]
			buffer = gltfobj.buffers[bv.buffer + 1]
			geomextension.setbufferfloatsfromtable(bv.byteOffset or 0, bv.byteLength or 0, buffer.data, gltf.uvs)
		end 

		-- Get normals accessor
		gltf.normals = {}
		local normidx = prim.attributes["NORMAL"]
		if(normidx) then 
			local aidx = gltfobj.accessors[normidx + 1]
			bv = gltfobj.bufferViews[aidx.bufferView + 1]
			buffer = gltfobj.buffers[bv.buffer + 1]
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
			local child = gltfobj.nodes[v + 1]
			self:makeNodeMeshes( gltfobj, gochild, gochildname, v + 1)
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

	-- Note: Meshname is important for idenifying a mesh you want to be able to modify
	if(pobj == nil) then 
		pobj = mpool.gettemp( meshname )
	end 
	
	-- Parent mesh
	local goname = msg.url(nil, pobj, meshname)	
	local goscript = pobj.."#script"
	
	-- local gltfobj = tinygltf_extension.loadmodel( fname )
	local gltfobj = loadgltf( fname )
		
	local gltfmesh 	= geom:New(goname)
	geom:New(goname, 1.0)
	tinsert(geom.meshes, goname)

	--gltfloader:loadimages( gltfobj, pobj, goname )	
	
	-- Go through the scenes (we will only care about the first one initially)
	for k,v in pairs(gltfobj.scenes) do
		if k > 1 then break end

		-- Go through the scenes nodes - this is recursive. nodes->children->children..
		local childtag = nil
		for ni, n in pairs(v.nodes) do
			self:makeNodeMeshes( gltfobj, goname, pobj, n + 1)
		end 
	end

	return pobj
end

-- --------------------------------------------------------------------------------------------------------
-- Uses a tempobject within the pool, assigns a goname to the mapping (must be unique). 
-- Initial pos vec3 or initial rotation quat can be provided. Otherwise will be default
function gltfloader:addmesh( filepath, name, initpos, initrot )

	local pos = initpos or vmath.vector3(0, 0, 0)
	local rot = initrot or vmath.quat_rotation_y(0.0)
	local goname = mpool.gettemp( name, filepath )

	gltfloader:load(filepath, goname, name)
	go.set_rotation(rot, goname)
	go.set_position(pos, goname)

	return goname 
end 

------------------------------------------------------------------------------------------------------------

return gltfloader

------------------------------------------------------------------------------------------------------------
