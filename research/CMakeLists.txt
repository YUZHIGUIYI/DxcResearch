file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_LIST_DIR}/src/*.h
        ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)

add_executable(DXCResearch ${SRC_FILES})

target_include_directories(DXCResearch PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)
target_include_directories(DXCResearch PRIVATE ${PROJECT_SOURCE_DIR}/dxc)
set_target_properties(DXCResearch PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS ON)