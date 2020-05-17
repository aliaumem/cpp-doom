set(TEST_NAME DoomApprovalTests)
set(CMAKE_CXX_STANDARD 20)

set(SOURCE_FILES
        main.cpp
        run_main.cpp
        mock_main.cpp
        new_main.cpp
        zallocator_test.cpp
)

add_executable("cpp_${TEST_NAME}" ${SOURCE_FILES})
target_link_libraries("cpp_${TEST_NAME}" -Wl,--start-group libcpp-doom cpp_deh cpp_doom -Wl,--end-group ApprovalTests::ApprovalTests Catch2::Catch2)
target_compile_definitions("cpp_${TEST_NAME}" PRIVATE CPP_DOOM)
add_test(NAME "cpp_${TEST_NAME}" COMMAND "cpp_${TEST_NAME}")

add_executable("crispy_${TEST_NAME}" ${SOURCE_FILES})
target_link_libraries("crispy_${TEST_NAME}" -Wl,--start-group libcrispy-doom crispy_deh doom -Wl,--end-group ApprovalTests::ApprovalTests Catch2::Catch2)
target_compile_definitions("crispy_${TEST_NAME}" PRIVATE CRISPY_DOOM)
add_test(NAME "crispy_${TEST_NAME}" COMMAND "crispy_${TEST_NAME}")

function(SET_WARNINGS PROJ_NAME)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_SIMULATE_ID MATCHES "MSVC")
        target_compile_options(${PROJ_NAME} PRIVATE
                /W4
                /WX
                )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${PROJ_NAME} PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -Werror
                )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        string(REGEX REPLACE " /W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        target_compile_options(${PROJ_NAME} PRIVATE
                /W4
                /WX
                )
    endif()
endfunction()


SET_WARNINGS("cpp_${TEST_NAME}")