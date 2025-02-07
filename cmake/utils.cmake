
# Remember x86/x64
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    SET( EX_PLATFORM 64)
    SET( EX_PLATFORM_NAME "x64")
else (CMAKE_SIZEOF_VOID_P EQUAL 8)
    SET( EX_PLATFORM 32)
    SET( EX_PLATFORM_NAME "x86")
endif (CMAKE_SIZEOF_VOID_P EQUAL 8)

set (CMAKE_DEBUG_POSTFIX "_Debug")
set (CMAKE_RELEASE_POSTFIX "_Release")
set (CMAKE_MINSIZEREL_POSTFIX "_MinSizeRel")
set (CMAKE_RELWITHDEBINFO_POSTFIX "_RelWithDebInfo")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin)

if (MSVC)
	SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D \"_CRT_SECURE_NO_WARNINGS\" /MP /bigobj")
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"))
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif()

if(MINGW OR CYGWIN)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wa,-mbig-obj")
endif()

# From http://stackoverflow.com/questions/148570/using-pre-compiled-headers-with-cmake
# Given a variable MySources with all the source files, use as follow:
# ADD_MSVC_PRECOMPILED_HEADER("precompiled.h" "precompiled.cpp" MySources)
MACRO(ADD_MSVC_PRECOMPILED_HEADER PrecompiledHeader PrecompiledSource SourcesVar)
  IF(MSVC)
    GET_FILENAME_COMPONENT(PrecompiledBasename ${PrecompiledHeader} NAME_WE)
    #SET(PrecompiledBinary "${CMAKE_CURRENT_BINARY_DIR}/${PrecompiledBasename}.pch")
    SET(PrecompiledBinary "$(IntDir)/${PrecompiledBasename}.pch")
    SET(Sources ${${SourcesVar}})

    SET_SOURCE_FILES_PROPERTIES(${PrecompiledSource}
                                PROPERTIES COMPILE_FLAGS "/Yc\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
                                           OBJECT_OUTPUTS "${PrecompiledBinary}")
    SET_SOURCE_FILES_PROPERTIES(${Sources}
                                PROPERTIES COMPILE_FLAGS "/Yu\"${PrecompiledHeader}\" /FI\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
                                           OBJECT_DEPENDS "${PrecompiledBinary}")  
    # Add precompiled header to SourcesVar
    LIST(APPEND ${SourcesVar} ${PrecompiledHeader} ${PrecompiledSource})
  ENDIF(MSVC)
ENDMACRO(ADD_MSVC_PRECOMPILED_HEADER)

function(cz_set_postfix)
	message(${PROJECT_NAME})
	set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX _${EX_PLATFORM_NAME}${CMAKE_DEBUG_POSTFIX})
	set_target_properties(${PROJECT_NAME} PROPERTIES RELEASE_POSTFIX _${EX_PLATFORM_NAME}${CMAKE_RELEASE_POSTFIX})
	set_target_properties(${PROJECT_NAME} PROPERTIES MINSIZEREL_POSTFIX _${EX_PLATFORM_NAME}${CMAKE_MINSIZEREL_POSTFIX})
	set_target_properties(${PROJECT_NAME} PROPERTIES RELWITHDEBINFO_POSTFIX _${EX_PLATFORM_NAME}${CMAKE_RELWITHDEBINFO_POSTFIX})
endfunction()

function(cz_add_common_libs)
  if (MSVC)
    target_link_libraries(${PROJECT_NAME} ws2_32 Shlwapi.lib)
	elseif (MINGW)
		target_link_libraries(${PROJECT_NAME} ws2_32 mswsock )
		#SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lws2_32")
	elseif(CMAKE_COMPILER_IS_GNUCXX OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"))
		target_link_libraries(${PROJECT_NAME} pthread )
	endif()
endfunction()


