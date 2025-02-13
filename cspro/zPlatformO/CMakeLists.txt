project(zPlatformO)

include(../WASM/Definitions.cmake)

add_library(${PROJECT_NAME} 
	CSProStdioFile.cpp
	MobileStringConversion.cpp
	PlatformInterface.cpp
	PortableFStream.cpp
	util_snprintf.cpp
)
