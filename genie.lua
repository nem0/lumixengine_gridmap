if plugin "myplugin" then
	files { 
		"src/**.c",
		"src/**.cpp",
		"src/**.h",
		"genie.lua"
	}
	defines { "BUILDING_MYPLUGIN" }
	dynamic_link_plugin { "engine" }
end