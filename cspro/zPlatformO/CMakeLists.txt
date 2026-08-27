project(zPlatformO)

add_library(${PROJECT_NAME}
    CSProStdioFile.cpp
    $<$<NOT:$<BOOL:${WIN32}>>:MobileStringConversion.cpp>
    PlatformInterface.cpp
    $<$<NOT:$<BOOL:${WIN32}>>:PortableFStream.cpp>
    util_snprintf.cpp
)
