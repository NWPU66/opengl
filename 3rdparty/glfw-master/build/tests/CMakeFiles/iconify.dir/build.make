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
CMAKE_SOURCE_DIR = E:\Study\CodeProj\opengl\3rdparty\glfw-master

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = E:\Study\CodeProj\opengl\3rdparty\glfw-master\build

# Include any dependencies generated for this target.
include tests/CMakeFiles/iconify.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include tests/CMakeFiles/iconify.dir/compiler_depend.make

# Include the progress variables for this target.
include tests/CMakeFiles/iconify.dir/progress.make

# Include the compile flags for this target's objects.
include tests/CMakeFiles/iconify.dir/flags.make

tests/CMakeFiles/iconify.dir/iconify.c.obj: tests/CMakeFiles/iconify.dir/flags.make
tests/CMakeFiles/iconify.dir/iconify.c.obj: tests/CMakeFiles/iconify.dir/includes_C.rsp
tests/CMakeFiles/iconify.dir/iconify.c.obj: E:/Study/CodeProj/opengl/3rdparty/glfw-master/tests/iconify.c
tests/CMakeFiles/iconify.dir/iconify.c.obj: tests/CMakeFiles/iconify.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=E:\Study\CodeProj\opengl\3rdparty\glfw-master\build\CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object tests/CMakeFiles/iconify.dir/iconify.c.obj"
	cd /d E:\Study\CodeProj\opengl\3rdparty\glfw-master\build\tests && E:\mingw64\bin\gcc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT tests/CMakeFiles/iconify.dir/iconify.c.obj -MF CMakeFiles\iconify.dir\iconify.c.obj.d -o CMakeFiles\iconify.dir\iconify.c.obj -c E:\Study\CodeProj\opengl\3rdparty\glfw-master\tests\iconify.c

tests/CMakeFiles/iconify.dir/iconify.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing C source to CMakeFiles/iconify.dir/iconify.c.i"
	cd /d E:\Study\CodeProj\opengl\3rdparty\glfw-master\build\tests && E:\mingw64\bin\gcc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E E:\Study\CodeProj\opengl\3rdparty\glfw-master\tests\iconify.c > CMakeFiles\iconify.dir\iconify.c.i

tests/CMakeFiles/iconify.dir/iconify.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling C source to assembly CMakeFiles/iconify.dir/iconify.c.s"
	cd /d E:\Study\CodeProj\opengl\3rdparty\glfw-master\build\tests && E:\mingw64\bin\gcc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S E:\Study\CodeProj\opengl\3rdparty\glfw-master\tests\iconify.c -o CMakeFiles\iconify.dir\iconify.c.s

tests/CMakeFiles/iconify.dir/__/deps/getopt.c.obj: tests/CMakeFiles/iconify.dir/flags.make
tests/CMakeFiles/iconify.dir/__/deps/getopt.c.obj: tests/CMakeFiles/iconify.dir/includes_C.rsp
tests/CMakeFiles/iconify.dir/__/deps/getopt.c.obj: E:/Study/CodeProj/opengl/3rdparty/glfw-master/deps/getopt.c
tests/CMakeFiles/iconify.dir/__/deps/getopt.c.obj: tests/CMakeFiles/iconify.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=E:\Study\CodeProj\opengl\3rdparty\glfw-master\build\CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object tests/CMakeFiles/iconify.dir/__/deps/getopt.c.obj"
	cd /d E:\Study\CodeProj\opengl\3rdparty\glfw-master\build\tests && E:\mingw64\bin\gcc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT tests/CMakeFiles/iconify.dir/__/deps/getopt.c.obj -MF CMakeFiles\iconify.dir\__\deps\getopt.c.obj.d -o CMakeFiles\iconify.dir\__\deps\getopt.c.obj -c E:\Study\CodeProj\opengl\3rdparty\glfw-master\deps\getopt.c

tests/CMakeFiles/iconify.dir/__/deps/getopt.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing C source to CMakeFiles/iconify.dir/__/deps/getopt.c.i"
	cd /d E:\Study\CodeProj\opengl\3rdparty\glfw-master\build\tests && E:\mingw64\bin\gcc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E E:\Study\CodeProj\opengl\3rdparty\glfw-master\deps\getopt.c > CMakeFiles\iconify.dir\__\deps\getopt.c.i

tests/CMakeFiles/iconify.dir/__/deps/getopt.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling C source to assembly CMakeFiles/iconify.dir/__/deps/getopt.c.s"
	cd /d E:\Study\CodeProj\opengl\3rdparty\glfw-master\build\tests && E:\mingw64\bin\gcc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S E:\Study\CodeProj\opengl\3rdparty\glfw-master\deps\getopt.c -o CMakeFiles\iconify.dir\__\deps\getopt.c.s

# Object files for target iconify
iconify_OBJECTS = \
"CMakeFiles/iconify.dir/iconify.c.obj" \
"CMakeFiles/iconify.dir/__/deps/getopt.c.obj"

# External object files for target iconify
iconify_EXTERNAL_OBJECTS =

tests/iconify.exe: tests/CMakeFiles/iconify.dir/iconify.c.obj
tests/iconify.exe: tests/CMakeFiles/iconify.dir/__/deps/getopt.c.obj
tests/iconify.exe: tests/CMakeFiles/iconify.dir/build.make
tests/iconify.exe: src/libglfw3.a
tests/iconify.exe: tests/CMakeFiles/iconify.dir/linkLibs.rsp
tests/iconify.exe: tests/CMakeFiles/iconify.dir/objects1.rsp
tests/iconify.exe: tests/CMakeFiles/iconify.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=E:\Study\CodeProj\opengl\3rdparty\glfw-master\build\CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking C executable iconify.exe"
	cd /d E:\Study\CodeProj\opengl\3rdparty\glfw-master\build\tests && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles\iconify.dir\link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
tests/CMakeFiles/iconify.dir/build: tests/iconify.exe
.PHONY : tests/CMakeFiles/iconify.dir/build

tests/CMakeFiles/iconify.dir/clean:
	cd /d E:\Study\CodeProj\opengl\3rdparty\glfw-master\build\tests && $(CMAKE_COMMAND) -P CMakeFiles\iconify.dir\cmake_clean.cmake
.PHONY : tests/CMakeFiles/iconify.dir/clean

tests/CMakeFiles/iconify.dir/depend:
	$(CMAKE_COMMAND) -E cmake_depends "MinGW Makefiles" E:\Study\CodeProj\opengl\3rdparty\glfw-master E:\Study\CodeProj\opengl\3rdparty\glfw-master\tests E:\Study\CodeProj\opengl\3rdparty\glfw-master\build E:\Study\CodeProj\opengl\3rdparty\glfw-master\build\tests E:\Study\CodeProj\opengl\3rdparty\glfw-master\build\tests\CMakeFiles\iconify.dir\DependInfo.cmake "--color=$(COLOR)"
.PHONY : tests/CMakeFiles/iconify.dir/depend

