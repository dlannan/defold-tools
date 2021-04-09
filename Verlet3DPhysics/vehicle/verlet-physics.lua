

local tinsert   = table.insert

local mpool 		= require("gltfloader.meshpool")
local geom 			= require("gltfloader.geometry-utils")
local imageutils 	= require("gltfloader.image-utils")

--------------------------------------------------------------------------------------

local Gravity = .1

local TBox          = require("vehicle.verlet-box")
local TVerlet       = require("vehicle.verlet-type")
local TConstraint   = require("vehicle.verlet-constraints")

local rootnode = TVerlet.create( -3, 10, 0, 999, false)

local BoxStack = {}    
--------------------------------------------------------------------------------------

local physics     = {}

physics.init = function()

    local rnode = mpool.gettemp( "rootnode" )
    geom:GenerateCube( rnode.."#temp", 1, 1 )
    imageutils.loadimage( rnode.."#temp", "/assets/images/green.png", 0 )
    go.set_position( vmath.vector3(-3, 10, 0), rnode )
        
    -- Constrain a bunch of boxes
    for i = 1, 50 do
        local vbox = TBox.create(2, 2, 0, i*0.5, 8, 0, i)        
        local box = mpool.gettemp( "box"..i )
        
        geom:GenerateCube( box.."#temp", 1, 1 )
        imageutils.loadimage( box.."#temp", "/assets/images/green.png", 0 )
        go.set_position( vmath.vector3(vbox.V1.x, vbox.V1.y, vbox.V1.z), box )

        if i == 1 then 
            rootC = TConstraint.constrain(vbox.V1, rootnode)	
            pprint(rootC)
        end
        BoxStack[i] = { vbox = vbox, gobox = box }
    end
end

--------------------------------------------------------------------------------------

physics.updateall = function( objHandler, delta )

    TConstraint.updateall( delta )
    TVerlet.updateall( delta )
    
    objHandler( BoxStack, 50 )
end

--------------------------------------------------------------------------------------

return physics 
