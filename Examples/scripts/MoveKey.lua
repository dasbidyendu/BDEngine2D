-- assets/scripts/PlayerMove.lua
function OnUpdate(entity, dt)
    local transform = GetTransform(entity)
    
    if transform then
        local speed = 200 * dt
        
        -- Use Methods (Robust & clean)
        if IsKeyDown(KEY_D) or IsKeyDown(KEY_RIGHT) then
            print("LUA:Pressed KEY")
            transform:Translate(speed, 0)
        end
        if IsKeyDown(KEY_A) or IsKeyDown(KEY_LEFT) then
            print("LUA:Pressed KEY")
            transform:Translate(-speed, 0)
        end
        if IsKeyDown(KEY_W) or IsKeyDown(KEY_UP) then
            print("LUA:Pressed KEY")
            transform:Translate(0, -speed)
        end
        if IsKeyDown(KEY_S) or IsKeyDown(KEY_DOWN) then
            print("LUA:Pressed KEY")
            transform:Translate(0, speed)
        end

        -- Explicit setting
        transform:SetRotation(transform:GetRotation() + (90 * dt))
    end
end