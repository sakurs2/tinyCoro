cmake_minimum_required(VERSION 3.15)
project(tinycoro
    VERSION 1.1
    LANGUAGES CXX
    DESCRIPTION "a high performance C++20 coroutine library combined with io_uring"
)

# check build dir
get_filename_component(BUILD_DIR_NAME ${CMAKE_BINARY_DIR} NAME)
string(TOLOWER "${BUILD_DIR_NAME}" BUILD_DIR_NAME_LOWER)
if(NOT BUILD_DIR_NAME_LOWER MATCHES "build")
    message(FATAL_ERROR 
        "The build directory must be named include 'build'.\n"
        "Current build directory: ${CMAKE_BINARY_DIR}\n"
        "Please create a build directory with the correct name and run CMake again."
    )
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# TODO: Complete api visiblity
# set(CMAKE_CXX_VISIBILITY_PRESET hidden) 
# set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

include(GNUInstallDirs)
include(GenerateExportHeader)
include(CMakeDependentOption)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})

set(SRC_INCLUDE_DIR 
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_BINARY_DIR}/include"
    "${PROJECT_SOURCE_DIR}/config")
set(THIRD_PARTY_INCLUDE_DIR
    "${PROJECT_SOURCE_DIR}/third_party/spdlog/include"
    "${PROJECT_SOURCE_DIR}/third_party/googletest/googletest/include"
    "${PROJECT_SOURCE_DIR}/third_party/atomic_queue/include"
    "${PROJECT_SOURCE_DIR}/third_party/benchmark/include")

option(ENABLE_UNIT_TESTS "Enable unit tests" ON)
option(ENABLE_DEBUG_MODE "Enable debug mode" OFF)
option(ENABLE_BUILD_SHARED_LIBS "Enable build shared libs" OFF)
cmake_dependent_option(ENABLE_COMPILE_OPTIMIZE "Enable compile options -O3" ON "NOT ENABLE_DEBUG_MODE" OFF)

set(BUILD_SHARED_LIBS ${ENABLE_BUILD_SHARED_LIBS} CACHE INTERNAL "")

if(NOT CMAKE_BUILD_TYPE)
    if(ENABLE_DEBUG_MODE)
        set(CMAKE_BUILD_TYPE Debug)
    else()
        set(CMAKE_BUILD_TYPE Release)
    endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(ENABLE_DEBUG_MODE ON)
    set(ENABLE_COMPILE_OPTIMIZE OFF)
endif()


# TODO: add more options

message(STATUS "${PROJECT_NAME} version: ${PROJECT_VERSION}")
message(STATUS "Complier path: ${CMAKE_CXX_COMPILER}")
message(STATUS "Enable testing: ${ENABLE_UNIT_TESTS}")
message(STATUS "Enable debug mode: ${ENABLE_DEBUG_MODE}")
message(STATUS "Enable build shared libs: ${ENABLE_BUILD_SHARED_LIBS}")
message(STATUS "Enable compile options -O3: ${ENABLE_COMPILE_OPTIMIZE}")

find_library(URING_PATH 
    NAMES uring
    PATHS /usr/lib
)

if (URING_PATH)
    message(STATUS "Found liburing at ${URING_PATH}")
else()
    message(FATAL_ERROR "Could not find liburing")
endif()

add_subdirectory(third_party)
add_subdirectory(src)
configure_file(${PROJECT_SOURCE_DIR}/config/config.h.in ${PROJECT_SOURCE_DIR}/config/config.h @ONLY)

add_library(${PROJECT_NAME} ${TINYCORO_SOURCE_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX PREFIX "" VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      target_compile_options(${PROJECT_NAME} PRIVATE "-g")
endif()
if(ENABLE_COMPILE_OPTIMIZE)
  target_compile_options(${PROJECT_NAME} PUBLIC -O3)
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:DEBUG>")
target_compile_definitions(${PROJECT_NAME} PRIVATE "$<$<CONFIG:RELEASE>:RELEASE>")
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_include_directories(${PROJECT_NAME} PUBLIC ${SRC_INCLUDE_DIR} ${THIRD_PARTY_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${URING_PATH})
generate_export_header(${PROJECT_NAME} BASE_NAME CORO EXPORT_FILE_NAME include/coro/export.hpp)

if(ENABLE_UNIT_TESTS)
  enable_testing()
endif()

add_subdirectory(tests)
add_subdirectory(benchtests)
add_subdirectory(examples)
add_subdirectory(benchmark)

configure_file(tinycoro.pc.in tinycoro.pc @ONLY)

# TODO: add install command
# install(TARGETS tinycoro)
# install(DIRECTORY include/coro TYPE INCLUDE)
# install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/coro TYPE INCLUDE)
# install(FILES ${CMAKE_CURRENT_BINARY_DIR}/tinycoro.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
