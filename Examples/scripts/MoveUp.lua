function OnUpdate(entity, dt)
	local x, y = GetPos(entity)
	SetPos(entity, x , y+ (100 * dt))
	SetRotation(entity, 45)
	SetScale(entity,2)
end
