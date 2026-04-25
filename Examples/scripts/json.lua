-- 1. Define a complex theme table (This can be modified dynamically!)
local myTheme = {
	name = "MidnightAmber",
	style = {
		rounding = { window = 12.0, frame = 8.0 },
		padding = { window = { 12.0, 12.0 }, frame = { 10.0, 8.0 } },
	},
	colors = {
		text = { 0.9, 0.9, 0.9, 1.0 },
		button = { 0.2, 0.2, 0.25, 1.0 },
	},
	font_path = "assets/fonts/Poppins-SemiBold.ttf",
}

-- 2. Use your new C++ API to serialize the table into a JSON string
local jsonString = json.encode(myTheme)
Log("Serialized Theme to JSON:")
Log(jsonString)

-- 3. Simulate saving to a file
local file = io.open("editor-configs/user-theme.json", "w")
if file then
	file:write(jsonString)
	file:close()
	LogSuccess("Theme saved to disk successfully!")
else
	LogError("Failed to open file for saving.")
end

-- 4. How to read it back (Demonstrating Decode)
local loadedData = json.decode(jsonString)
Log("Verified Theme Name: " .. loadedData["name"])
