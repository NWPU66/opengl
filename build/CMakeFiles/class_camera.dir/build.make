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
include CMakeFiles/class_camera.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/class_camera.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/class_camera.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/class_camera.dir/flags.make

CMakeFiles/class_camera.dir/src/util/class_camera.cpp.obj: CMakeFiles/class_camera.dir/flags.make
CMakeFiles/class_camera.dir/src/util/class_camera.cpp.obj: CMakeFiles/class_camera.dir/includes_CXX.rsp
CMakeFiles/class_camera.dir/src/util/class_camera.cpp.obj: E:/Study/CodeProj/opengl/src/util/class_camera.cpp
CMakeFiles/class_camera.dir/src/util/class_camera.cpp.obj: CMakeFiles/class_camera.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=E:\Study\CodeProj\opengl\build\CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/class_camera.dir/src/util/class_camera.cpp.obj"
	E:\mingw64\bin\g++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/class_camera.dir/src/util/class_camera.cpp.obj -MF CMakeFiles\class_camera.dir\src\util\class_camera.cpp.obj.d -o CMakeFiles\class_camera.dir\src\util\class_camera.cpp.obj -c E:\Study\CodeProj\opengl\src\util\class_camera.cpp

CMakeFiles/class_camera.dir/src/util/class_camera.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/class_camera.dir/src/util/class_camera.cpp.i"
	E:\mingw64\bin\g++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E E:\Study\CodeProj\opengl\src\util\class_camera.cpp > CMakeFiles\class_camera.dir\src\util\class_camera.cpp.i

CMakeFiles/class_camera.dir/src/util/class_camera.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/class_camera.dir/src/util/class_camera.cpp.s"
	E:\mingw64\bin\g++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S E:\Study\CodeProj\opengl\src\util\class_camera.cpp -o CMakeFiles\class_camera.dir\src\util\class_camera.cpp.s

# Object files for target class_camera
class_camera_OBJECTS = \
"CMakeFiles/class_camera.dir/src/util/class_camera.cpp.obj"

# External object files for target class_camera
class_camera_EXTERNAL_OBJECTS =

libclass_camera.a: CMakeFiles/class_camera.dir/src/util/class_camera.cpp.obj
libclass_camera.a: CMakeFiles/class_camera.dir/build.make
libclass_camera.a: CMakeFiles/class_camera.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=E:\Study\CodeProj\opengl\build\CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libclass_camera.a"
	$(CMAKE_COMMAND) -P CMakeFiles\class_camera.dir\cmake_clean_target.cmake
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles\class_camera.dir\link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/class_camera.dir/build: libclass_camera.a
.PHONY : CMakeFiles/class_camera.dir/build

CMakeFiles/class_camera.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles\class_camera.dir\cmake_clean.cmake
.PHONY : CMakeFiles/class_camera.dir/clean

CMakeFiles/class_camera.dir/depend:
	$(CMAKE_COMMAND) -E cmake_depends "MinGW Makefiles" E:\Study\CodeProj\opengl E:\Study\CodeProj\opengl E:\Study\CodeProj\opengl\build E:\Study\CodeProj\opengl\build E:\Study\CodeProj\opengl\build\CMakeFiles\class_camera.dir\DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/class_camera.dir/depend

