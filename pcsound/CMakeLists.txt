add_library(cpp_pcsound STATIC
            pcsound.cpp       pcsound.hpp
            pcsound_bsd.cpp
            pcsound_sdl.cpp
            pcsound_linux.cpp
            pcsound_win32.cpp
                            pcsound_internal.hpp)
target_include_directories(cpp_pcsound
                           INTERFACE "."
                           PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/../")
target_link_libraries(cpp_pcsound SDL2::mixer)
