cmake_minimum_required(VERSION 3.5)
project(vim-vs)

include(cmake/ucm.cmake)
include(cmake/utils.cmake)

set_property( GLOBAL PROPERTY USE_FOLDERS ON )

# Remove the MinSizeRel and RelWithDebInfo configs
# https://cmake.org/Wiki/CMake_FAQ#How_can_I_specify_my_own_configurations_.28for_generators_that_allow_it.29_.3F
if(CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CONFIGURATION_TYPES Debug Release)
	set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING
		"Reset the configurations to what we need"
 		FORCE)
endif()

# Tweak some more things
if(MSVC)
	add_compile_options(/MP /DNOMINMAX)
	# Add debug information to Release (By default, the "Release" configuration cmake creates has no debug info)
	ucm_add_flags(/Zi CONFIG Release)
	if (MSVC12)
		ucm_add_linker_flags(EXE MODULE /DEBUG CONFIG Release)
	elseif(MSVC14)
		ucm_add_linker_flags(EXE MODULE /DEBUG:Fastlink CONFIG Release)
	endif()
endif()

# Will cause building to display what compiler parameters are being used, among other things
SET(CMAKE_VERBOSE_MAKEFILE_ON)

add_subdirectory(source)
add_subdirectory(dummy)

