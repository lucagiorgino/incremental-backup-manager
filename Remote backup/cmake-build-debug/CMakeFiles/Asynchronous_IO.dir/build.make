# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.17

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

# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /snap/clion/123/bin/cmake/linux/bin/cmake

# The command to remove a file.
RM = /snap/clion/123/bin/cmake/linux/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = "/home/cosimo/Desktop/ProgettoPDS/Remote backup"

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = "/home/cosimo/Desktop/ProgettoPDS/Remote backup/cmake-build-debug"

# Include any dependencies generated for this target.
include CMakeFiles/Asynchronous_IO.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/Asynchronous_IO.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/Asynchronous_IO.dir/flags.make

CMakeFiles/Asynchronous_IO.dir/main.cpp.o: CMakeFiles/Asynchronous_IO.dir/flags.make
CMakeFiles/Asynchronous_IO.dir/main.cpp.o: ../main.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/home/cosimo/Desktop/ProgettoPDS/Remote backup/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/Asynchronous_IO.dir/main.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Asynchronous_IO.dir/main.cpp.o -c "/home/cosimo/Desktop/ProgettoPDS/Remote backup/main.cpp"

CMakeFiles/Asynchronous_IO.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Asynchronous_IO.dir/main.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E "/home/cosimo/Desktop/ProgettoPDS/Remote backup/main.cpp" > CMakeFiles/Asynchronous_IO.dir/main.cpp.i

CMakeFiles/Asynchronous_IO.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Asynchronous_IO.dir/main.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S "/home/cosimo/Desktop/ProgettoPDS/Remote backup/main.cpp" -o CMakeFiles/Asynchronous_IO.dir/main.cpp.s

CMakeFiles/Asynchronous_IO.dir/client/FileWatcher.cpp.o: CMakeFiles/Asynchronous_IO.dir/flags.make
CMakeFiles/Asynchronous_IO.dir/client/FileWatcher.cpp.o: ../client/FileWatcher.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/home/cosimo/Desktop/ProgettoPDS/Remote backup/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/Asynchronous_IO.dir/client/FileWatcher.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Asynchronous_IO.dir/client/FileWatcher.cpp.o -c "/home/cosimo/Desktop/ProgettoPDS/Remote backup/client/FileWatcher.cpp"

CMakeFiles/Asynchronous_IO.dir/client/FileWatcher.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Asynchronous_IO.dir/client/FileWatcher.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E "/home/cosimo/Desktop/ProgettoPDS/Remote backup/client/FileWatcher.cpp" > CMakeFiles/Asynchronous_IO.dir/client/FileWatcher.cpp.i

CMakeFiles/Asynchronous_IO.dir/client/FileWatcher.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Asynchronous_IO.dir/client/FileWatcher.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S "/home/cosimo/Desktop/ProgettoPDS/Remote backup/client/FileWatcher.cpp" -o CMakeFiles/Asynchronous_IO.dir/client/FileWatcher.cpp.s

CMakeFiles/Asynchronous_IO.dir/client/Client.cpp.o: CMakeFiles/Asynchronous_IO.dir/flags.make
CMakeFiles/Asynchronous_IO.dir/client/Client.cpp.o: ../client/Client.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/home/cosimo/Desktop/ProgettoPDS/Remote backup/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/Asynchronous_IO.dir/client/Client.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Asynchronous_IO.dir/client/Client.cpp.o -c "/home/cosimo/Desktop/ProgettoPDS/Remote backup/client/Client.cpp"

CMakeFiles/Asynchronous_IO.dir/client/Client.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Asynchronous_IO.dir/client/Client.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E "/home/cosimo/Desktop/ProgettoPDS/Remote backup/client/Client.cpp" > CMakeFiles/Asynchronous_IO.dir/client/Client.cpp.i

CMakeFiles/Asynchronous_IO.dir/client/Client.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Asynchronous_IO.dir/client/Client.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S "/home/cosimo/Desktop/ProgettoPDS/Remote backup/client/Client.cpp" -o CMakeFiles/Asynchronous_IO.dir/client/Client.cpp.s

# Object files for target Asynchronous_IO
Asynchronous_IO_OBJECTS = \
"CMakeFiles/Asynchronous_IO.dir/main.cpp.o" \
"CMakeFiles/Asynchronous_IO.dir/client/FileWatcher.cpp.o" \
"CMakeFiles/Asynchronous_IO.dir/client/Client.cpp.o"

# External object files for target Asynchronous_IO
Asynchronous_IO_EXTERNAL_OBJECTS =

Asynchronous_IO: CMakeFiles/Asynchronous_IO.dir/main.cpp.o
Asynchronous_IO: CMakeFiles/Asynchronous_IO.dir/client/FileWatcher.cpp.o
Asynchronous_IO: CMakeFiles/Asynchronous_IO.dir/client/Client.cpp.o
Asynchronous_IO: CMakeFiles/Asynchronous_IO.dir/build.make
Asynchronous_IO: CMakeFiles/Asynchronous_IO.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir="/home/cosimo/Desktop/ProgettoPDS/Remote backup/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_4) "Linking CXX executable Asynchronous_IO"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/Asynchronous_IO.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/Asynchronous_IO.dir/build: Asynchronous_IO

.PHONY : CMakeFiles/Asynchronous_IO.dir/build

CMakeFiles/Asynchronous_IO.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/Asynchronous_IO.dir/cmake_clean.cmake
.PHONY : CMakeFiles/Asynchronous_IO.dir/clean

CMakeFiles/Asynchronous_IO.dir/depend:
	cd "/home/cosimo/Desktop/ProgettoPDS/Remote backup/cmake-build-debug" && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" "/home/cosimo/Desktop/ProgettoPDS/Remote backup" "/home/cosimo/Desktop/ProgettoPDS/Remote backup" "/home/cosimo/Desktop/ProgettoPDS/Remote backup/cmake-build-debug" "/home/cosimo/Desktop/ProgettoPDS/Remote backup/cmake-build-debug" "/home/cosimo/Desktop/ProgettoPDS/Remote backup/cmake-build-debug/CMakeFiles/Asynchronous_IO.dir/DependInfo.cmake" --color=$(COLOR)
.PHONY : CMakeFiles/Asynchronous_IO.dir/depend
