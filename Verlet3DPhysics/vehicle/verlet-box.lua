
local Tconstraint = require("vehicle.verlet-constraints")
local TVerlet     = require("vehicle.verlet-type")

--------------------------------------------------------------------------------------

local TBox = {
    
    -- Verlets
	V1,
	V2,
	V3,
	V4,
}

--------------------------------------------------------------------------------------

TBox.create = function(sx, sy, sz, px, py, pz, id)
        
    local B = {}
    B.V1 = TVerlet.create( 0+px, sy+py, pz, id)
    B.V2 = TVerlet.create(sx+px,  0+py, pz, id)
    B.V3 = TVerlet.create( 0+px,  0+py, pz, id)
    B.V4 = TVerlet.create(sx+px, sy+py, pz, id)
   
    Tconstraint.constrain(B.V1, B.V2)
    Tconstraint.constrain(B.V1, B.V3)
    Tconstraint.constrain(B.V2, B.V3)
    Tconstraint.constrain(B.V1, B.V4)
    Tconstraint.constrain(B.V2, B.V4)
    Tconstraint.constrain(B.V3, B.V4)
    return B
end

--------------------------------------------------------------------------------------

return TBox