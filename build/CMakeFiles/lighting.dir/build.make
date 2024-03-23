# CMAKE generated file: DO NOT EDIT!
# Generated by "MinGW Makefiles" Generator, CMake Version 3.28

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

SHELL = cmd.exe

# The CMake executable.
CMAKE_COMMAND = D:\Cmake\bin\cmake.exe

# The command to remove a file.
RM = D:\Cmake\bin\cmake.exe -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = E:\Study\CodeProj\opengl

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = E:\Study\CodeProj\opengl\build

# Include any dependencies generated for this target.
include CMakeFiles/lighting.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/lighting.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/lighting.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/lighting.dir/flags.make

CMakeFiles/lighting.dir/src/lighting/lighting.cpp.obj: CMakeFiles/lighting.dir/flags.make
CMakeFiles/lighting.dir/src/lighting/lighting.cpp.obj: CMakeFiles/lighting.dir/includes_CXX.rsp
CMakeFiles/lighting.dir/src/lighting/lighting.cpp.obj: E:/Study/CodeProj/opengl/src/lighting/lighting.cpp
CMakeFiles/lighting.dir/src/lighting/lighting.cpp.obj: CMakeFiles/lighting.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=E:\Study\CodeProj\opengl\build\CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/lighting.dir/src/lighting/lighting.cpp.obj"
	E:\mingw64\bin\g++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/lighting.dir/src/lighting/lighting.cpp.obj -MF CMakeFiles\lighting.dir\src\lighting\lighting.cpp.obj.d -o CMakeFiles\lighting.dir\src\lighting\lighting.cpp.obj -c E:\Study\CodeProj\opengl\src\lighting\lighting.cpp

CMakeFiles/lighting.dir/src/lighting/lighting.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/lighting.dir/src/lighting/lighting.cpp.i"
	E:\mingw64\bin\g++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E E:\Study\CodeProj\opengl\src\lighting\lighting.cpp > CMakeFiles\lighting.dir\src\lighting\lighting.cpp.i

CMakeFiles/lighting.dir/src/lighting/lighting.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/lighting.dir/src/lighting/lighting.cpp.s"
	E:\mingw64\bin\g++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S E:\Study\CodeProj\opengl\src\lighting\lighting.cpp -o CMakeFiles\lighting.dir\src\lighting\lighting.cpp.s

# Object files for target lighting
lighting_OBJECTS = \
"CMakeFiles/lighting.dir/src/lighting/lighting.cpp.obj"

# External object files for target lighting
lighting_EXTERNAL_OBJECTS =

lighting.exe: CMakeFiles/lighting.dir/src/lighting/lighting.cpp.obj
lighting.exe: CMakeFiles/lighting.dir/build.make
lighting.exe: C:/Windows/System32/opengl32.dll
lighting.exe: E:/Study/CodeProj/opengl/3rdparty/glm-1.0.1/install/lib/libglm.a
lighting.exe: E:/Study/CodeProj/opengl/3rdparty/glfw-master/install/lib/libglfw3.a
lighting.exe: libGLAD.a
lighting.exe: libclass_shader.a
lighting.exe: libclass_camera.a
lighting.exe: libstructure.a
lighting.exe: CMakeFiles/lighting.dir/linkLibs.rsp
lighting.exe: CMakeFiles/lighting.dir/objects1.rsp
lighting.exe: CMakeFiles/lighting.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=E:\Study\CodeProj\opengl\build\CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable lighting.exe"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles\lighting.dir\link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/lighting.dir/build: lighting.exe
.PHONY : CMakeFiles/lighting.dir/build

CMakeFiles/lighting.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles\lighting.dir\cmake_clean.cmake
.PHONY : CMakeFiles/lighting.dir/clean

CMakeFiles/lighting.dir/depend:
	$(CMAKE_COMMAND) -E cmake_depends "MinGW Makefiles" E:\Study\CodeProj\opengl E:\Study\CodeProj\opengl E:\Study\CodeProj\opengl\build E:\Study\CodeProj\opengl\build E:\Study\CodeProj\opengl\build\CMakeFiles\lighting.dir\DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/lighting.dir/depend

