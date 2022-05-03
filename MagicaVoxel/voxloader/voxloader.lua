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
		dict[key] = value
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
	local maxSize = math.max(x, y)
	--print("[ SIZE ] ",x,y,z)
	return {x=x,y=y,z=z}, maxSize
end
------------------------------------------------------------------------------------------------------------

local function doChunkXYZI( vdata, csize, msize )
	local numvoxels = vdata:unpack("i4")
	--print("[ VOXELS ] ", numvoxels)
	local offx = -(math.floor((csize.x + 0.5) / 2))
	local offy = -(math.floor((csize.y + 0.5) / 2))
	local offz = -(math.floor((csize.z + 0.5) / 2))
	local voxels = {}
	for i=1, numvoxels do
		local x,y,z,i = vdata:unpack("BBBB")
		tinsert(voxels, { x=x+offx, y=y+offy, z=z+offz, i=i })
		--pprint("[ XYZI ] ", z,y,z,i)
	end 
	assert( numvoxels == #voxels )
	return { numvoxels = numvoxels, voxels = voxels }
end
------------------------------------------------------------------------------------------------------------

local function doChunkRGBA( vdata )
	local colorpalette = {}
	tinsert(colorpalette, {0,0,0,0})
	for i=1, 255 do 
		local r,g,b,a = vdata:unpack("BBBB")
		tinsert(colorpalette, {r=r * 0.0039125,g=g * 0.0039125,b=b * 0.0039125,a=a * 0.0039125})
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
	return { nodeid=nodeid, childid=childid, layerid=layerid, frames=frames }
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
	voxobj.nodes 	= {}
	voxobj.transforms = {}
	voxobj.shapes 	= {} 
	voxobj.layers 	= {} 
	local currentsize = nil
	local maxSize = 0

	-- Keep reading until we have it all.
	while( dlen - vdata.pos > 4  ) do 

		-- Read chunk first:
		local chunkid = vdata:bytes(4)
		print(chunkid)
		local bcontent = vdata:unpack("i4")
		local bchildren = vdata:unpack("i4")
		--print(bcontent, bchildren)
	
		if(chunkid == "PACK") then 
			doChunkPack(vdata)
		elseif(chunkid == "MAIN") then 
			doChunkMain(vdata)
		elseif(chunkid == "SIZE") then 
			currentsize, maxSize = doChunkSize(vdata)
			tinsert( voxobj.sizes, currentsize )
		elseif(chunkid == "XYZI") then 
			tinsert( voxobj.xyzi, doChunkXYZI(vdata, currentsize, maxSize) )
		elseif(chunkid == "RGBA") then 
			voxobj.palette = doChunkRGBA(vdata)
		elseif(chunkid == "nTRN") then 
			local tform = doChunknTRN(vdata)
			voxobj.transforms[tform.nodeid] = tform
		elseif(chunkid == "nGRP") then 
			local children = doChunknGRP(vdata)
			for k,v in ipairs(children) do tinsert( voxobj.nodes, v ) end
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
		--print("[ DLEN ] ", vdata.pos, dlen)
	end

	voxobj.scenes = {
		voxobj
	}
	pprint(voxobj.groups)
end

------------------------------------------------------------------------------------------------------------

local function genMeshObjects( voxdata )
end

------------------------------------------------------------------------------------------------------------
local VSID 	= 1024

local MAXXSIZ = 256 -- //Default values
local MAXYSIZ = 256
local BUFZSIZ = 256 -- //(BUFZSIZ&7) MUST == 0
local LIMZSIZ = 255 -- //Limited to 255 by: char ylen[?][?]

local function parsevox( voxdata )

	-- Iterate through the file. Stepping and converting bytes.
	-- print(blob.backend)
	local voxobj = {}

	local vdata = blob.new(voxdata)
	local dlen = #voxdata 
	print("[ VOX DATA SIZE ] ", dlen)

	vdata:mark()
	-- Sometimes the header isnt included.
	if(vdata:bytes(4) == "VOX ") then 
		local ver = vdata:unpack("i4")
		processChunks(vdata, dlen, voxobj)
	else 
		vdata:restore()
		local xd = vdata:unpack("I4")
		local yd = vdata:unpack("I4")
		local zd = vdata:unpack("I4")
		pprint(xd, yd, zd)
		local xpiv = xd*0.5
		local ypiv = yd*0.5
		local zpiv = zd*0.5
		pprint(xpiv, ypiv, zpiv)

		voxobj.sizes = { { xd, yd, zd } }

		local voxels = {}
		
		for x = 0, xd - 1 do
			for y = 0, yd - 1 do 
				local j = ((x*MAXYSIZ) + y ) * BUFZSIZ
				for z = 0, zd - 1 do 
					local c = vdata:unpack("B")
					if(c ~= 255) then 
						local idx = bit.rshift( j + z, 5 )
						local newvox = bit.bnot( bit.lshift(1, (j + z) ) )
						tinsert( voxels, { x = x-xpiv, y = y-ypiv, z = z-zpiv, i = c } )
						-- vbit[idx] = bit.band( vbit[idx] or 0, newvox ) 
					end 
				end 
			end 
		end

		voxobj.xyzi = { { numvoxels = #voxels, voxels = voxels } }

		local paldata = {}
		--tinsert(paldata, {r=0,g=0,b=0})
		for i = 1, 256 do 
			local r,g,b = vdata:unpack("BBB")
			tinsert(paldata, {r=r * 0.0039125,g=g * 0.0039125,b=b * 0.0039125})	
		end
		voxobj.palette = paldata
		pprint("[ PALETTE ] ", palette)

		voxobj.nodes 	= { { id = 1 } }
		voxobj.scenes 	= {
			voxobj
		}
	end

	-- Process voxels and make vertbuffers for each voxel.
	-- Should create scenes, meshes etc the same as gtlfobj.
	genMeshObjects( voxobj )

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
-- Vertex value based on 0,0,0 center of cube with 1 in size. 

local vertexPositions = {
	-- Face 1 - Front
	{ x = -0.5, y = 0.5, z = -0.5 },
	{ x = 0.5, y = 0.5, z = -0.5 },
	{ x = 0.5, y = -0.5, z = -0.5 },
	{ x = -0.5, y = 0.5, z = -0.5 },
	{ x = 0.5, y = -0.5, z = -0.5 },
	{ x = -0.5, y = -0.5, z = -0.5 },
	-- Face 2 - Left
	{ x = -0.5, y = 0.5, z = 0.5 },
	{ x = -0.5, y = 0.5, z = -0.5 },
	{ x = -0.5, y = -0.5, z = -0.5 },
	{ x = -0.5, y = 0.5, z = 0.5 },
	{ x = -0.5, y = -0.5, z = -0.5 },
	{ x = -0.5, y = -0.5, z = 0.5 },
	-- Face 3 - Rear
	{ x = 0.5, y = 0.5, z = 0.5 },
	{ x = -0.5, y = 0.5, z = 0.5 },
	{ x = -0.5, y = -0.5, z = 0.5 },
	{ x = 0.5, y = 0.5, z = 0.5 },
	{ x = -0.5, y = -0.5, z = 0.5 },
	{ x = 0.5, y = -0.5, z = 0.5 },
	-- Face 4 - Right
	{ x = 0.5, y = -0.5, z = 0.5 },
	{ x = 0.5, y = -0.5, z = -0.5 },
	{ x = 0.5, y = 0.5, z = -0.5 },
	{ x = 0.5, y = -0.5, z = 0.5 },
	{ x = 0.5, y = 0.5, z = -0.5 },
	{ x = 0.5, y = 0.5, z = 0.5 },
	-- Face 5 - Top
	{ x = -0.5, y = 0.5, z = -0.5 },
	{ x = -0.5, y = 0.5, z = 0.5 },
	{ x = 0.5, y = 0.5, z = 0.5 },
	{ x = -0.5, y = 0.5, z = -0.5 },
	{ x = 0.5, y = 0.5, z = 0.5 },
	{ x = 0.5, y = 0.5, z = -0.5 },
	-- Face 6 - Bottom
	{ x = -0.5, y = -0.5, z = -0.5 },
	{ x = 0.5, y = -0.5, z = -0.5 },
	{ x = 0.5, y = -0.5, z = 0.5 },
	{ x = -0.5, y = -0.5, z = -0.5 },
	{ x = 0.5, y = -0.5, z = 0.5 },
	{ x = -0.5, y = -0.5, z = 0.5 },	
}

------------------------------------------------------------------------------------------------------------
-- All faces have same uv coords
local uvcoords = {
	{ x = 0.0, y = 0.0 },
	{ x = 1.0, y = 0.0 },
	{ x = 1.0, y = 1.0 },
	{ x = 0.0, y = 1.0 },
}
------------------------------------------------------------------------------------------------------------
-- Only 6 real normals to worry about
local normals = {
	{ x = 0.0, y = 0.0, z = -1.0 },	-- front
	{ x = -1.0, y = 0.0, z = 0.0 },	-- left
	{ x = 0.0, y = 0.0, z = 1.0 },	-- rear
	{ x = 1.0, y = 0.0, z = 0.0 }, 	-- right
	{ x = 0.0, y = 1.0, z = 0.0 },  -- top
	{ x = 0.0, y = -1.0, z = 0.0 }, -- bottom
}

local function matrixToQuat( m )

	local q = vmath.quat()
	q.w= math.sqrt(1 + m.m00 + m.m11 + m.m22) /2
	q.x = (m.m21 - m.m12)/( 4 *q.w)
	q.y = (m.m02 - m.m20)/( 4 *q.w)
	q.z = (m.m10 - m.m01)/( 4 *q.w)
	return q 
end 


------------------------------------------------------------------------------------------------------------

function voxloader:makeVoxelMeshes( voxobj, goname, parent, n )

	-- Each node can have a mesh reference. If so, get the mesh data and make one, set its parent to the
	--  parent node mesh
	local gomeshname  = parent.."/node"..string.format("%04d", n)
	local gochild = mpool.gettemp( gomeshname )
	local gochildname = gochild.."#temp"
	
	local thisnode = voxobj.nodes[n] 

	print("ID:", thisnode.id)	
	if(thisnode.id) then 
		
		-- Temp.. 
		local vox = {}
		
		-- Get indices from accessor 
		local thisvox = { 
			sizes 	= voxobj.sizes[n],
			xyzi 	= voxobj.xyzi[n].voxels
		}

		if(thisvox.xyzi == nil) then print("No Voxels?"); return end 		

		-- Material is always white and we set vertex color
		-- voxloader:loadimages( voxobj, gochildname, 1, 0 )

		vox.indices = {}
		vox.verts 	= {}
		vox.uvs 	= {}
		vox.normals = {}
		local vertoff = 0
		
		-- iterate voxels in object and make indices, vertices, uvs and normals for a single voxel
		local size = thisvox.sizes
		for k,v in pairs(thisvox.xyzi) do 
			
			-- Indices are 4 quads x 6 sides. In order.
			for i=1, 36 do
				tinsert(vox.indices, vertoff); vertoff = vertoff + 1
			end 

			-- Get positions (or verts) 
			for i=1, 36 do
				local vert = vertexPositions[i]
				tinsert(vox.verts, vert.x + v.x)
				tinsert(vox.verts, vert.y + v.y)
				tinsert(vox.verts, vert.z + v.z)
			end 
		
			for i=1, 36 do
				local idx = ((i-1) % 4) + 1
				local uv = uvcoords[ idx ]
				tinsert(vox.uvs, uv.x)
				tinsert(vox.uvs, uv.y)
			end 
			
			for i=1, 36 do
				-- local norm = normals[((i-1) % 6) + 1]
				local color = voxobj.palette[v.i+1]
				tinsert(vox.normals, color.r)
				tinsert(vox.normals, color.g)
				tinsert(vox.normals, color.b)
			end 
		end

		if(voxobj.transforms) then 
			local trans = voxobj.transforms[thisnode.id]
			if(trans and trans.frames[1]) then 
				pprint(trans)
				local pos = { 0, 0, 0 }
				if(trans.frames[1].dict._t) then 
					pos = { string.match(trans.frames[1].dict._t, "(%-?%d+) (%-?%d+) (%-?%d+)") }
					go.set_position(vmath.vector3(pos[1], pos[2], pos[3]), gochildname) 
				end
				if(trans.frames[1].dict._r) then 
					local rot = string.match(trans.frames[1].dict._r, "(%d+)")
					local m = vmath.matrix4()
					m.m00 	= 0.0
					m.m11 	= 0.0
					-- m.m22 	= 0.0
					-- m.m20 	= 1.0 
					m.m33 	= 1.0
					local r1idx = bit.band(rot, 3)
					local r2idx = bit.rshift(bit.band(rot, 12), 2)
					print(r2idx)
					m["m0"..r1idx] = 1.0
					m["m1"..r2idx] = 1.0
				
					if(bit.band(rot, 16) == 16) then m["m0"..r1idx] = -1.0 end
					if(bit.band(rot, 32) == 32) then m["m1"..r2idx] = -1.0 end
					if(bit.band(rot, 64) == 64) then m.m22 = -1.0 end
					--pprint(m, r1idx, r2idx)
					go.set_rotation(matrixToQuat(m), gochildname) 
				end
			end
		end 
		
		geom:makeMesh( gochildname, vox.indices, vox.verts, vox.uvs, vox.normals )

	-- No mesh.. try children
	end 

	-- Parent this mesh to the incoming node 
	go.set_parent(go.get_id(gochild), parent)	
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
		
	local voxmesh 	= geom:New(goname)
	geom:New(goname, 1.0)
	tinsert(geom.meshes, goname)

	--voxloader:loadimages( voxobj, pobj, goname )	
	
	-- Go through the scenes (we will only care about the first one initially)
	for k,v in pairs(voxobj.scenes) do
		if k > 1 then break end

		-- Go through the scenes nodes - this is recursive. nodes->children->children..
		local childtag = nil
		for ni, n in pairs(v.nodes) do
			self:makeVoxelMeshes( voxobj, goname, pobj, ni)
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
