file(GLOB SOURCES "*.cpp")

# 输出找到的文件名
foreach(SOURCE ${SOURCES})
    get_filename_component(FILENAME "${SOURCE}" NAME)
    get_filename_component(TARNAME "${SOURCE}" NAME_WE)
    add_executable(${TARNAME} ${FILENAME})
    target_link_libraries(${TARNAME} ${PROJECT_NAME})
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      target_compile_options(${TARNAME} PRIVATE "-g")
    endif()
endforeach()