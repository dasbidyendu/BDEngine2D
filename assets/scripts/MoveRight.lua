-- assets/scripts/MoveRight.lua
function OnUpdate(entity, dt)
	local x, y = GetPos(entity)
	SetPos(entity, x + (100 * dt), y)

	-- No headers, no C++, just simple functions
	SetRotation(entity, 45)
end
