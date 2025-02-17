cmake_minimum_required(VERSION 3.0)

project("Zero Standard Library" VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(BUILD_SHARED_LIBS "Compile zsl as a shared/dynamic library." OFF)
option(FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." ON)
option(ZSL_BUILD_TESTS "Build the tests executable." OFF)
#option(ZSL_BUILD_BENCHMARKS "Build the benchmarks executable." OFF)
#option(ZSL_BUILD_DOCS "Build the documention." OFF)

#-fsanitize-undefined

if(${FORCE_COLORED_OUTPUT})
	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		add_compile_options(-fdiagnostics-color=always)
	elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		add_compile_options(-fcolor-diagnostics)
	endif()
endif()

add_library(zsl src/zsl.cpp)
target_include_directories(zsl PUBLIC include/)
set_target_properties(zsl PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
if(("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang") AND CMAKE_SIZEOF_VOID_P EQUAL 8)
	# This is a linux machine.
	target_link_libraries(zsl PUBLIC -pthread)
	target_compile_options(zsl PUBLIC -mcx16)
	#target_compile_options(zsl PUBLIC -pedantic-errors)
endif()

if(${ZSL_BUILD_TESTS})
	add_subdirectory(tests)
endif()
