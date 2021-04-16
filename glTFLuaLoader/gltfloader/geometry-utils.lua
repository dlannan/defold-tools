------------------------------------------------------------------------------------------------------------
--    /// Some default models so can easily do simple things
--    /// Also this is a reference for math generated geometry 
--    /// 

-- luajit helpers :) 
local tinsert 	= table.insert 
local tremove 	= table.remove


------------------------------------------------------------------------------------------------------------

local geom = {

	meshes 		= {},
}
------------------------------------------------------------------------------------------------------------

function geom:makeMesh( goname, indices, verts, uvs, normals )

--	local mesh = resource.load("/assets/gotemplate/temp.mesh")
	local res = go.get(goname, "vertices")
	local iverts = #indices

	local meshdata = {}
	-- positions are required (should assert or something)
	tinsert(meshdata, { name = hash("position"), type=buffer.VALUE_TYPE_FLOAT32, count = 3 } )
	if(normals) then tinsert(meshdata, { name = hash("normal"), type=buffer.VALUE_TYPE_FLOAT32, count = 3 } ) end
	if(uvs) then tinsert(meshdata, { name = hash("texcoord0"), type=buffer.VALUE_TYPE_FLOAT32, count = 2 } ) end
	--{ name = hash("color0"), type=buffer.VALUE_TYPE_FLOAT32, count = 4 }
	
	local meshbuf = buffer.create(iverts, meshdata)

	geomextension.setbufferbytesfromtable( meshbuf, "position", indices, verts )
	if(normals) then geomextension.setbufferbytesfromtable( meshbuf, "normal", indices, normals ) end
	if(uvs) then geomextension.setbufferbytesfromtable( meshbuf, "texcoord0", indices, uvs ) end 
		
	-- set the buffer with the vertices on the mesh
	resource.set_buffer(res, meshbuf)
end

------------------------------------------------------------------------------------------------------------

function geom:New(goname, sz)

	local props = {}
	props[goname] = { }
end

------------------------------------------------------------------------------------------------------------

