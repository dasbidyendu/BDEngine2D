function OnUpdate(entity, dt)
    local transform = GetTransform(entity)

    if IsKeyDown(262) then -- Right Arrow
        transform.x = transform.x + (100 * dt)
    end

    transform.rotation = transform.rotation + (90 * dt)
    
    print("Entity Pos: " .. transform.pos.x .. ", " .. transform.pos.y)
end