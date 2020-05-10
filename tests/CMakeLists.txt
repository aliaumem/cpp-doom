set(TEST_NAME DoomApprovalTests)
set(CMAKE_CXX_STANDARD 20)
set(SOURCE_FILES
        main.cpp)

add_executable("cpp_${TEST_NAME}" ${SOURCE_FILES})
target_link_libraries("cpp_${TEST_NAME}" cpp_doom ApprovalTests::ApprovalTests Catch2::Catch2)
add_test(NAME "cpp_${TEST_NAME}" COMMAND "cpp_${TEST_NAME}")

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