function geom:GenerateCube(goname, sz, d )

	geom:New(goname, 1.0)
	tinsert(self.meshes, goname)

	local verts = {}
	local indices = {} 
	local normals = {}
	local uvs = {}			-- TODO - generate UVS

	local vcount = 1
	local ucount = 1
	local ncount = 1
	local icount = 1
	local index = 1

	-- Start with a cube. Then for number x/y sizes iterate each side of the cube
	-- For each side of the cube cal vert trace back to center of cube, then recalc vert based on radius.
	-- Collect verts in order, making triangles along the way	

	local targets = {  
		[1] = function( a, b ) return { a, -sz, b, -1, 0.25, 0.333, 0, -1, 0 }; end,
		[2] = function( a, b ) return { a, b, -sz, 1, 0.0, 0.333, 0, 0, -1 }; end,
		[3] = function( a, b ) return { a, b, sz, -1, 0.25, 0.333, 0, 0, 1 }; end,
		[4] = function( a, b ) return { -sz, b, a, -1, 0.25, 0.333, -1, 0, 0 }; end,
		[5] = function( a, b ) return { sz, b, a, 1, 0.0, 0.333, 1, 0, 0 }; end,
		[6] = function( a, b ) return { a, sz, b, 1, 0.0, 0.333, 0, 1, 0 }; end
	}

	local startuvs = {
		[1] = { 0.25, 0.666 },		-- Ground
		[2] = { 0.25, 0.333 },		-- Front
		[3] = { 0.75, 0.333 },		-- Back
		[4] = { 0.0, 0.333 },		-- Left
		[5] = { 0.5, 0.333 },		-- Right
		[6] = { 0.25, 0.0 }			-- Sky
	}

	function make_vert( icount, f, uv1, uv2, uv1fac, uv2fac, sz)

		tinsert(indices, icount)
		tinsert(verts, f[1])
		tinsert(verts, f[2])
		tinsert(verts, f[3])
		tinsert(normals, f[7])
		tinsert(normals, f[8])
		tinsert(normals, f[9])
		tinsert(uvs, uv1 + f[5] + f[4] * uv1fac)
		tinsert(uvs, uv2 + f[6] - uv2fac)

		index 	= #indices + 1
		vcount 	= #verts + 1
		ucount 	= #uvs + 1
		ncount 	= #normals + 1
	end	
		
	local stepsize = sz * 2 / d
	for key, func in ipairs(targets) do

		local uv1 = startuvs[key][1]
		local vstep = 1.0 / d

		local amult = 1.0 / ( 2.005 * sz * 4.0 )
		local bmult = 1.0 / ( 2.005 * sz * 3.0 )

		for a = -sz, sz-stepsize, stepsize do

			local uv2 = startuvs[key][2]
			for b = -sz, sz-stepsize, stepsize do

				local switch = false
				if(key == 3 or key == 4 or key == 1) then switch = true end

				if(switch == false) then 
					local v = func(a, b)
					make_vert(icount, v, uv1, uv2, (a + sz) * amult, (b + sz) * bmult)
					local x = func(a+stepsize, b)
					make_vert(icount-1, x, uv1, uv2, (a + sz + stepsize) * amult, (b + sz) * bmult)

				else 
					local x = func(a+stepsize, b)
					make_vert(icount, x, uv1, uv2, (a + sz + stepsize) * amult, (b + sz) * bmult)
					local v = func(a, b)
					make_vert(icount-1, v, uv1, uv2, (a + sz) * amult, (b + sz) * bmult)
				end				

				local w = func(a, b+stepsize)
				make_vert(icount+1, w, uv1, uv2, (a + sz) * amult, (b + sz + stepsize) * bmult)
								
				if(switch == false) then 
					local x = func(a+stepsize, b)
					make_vert(icount+3, x, uv1, uv2, (a + sz + stepsize) * amult,  (b + sz) * bmult)
					local y = func(a+stepsize,b+stepsize)
					make_vert(icount+2, y, uv1, uv2, (a + sz + stepsize) * amult,  (b + sz + stepsize) * bmult)
				else 
					local y = func(a+stepsize,b+stepsize)
					make_vert(icount+3, y, uv1, uv2, (a + sz + stepsize) * amult,  (b + sz + stepsize) * bmult)
					local x = func(a+stepsize, b)
					make_vert(icount+2, x, uv1, uv2, (a + sz + stepsize) * amult,  (b + sz) * bmult)
				end 
									
				local w = func(a, b+stepsize)
				make_vert(icount+4, w, uv1, uv2, (a + sz) * amult,  (b + sz + stepsize) * bmult)

 				icount = icount + 6
 			end
		end
	end

	geom:makeMesh( goname, indices, verts, uvs, normals )
end

------------------------------------------------------------------------------------------------------------

function geom:GeneratePlane( goname, sx, sy, uvMult, offx, offy )

	offx     = offx or 0
	offy     = offy or 0
	uvMult   = uvMult or 1.0
	local plane 	= geom:New(goname)
	geom:New(goname, 1.0)
	tinsert(self.meshes, goname)
	
	local indices	= { 0, 1, 2, 0, 2, 3 }
	local verts		= { -sx + offx, 0.0, sy + offy, sx + offx, 0.0, sy + offy, sx + offx, 0.0, -sy + offy, -sx + offx, 0.0, -sy + offy }
	local uvs		= { 0.0, 0.0, uvMult, 0.0, uvMult, uvMult, 0.0, uvMult }
	local normals	= { 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0 }

	geom:makeMesh( goname, indices, verts, uvs, normals )
end


