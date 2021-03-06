
require("utils.copy")

local tinsert = table.insert

local ConstraintList  = {}

--------------------------------------------------------------------------------------

local TConstraint = { }

--------------------------------------------------------------------------------------

TConstraint.update = function( C )

    local V1 = C.V1 
    local V2 = C.V2
    
    local tx = V1.x-V2.x
    local ty = V1.y-V2.y
    local tz = V1.z-V2.z
    
    local dist = math.sqrt(tx*tx + ty*ty + tz*tz)
    tx = tx * 0.5    
    ty = ty * 0.5    
    tz = tz * 0.5    

    local diff = 0.0
    if dist ~= 0.0 then
        diff = (dist - C.Length) / dist
    end
    
    V1.x = V1.x - diff * tx
    V1.y = V1.y - diff * ty
    V1.z = V1.z - diff * tz
    V2.x = V2.x + diff * tx
    V2.y = V2.y + diff * ty
    V2.z = V2.z + diff * tz
end 

--------------------------------------------------------------------------------------

TConstraint.constrain = function( V1, V2 )

    local C = {
        V1     = V1,
        V2     = V2,
        Length = math.sqrt( math.pow(V1.x - V2.x, 2) + math.pow(V1.y - V2.y, 2) + math.pow(V1.z - V2.z, 2) ),
    }
    tinsert(ConstraintList, C)
    return C
end

--------------------------------------------------------------------------------------

TConstraint.updateall = function( delta )

    for k,C in pairs(ConstraintList) do
        TConstraint.update( C, delta )
    end
end

--------------------------------------------------------------------------------------

return TConstraint

--------------------------------------------------------------------------------------
