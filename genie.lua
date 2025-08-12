if plugin "gridmap" then
	files { 
		"src/**.c",
		"src/**.cpp",
		"src/**.h",
		"genie.lua"
	}
	defines { "BUILDING_GRIDMAP" }
	dynamic_link_plugin { "engine" }
end