------------------------------------------------------------------------------------------------------------
-- 
function geom:GenerateSphere( goname, sz, d, inverted )

	if inverted == nil then inverted = 1.0 end
	geom:New(goname, 1.0)
	tinsert(self.meshes, goname)

	local verts 	= {}
	local indices 	= {} 
	local uvs 		= {}
	local normals 	= {}

	local vcount    = 1
	local ncount    = 1
	local ucount    = 1
	local icount    = 1
	local index     = 1

	-- Start with a cube. Then for number x/y sizes iterate each side of the cube
	-- For each side of the cube cal vert trace back to center of cube, then recalc vert based on radius.
	-- Collect verts in order, making triangles along the way
	function spherevec( vec )		
		local nvec = vmath.normalize( vmath.vector3( vec[1], vec[2], vec[3] ) )
		return { nvec.x * sz, nvec.y * sz, nvec.z * sz, vec[4], vec[5], vec[6] }
	end

	local targets = {  
		[1] = function( a, b ) return spherevec( { a, -sz, b, -1, 0.25, 0.333 } ); end,
		[2] = function( a, b ) return spherevec( { a, b, -sz, 1, 0.0, 0.333 } ); end,
		[3] = function( a, b ) return spherevec( { a, b, sz, -1, 0.25, 0.333 } ); end,
		[4] = function( a, b ) return spherevec( { -sz, b, a, -1, 0.25, 0.333 } ); end,
		[5] = function( a, b ) return spherevec( { sz, b, a, 1, 0.0, 0.333 } ); end,
		[6] = function( a, b ) return spherevec( { a, sz, b, 1, 0.0, 0.333 } ); end
	}

	local startuvs = {
		[1] = { 0.25, 0.666 },		-- Ground
		[2] = { 0.25, 0.333 },		-- Front
		[3] = { 0.75, 0.333 },		-- Back
		[4] = { 0.0, 0.333 },		-- Left
		[5] = { 0.5, 0.333 },		-- Right
		[6] = { 0.25, 0.0 }			-- Sky
	}

	local stepsize = sz * 2 / d
	for key, func in ipairs(targets) do

		local uv1 = startuvs[key][1]
		local vstep = 1.0 / d

		local amult = 1.0 / ( 2.005 * sz * 4.0 )
		local bmult = 1.0 / ( 2.005 * sz * 3.0 )

		for a = -sz, sz-stepsize, stepsize do

			local uv2 = startuvs[key][2]
			for b = -sz, sz-stepsize, stepsize do
				local toggle = 1

				local v = func(a, b)
				indices[index]  = icount+v[4] * inverted ; index = index + 1
				verts[vcount]   = v[1]; vcount = vcount + 1
				verts[vcount]   = v[2]; vcount = vcount + 1
				verts[vcount]   = v[3]; vcount = vcount + 1
				normals[ncount]   = v[1]; ncount = ncount + 1
				normals[ncount]   = v[2]; ncount = ncount + 1
				normals[ncount]   = v[3]; ncount = ncount + 1
				uvs[ucount]     = uv1 + v[5] + v[4] * (a + sz) * amult; ucount = ucount + 1
				uvs[ucount]     = uv2 + v[6] - (b + sz) * bmult; ucount = ucount + 1

				local x = func(a+stepsize, b)
				indices[index]  = icount ; index = index + 1
				verts[vcount]   = x[1]; vcount = vcount + 1
				verts[vcount]   = x[2]; vcount = vcount + 1
				verts[vcount]   = x[3]; vcount = vcount + 1
				normals[ncount]   = x[1]; ncount = ncount + 1
				normals[ncount]   = x[2]; ncount = ncount + 1
				normals[ncount]   = x[3]; ncount = ncount + 1
				uvs[ucount]     = uv1 + x[5] + x[4] * (a + sz + stepsize) * amult; ucount = ucount + 1
				uvs[ucount]     = uv2 + x[6] - (b + sz) * bmult; ucount = ucount + 1

				local w = func(a, b+stepsize)
				indices[index]  = icount-v[4] * inverted ; index = index + 1
				verts[vcount]   = w[1]; vcount = vcount + 1
				verts[vcount]   = w[2]; vcount = vcount + 1
				verts[vcount]   = w[3]; vcount = vcount + 1
				normals[ncount]   = w[1]; ncount = ncount + 1
				normals[ncount]   = w[2]; ncount = ncount + 1
				normals[ncount]   = w[3]; ncount = ncount + 1
				uvs[ucount]     = uv1 + w[5] + w[4] * (a + sz) * amult; ucount = ucount + 1
				uvs[ucount]     = uv2 + w[6] - (b + sz + stepsize) * bmult; ucount = ucount + 1

				local y = func(a+stepsize,b+stepsize)
				verts[vcount]   = y[1]; vcount = vcount + 1
				verts[vcount]   = y[2]; vcount = vcount + 1
				verts[vcount]   = y[3]; vcount = vcount + 1
				normals[ncount]   = y[1]; ncount = ncount + 1
				normals[ncount]   = y[2]; ncount = ncount + 1
				normals[ncount]   = y[3]; ncount = ncount + 1
				uvs[ucount]     = uv1 + y[5] + y[4] * (a + sz + stepsize) * amult; ucount = ucount + 1
				uvs[ucount]     = uv2 + y[6] - (b + sz + stepsize) * bmult; ucount = ucount + 1

				indices[index]  = icount+2-(1-v[4] * inverted) ; index = index + 1
				indices[index]  = icount+1-v[4] * inverted ; index = index + 1

				-- Build the extra tri from previous verts and one new one.
				indices[index]  = icount+1 ; index = index + 1
				icount = icount + 4
			end
		end
	end

	geom:makeMesh( goname, indices, verts, uvs, normals )
