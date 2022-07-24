 cmake_minimum_required(VERSION 3.15)

 project(RULE_ENGINE_CLIPS)

 set(CMAKE_CXX_STANDARD 14)

 aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/executables EXECUTABLE_DIR)
 aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/lib LIB_DIR)
 aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/clips CLIPS_DIR)
 message("LIB: ${LIB_DIR}")
 message("CLIPS: ${CLIPS_DIR}")
 include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

 file(GLOB RULE_ENGINE_CLIPS_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/lib/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/lib/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/clips/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/clips/*.hpp")
 message("System DIR: ${RULE_ENGINE_CLIPS_HEADERS}")

 add_executable(clips-test ${EXECUTABLE_DIR} ${LIB_DIR} ${CLIPS_DIR})