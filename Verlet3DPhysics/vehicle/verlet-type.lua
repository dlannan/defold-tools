

local Gravity       = 0.98
local tinsert       = table.insert
local VerletList    = {}


local function defaultHandler( V, delta ) 

    -- Insert rigid body collision here!!
    if V.y < 0.0 then
        V.collided = true
        V.y = 0.0
        V.oy = 0.0 - (V.dy/3)
    end
end

--------------------------------------------------------------------------------------

local TVerlet = {}

--------------------------------------------------------------------------------------

function TVerlet.new()

    local vlet = {
        x         = 0.0,
        y         = 0.0,
        z         = 0.0,
        dx         = 0.0,
        dy         = 0.0,
        dz         = 0.0,
        ox         = 0.0,
        oy         = 0.0,
        oz         = 0.0,

        ID          = 0,
        active      = false,
        mass        = 0.0,
        collided    = false,
        
        colliderHandler = defaultHandler,
    }
    return vlet    
end 

--------------------------------------------------------------------------------------

function TVerlet.applyForces( V, delta )

    V.y = V.y - Gravity * delta
end 

--------------------------------------------------------------------------------------

function TVerlet.update( V, delta )

    if V.active == true then
         
        if V.collided == true then
            V.fric = 0.8
        else
            V.fric = 1
        end
        V.collided = false

        V.dx = (V.x-V.ox)*V.fric  
        V.dy = (V.y-V.oy)*V.fric
        V.dz = (V.z-V.oz)*V.fric 
        
        V.ox = V.x
        V.oy = V.y
        V.oz = V.z
        
        V.x = V.x + V.dx
        V.y = V.y + V.dy 
        V.z = V.z + V.dz

        TVerlet.applyForces(V, delta)
        V.colliderHandler(V, delta)        
    else
        V.x = V.ox
        V.y = V.oy
        V.z = V.oz
    end
end

--------------------------------------------------------------------------------------
	
TVerlet.create = function(x,y,z,ID,active)

    if(active == nil) then active = true end
    local V = TVerlet.new()
    V.x         = x
    V.y         = y
    V.z         = z
    V.ID        = ID
    V.ox        = x
    V.oy        = y
    V.oz        = z
    V.active    = active 
    V.colliderHandler = defaultHandler
    
    tinsert(VerletList, V)
    return V
end

--------------------------------------------------------------------------------------

TVerlet.updateall = function( delta )
    
    for k,v in pairs(VerletList) do
        TVerlet.update( v, delta )
    end		
end

--------------------------------------------------------------------------------------

return TVerlet
