-- rotator.lua
-- Rotates an entity with a configurable speed and color.

-- Register variables to the inspector (Top-level calls show up immediately in Editor)
RegisterProperty("Rotation Speed", 90.0)
RegisterProperty("Active", true)
RegisterProperty("Spin Color", MakeColor(0, 255, 0, 255))

function OnStart(entity)
    Log("Rotator started for entity " .. entity)
end

function OnUpdate(entity, dt)
    local isActive = GetProperty("Active")
    if not isActive then return end
    
    local speed = GetProperty("Rotation Speed")
    local transform = GetTransform(entity)
    
    if transform then
        local currentRot = transform:GetRotation()
        transform:SetRotation(currentRot + speed * dt)
    end
    
    -- Change tint based on property
    local col = GetProperty("Spin Color")
    SetTint(entity, col)
end