end

-- ------------------------------------------------------------------------------------------------------------
-- 
function geom:GeneratePyramid(goname, sz)

	geom:New(goname, 1.0)
	tinsert(self.meshes, goname)

	local verts 	= {   
		-sz, 0.0, -sz,  -sz, 0.0, sz,  sz, 0.0, sz,   sz, 0.0, -sz,
		0.0, sz, 0.0 
	}

	local uvs 		= { 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.5, 0.5 }
	local normals 	= {
		-.707, 0.0, -.707, -.707, 0.0, .707,  .707, 0.0, .707, .707, 0.0, -.707, 
		0.0, 1.0, 0.0
	}

	local indices 	= {
		0, 2, 1, 2, 0, 3,      					-- // Base
		0, 4, 3,  0, 1, 4,  1, 2, 4, 2, 3, 4,  	-- // Front Left Back Right
	}

	geom:makeMesh( goname, indices, verts, uvs, normals )
end

-- ------------------------------------------------------------------------------------------------------------
-- 
function geom:GenerateBlock( goname, sx, sy, sz, uvMult )

	if uvMult   == nil then uvMult = 1.0 end
	geom:New(goname, 1.0)
	tinsert(self.meshes, goname)

	local normals 	= {}
	local indices	= { 
		0, 1, 2,  2, 3, 0,  6, 5, 7,  7, 5, 4,
		4, 0, 7,  0, 3, 7,  5, 6, 1,  1, 6, 2,
		0, 4, 5,  0, 5, 1,  2, 7, 3,  2, 6, 7 
	}
	local verts		= { 
		-sx, sy, -sz,   sx, sy, -sz,    sx, -sy, -sz,   -sx, -sy, -sz,
		-sx, sy, sz,    sx, sy, sz,     sx, -sy, sz,    -sx, -sy, sz 
	}
	local uvs		= { 
		0.0, 0.0, uvMult, 0.0, uvMult, uvMult, 0.0, uvMult,
		uvMult, uvMult, 0.0, 0.0, 0.0, uvMult, uvMult, 0.0 
	}

	geom:makeMesh( goname, indices, verts, uvs, normals )
end

-- ------------------------------------------------------------------------------------------------------------
-- 

return geom