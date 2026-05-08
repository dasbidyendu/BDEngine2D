local echoPulse = {
    active = false,
    radius = 0,
    maxRadius = 600,
    speed = 400,
    originX = 0,
    originY = 0
}

function OnUpdate(self, dt)
    -- 1. FIX: Convert Mouse Screen Pos to World Pos
    if IsMouseButtonPressed(0) and not echoPulse.active then
        local mouseScreen = GetMousePos()
        local mouseWorld = ScreenToWorld(mouseScreen.x, mouseScreen.y)
        
        echoPulse.active = true
        echoPulse.radius = 0
        
        -- Trigger at click location
        echoPulse.originX = mouseWorld.x
        echoPulse.originY = mouseWorld.y
        
        -- OPTIONAL: If you want the pulse to always come from the player, use:
        -- echoPulse.originX, echoPulse.originY = GetPos(self)
    end

    if echoPulse.active then
        echoPulse.radius = echoPulse.radius + (echoPulse.speed * dt)
        
        -- Draw the visual ring at the world-corrected origin
        DrawCircleWorld(echoPulse.originX, echoPulse.originY, echoPulse.radius, MakeColor(0, 255, 255, 200))

        -- Reveal logic using world coordinates
        local found = GetEntities(echoPulse.originX, echoPulse.originY, echoPulse.radius)
        for i, ent in pairs(found) do
            if ent ~= self then
                SetTint(ent, MakeColor(255, 255, 255, 255))
            end
        end

        if echoPulse.radius > echoPulse.maxRadius then
            echoPulse.active = false
        end
    end

    -- Fading Logic
    local all = GetEntities() 
    for i, ent in pairs(all) do
        local c = GetTint(ent)
        if c and ent ~= self then
            if c.a > 0 then
                local newAlpha = math.max(0, c.a - (150 * dt))
                SetTint(ent, MakeColor(c.r, c.g, c.b, newAlpha))
            end
        end
    end
end