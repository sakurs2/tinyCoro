file(GLOB API_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(
  tinycoro_net
  OBJECT
  ${API_SOURCE_FILES}
)

set(ALL_OBJECT_FILES
  ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:tinycoro_net>
  PARENT_SCOPE)