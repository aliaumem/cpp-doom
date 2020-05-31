add_library(testing_tools
    i_system_ext.cpp
    lump_ext.h
    lump_ext.cpp
    lump_testing.hpp
    z_zone_ext.h
    z_zone_ext.cpp
        zone_testing.hpp)

target_include_directories(testing_tools PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})