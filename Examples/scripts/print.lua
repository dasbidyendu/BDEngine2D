-- print.lua
function OnUpdate(entity, dt)
    -- Your code here
				print("Hello from lua side")
    Log("This is a Log")
    LogWarn("Warning")
    LogError("Error")
    LogSuccess("Success")
end
