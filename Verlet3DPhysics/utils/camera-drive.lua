
-- A camera controller so I can pan and move around the scene. 
-- Enable/Disable using keys
--------------------------------------------------------------------------------
local cameradrive = {

	lookvec 	= vmath.vector3(),
	pos			= vmath.vector3(),
	movevec 	= vmath.vector3(),
	
	xangle 		= 0.0,
	yangle		= 0.0,
}

--------------------------------------------------------------------------------
-- A simple handler, can be easily replaced
local function defaulthandler( self, delta )

	local pitch		= -cameradrive.xangle
	local yaw		= cameradrive.yangle

	xzLen = math.cos(yaw)
	cameradrive.lookvec.z = -xzLen * math.cos(pitch)
	cameradrive.lookvec.y = math.sin(yaw)
	cameradrive.lookvec.x = xzLen * math.sin(-pitch)
	
	-- do some default movement stuff
	cameradrive.movevec = vmath.vector3( cameradrive.lookvec.x * cameradrive.speed * delta,
		cameradrive.lookvec.y * cameradrive.speed * delta,
		cameradrive.lookvec.z * cameradrive.speed * delta)
		
	cameradrive.pos =  vmath.vector3(cameradrive.pos.x + cameradrive.movevec.x, 
		cameradrive.pos.y + cameradrive.movevec.y,
		cameradrive.pos.z + cameradrive.movevec.z)

	local xrot = vmath.quat_rotation_y(pitch)
	local yrot = vmath.quat_rotation_x(yaw)
	cameradrive.rot = xrot * yrot 
	
	go.set_rotation( cameradrive.rot, cameradrive.cameraobj )		
	go.set_position( cameradrive.pos, cameradrive.cameraobj )
end

--------------------------------------------------------------------------------

cameradrive.init = function( cameraobj, speed, handler )

	cameradrive.cameraobj = cameraobj 
	cameradrive.speed = speed or 0.0
	cameradrive.handler = handler or defaulthandler

	cameradrive.pos = go.get_position(cameraobj)
	cameradrive.rot = go.get_rotation(cameraobj)
	
	cameradrive.enabled = true 		-- enabled by default
end 

--------------------------------------------------------------------------------

cameradrive.update = function( self, delta )

	if(cameradrive.enabled ~= true) then return end
	if(cameradrive.handler) then cameradrive.handler( self, delta ) end
end

--------------------------------------------------------------------------------

return cameradrive

--------------------------------------------------------------------------------