

local tinsert   = table.insert

local mpool 		= require("gltfloader.meshpool")
local geom 			= require("gltfloader.geometry-utils")
local imageutils 	= require("gltfloader.image-utils")

--------------------------------------------------------------------------------------

local Gravity = .1

local TBox          = require("vehicle.verlet-box")
local TVerlet       = require("vehicle.verlet-type")
local TConstraint   = require("vehicle.verlet-constraints")

local BoxStack = {}    
--------------------------------------------------------------------------------------

local physics     = {}

physics.init = function()

    local rootnode1 = TVerlet.create( -3, 10, 0, 999, false)
    local rootnode2 = TVerlet.create( 3, 20, 0, 999, false)
    local storednode = nil
    
    -- Random box test
    local rnode = mpool.gettemp( "rootnode1" )
    geom:GenerateCube( rnode.."#temp", 1, 1 )
    imageutils.loadimage( rnode.."#temp", "/assets/images/green.png", 0 )
    go.set_position( vmath.vector3(rootnode1.x, rootnode1.y, 0), rnode )

    rnode = mpool.gettemp( "rootnode2" )
    geom:GenerateCube( rnode.."#temp", 1, 1 )
    imageutils.loadimage( rnode.."#temp", "/assets/images/green.png", 0 )
    go.set_position( vmath.vector3(rootnode2.x, rootnode2.y, 0), rnode )
            
    -- Constrain a bunch of boxes
    for i = 1, 50 do
        local vbox = TBox.create(2, 2, 0, i*0.5, 8 + math.random(5), 0, i)        
        local box = mpool.gettemp( "box"..i )
        -- Add a z wobble 
        vbox.V1.z = vbox.V1.z + math.random() * 0.1 - 0.05
        
        geom:GenerateCube( box.."#temp", 1, 1 )
        imageutils.loadimage( box.."#temp", "/assets/images/green.png", 0 )
        go.set_position( vmath.vector3(vbox.V1.x, vbox.V1.y, vbox.V1.z), box )

        if i == 1 then 
            TConstraint.constrain(vbox.V1, rootnode1)
        end

        -- Chain two boxes to a fixed node
        if i == 2 then 
            TConstraint.constrain(vbox.V1, rootnode2)
            storednode = vbox.V2
        end
        if i == 3 then 
            TConstraint.constrain(vbox.V1, storednode)
        end
        
        BoxStack[i] = { vbox = vbox, gobox = box }
    end
end

--------------------------------------------------------------------------------------

physics.updateall = function( objHandler, delta )

    TVerlet.updateall( delta )
    TConstraint.updateall( delta )
        
    objHandler( BoxStack, 50 )
end

--------------------------------------------------------------------------------------

return physics 
