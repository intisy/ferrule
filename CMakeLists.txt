cmake_minimum_required(VERSION 3.20)
project(ferrule C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(MSVC)
    set(FERRULE_WARNING_FLAGS /W4 /WX /D_CRT_SECURE_NO_WARNINGS)
    # greatest.h's assertion macros expand literal-constant comparisons into an
    # `if`, which MSVC's C4127 flags even though the do/while(0) wrapper is fine.
    set(FERRULE_TEST_WARNING_FLAGS ${FERRULE_WARNING_FLAGS} /wd4127)
else()
    set(FERRULE_WARNING_FLAGS -Wall -Wextra -Werror)
    set(FERRULE_TEST_WARNING_FLAGS ${FERRULE_WARNING_FLAGS})
endif()

add_library(ferrule_vendor STATIC vendor/cJSON/cJSON.c)
target_include_directories(ferrule_vendor PUBLIC vendor/cJSON)

file(GLOB FERRULE_CORE_SOURCES CONFIGURE_DEPENDS src/*.c)
list(REMOVE_ITEM FERRULE_CORE_SOURCES
     ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
     ${CMAKE_CURRENT_SOURCE_DIR}/src/http_winhttp.c
     ${CMAKE_CURRENT_SOURCE_DIR}/src/http_curl.c)

if(WIN32)
    list(APPEND FERRULE_CORE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/http_winhttp.c)
else()
    list(APPEND FERRULE_CORE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/http_curl.c)
endif()

add_library(ferrule_core STATIC ${FERRULE_CORE_SOURCES})
target_compile_options(ferrule_core PRIVATE ${FERRULE_WARNING_FLAGS})
target_include_directories(ferrule_core PUBLIC src vendor/cJSON)
target_link_libraries(ferrule_core PUBLIC ferrule_vendor)

if(WIN32)
    target_link_libraries(ferrule_core PUBLIC winhttp)
else()
    find_package(CURL REQUIRED)
    target_link_libraries(ferrule_core PUBLIC CURL::libcurl)
endif()

add_executable(ferrule src/main.c)
target_link_libraries(ferrule PRIVATE ferrule_core)
target_compile_options(ferrule PRIVATE ${FERRULE_WARNING_FLAGS})

enable_testing()
add_library(ferrule_test_support STATIC test/support.c)
target_include_directories(ferrule_test_support PUBLIC test)
target_compile_options(ferrule_test_support PRIVATE ${FERRULE_WARNING_FLAGS})

file(GLOB FERRULE_TESTS CONFIGURE_DEPENDS test/test_*.c)
foreach(test_file ${FERRULE_TESTS})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(${test_name} ${test_file})
    target_include_directories(${test_name} PRIVATE vendor/greatest test)
    target_link_libraries(${test_name} PRIVATE ferrule_core ferrule_test_support)
    target_compile_options(${test_name} PRIVATE ${FERRULE_TEST_WARNING_FLAGS})
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()

add_test(NAME agnostic_core COMMAND ${CMAKE_COMMAND}
         -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
         -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/check_agnostic.cmake)
