cmake_minimum_required(VERSION 3.4 FATAL_ERROR)
project(blue_sky)
# target
set(T ${PROJECT_NAME})
# sources
file(GLOB_RECURSE SOURCES *.cpp *.h *.hpp)
# python bindings
if(NOT PY EQUAL 1)
    list(FILTER SOURCES EXCLUDE REGEX ".*/python/.*")
endif()
# define target
add_library(${T} SHARED ${SOURCES})
# set output dir
set_outdir(${T} ${BS_OUTPUT_DIR})
# definitions
if(UNIX)
    target_compile_definitions(${T} PUBLIC -DUNIX)
endif()
target_compile_definitions(${T} PRIVATE -DBS_EXPORTING -DBS_EXPORTING_PLUGIN -DBS_CREATE_UNMANAGED_OBJECTS)
if(PY EQUAL 1)
    target_compile_definitions(${T} PRIVATE -DBSPY_EXPORTING -DBSPY_EXPORTING_PLUGIN)
endif()
# include dirs
target_include_directories(${T} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
# dependencies
if (UNIX)
    add_dependencies(${T} libcaf_core libcaf_io)
endif()
if(PY EQUAL 1)
    use_python(${T})
    use_pybind11(${T})
endif()
if(UNIX)
    target_link_libraries(${T} PRIVATE stdc++fs)
endif()
use_boost(${T} regex thread serialization locale)
use_caf(${T})
use_cereal(${T})
use_eigen(${T})
use_expected(${T})
use_spdlog(${T})
use_loki(${T})
use_shlwapi(${T})
use_fmt(${T})
