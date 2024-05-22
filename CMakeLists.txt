cmake_minimum_required(VERSION 3.25)
project(opengl)

# opengl
set(INCLUDES ${INCLUDES} E:/mingw64/x86_64-w64-mingw32/include)
set(LIBS ${LIBS} C:/Windows/System32/opengl32.dll)

# glm
set(glm_DIR ${CMAKE_SOURCE_DIR}/3rdparty/glm-1.0.1/install)
set(INCLUDES ${INCLUDES} ${glm_DIR}/include)
set(LIBS ${LIBS} ${glm_DIR}/lib/libglm.a)

# glfw
set(GLFW_DIR ${CMAKE_SOURCE_DIR}/3rdparty/glfw-master/install)
set(INCLUDES ${INCLUDES} ${GLFW_DIR}/include)
set(LIBS ${LIBS} ${GLFW_DIR}/lib/libglfw3.a)

#glad
set(GLAD_DIR ${CMAKE_SOURCE_DIR}/3rdparty/glad)
set(INCLUDES ${INCLUDES} ${GLAD_DIR}/include)
add_library(GLAD STATIC ${GLAD_DIR}/src/glad.c)
set(LIBS ${LIBS} GLAD)

# # glew
# set(GLEW_DIR ${CMAKE_SOURCE_DIR}/3rdparty/glew-master/install)
# set(INCLUDES ${INCLUDES} ${GLEW_DIR}/include)
# set(LIBS ${LIBS} ${GLEW_DIR}/lib/Release/x64/glew32s.lib)

# # freeglut
# set(FreeGLUT_DIR ${CMAKE_SOURCE_DIR}/3rdparty/freeglut-master/install)
# set(INCLUDES ${INCLUDES} ${FreeGLUT_DIR}/include)
# set(LIBS ${LIBS} ${FreeGLUT_DIR}/lib/libfreeglut_static.a)

# stb_image
set(STD_LIB_PATH ${CMAKE_SOURCE_DIR}/3rdparty/stb-master)
set(INCLUDES ${INCLUDES} ${STD_LIB_PATH})

# assimp
set(ASSIMP_PATH ${CMAKE_SOURCE_DIR}/3rdparty/assimp/assimp_install)
set(INCLUDES ${INCLUDES} ${ASSIMP_PATH}/include)
set(LIBS ${LIBS} ${ASSIMP_PATH}/bin/libassimp-5.dll)  #bin/libassimp-5.dll lib/libzlibstatic.a

# util tool
file(GLOB_RECURSE UTILTOOL_SRC "${CMAKE_SOURCE_DIR}/src/util/*")
add_library(utilTool STATIC ${UTILTOOL_SRC})
set(LIBS utilTool ${LIBS})  #FIXME - 注意链接顺序

# include directory
include_directories(${INCLUDES} ${CMAKE_SOURCE_DIR}/src)
# link library
link_libraries(${LIBS})
message(STATUS "LIBS: ${LIBS}")

# create helloworld program
# add_executable(helloopengl ${CMAKE_SOURCE_DIR}/src/helloopengl.cpp)
# add_executable(template ${CMAKE_SOURCE_DIR}/src/template.cpp)

# set src file 
file(GLOB BEGINNERS__SRC "${CMAKE_SOURCE_DIR}/src/beginners/*.cpp")
file(GLOB LIGHTING__SRC "${CMAKE_SOURCE_DIR}/src/lighting/*.cpp")
file(GLOB MODEL_LOADING__SRC "${CMAKE_SOURCE_DIR}/src/model_loading/*.cpp")
file(GLOB ADVANCED_OPENGL_DRC "${CMAKE_SOURCE_DIR}/src/advancedOpenGL/*.cpp")
file(GLOB ADVANCED_LIGHTING_DRC "${CMAKE_SOURCE_DIR}/src/advancedLighting/*.cpp")
# create program
add_executable(chapter_0 ${CMAKE_SOURCE_DIR}/src/beginners/chapter_0.cpp)
add_executable(chapter_1 ${CMAKE_SOURCE_DIR}/src/lighting/chapter_1.cpp)
add_executable(chapter_2 ${CMAKE_SOURCE_DIR}/src/model_loading/mesh.cpp)
foreach(SRC_FILE ${ADVANCED_OPENGL_DRC})
    get_filename_component(SRC_NAME ${SRC_FILE} NAME_WE)
    add_executable(${SRC_NAME} ${SRC_FILE})
endforeach()
foreach(SRC_FILE ${ADVANCED_LIGHTING_DRC})
    get_filename_component(SRC_NAME ${SRC_FILE} NAME_WE)
    add_executable(${SRC_NAME} ${SRC_FILE})
endforeach()


# install
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install)
file(GLOB_RECURSE SHADER_SRC "${CMAKE_SOURCE_DIR}/src/*.glsl")
file(GLOB_RECURSE TEXTURE_SRC 
    "${CMAKE_SOURCE_DIR}/src/*.jpg"
    "${CMAKE_SOURCE_DIR}/src/*.png")
file(COPY ${SHADER_SRC} DESTINATION  ${CMAKE_SOURCE_DIR}/build/shader)
file(COPY ${TEXTURE_SRC} DESTINATION  ${CMAKE_SOURCE_DIR}/build/texture)
file(COPY ${ASSIMP_PATH}/bin/libassimp-5.dll DESTINATION ${CMAKE_SOURCE_DIR}/build)