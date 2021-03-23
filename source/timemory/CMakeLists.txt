
set_property(GLOBAL APPEND PROPERTY
    BUILDTREE_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR})

get_property(_CMAKE_DEFINES GLOBAL PROPERTY ${PROJECT_NAME}_CMAKE_DEFINES)
list(SORT _CMAKE_DEFINES)

set(${PROJECT_NAME}_CMAKE_DEFINES)
foreach(_DEF ${_CMAKE_DEFINES})
    set(${PROJECT_NAME}_CMAKE_DEFINES "${${PROJECT_NAME}_CMAKE_DEFINES}\n#cmakedefine ${_DEF}")
endforeach()
set(${PROJECT_NAME}_CMAKE_DEFINES "${${PROJECT_NAME}_CMAKE_DEFINES}\n")

get_property(_DEFAULT_CMAKE_DEFINES GLOBAL PROPERTY ${PROJECT_NAME}_DEFAULT_CMAKE_DEFINES)
set(${PROJECT_NAME}_DEFAULT_CMAKE_DEFINES)
foreach(_DEF ${_DEFAULT_CMAKE_DEFINES})
    set(${PROJECT_NAME}_DEFAULT_CMAKE_DEFINES "${${PROJECT_NAME}_DEFAULT_CMAKE_DEFINES}\n${_DEF}")
endforeach()
set(${PROJECT_NAME}_DEFAULT_CMAKE_DEFINES "${${PROJECT_NAME}_DEFAULT_CMAKE_DEFINES}\n")

# configure once to generate list of cmakedefine
configure_file(${PROJECT_SOURCE_DIR}/cmake/Templates/defines.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/temporary/defines.h.in @ONLY)

# configure second time to populate cmakedefine
configure_file(${CMAKE_CURRENT_BINARY_DIR}/temporary/defines.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/temporary/defines.h @ONLY)

# configure version file
configure_file(${PROJECT_SOURCE_DIR}/cmake/Templates/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/temporary/version.h @ONLY)

# define where the configured files where be put
if(timemory_MASTER_PROJECT)
    set(CONFIG_FILE_OUTDIR ${CMAKE_CURRENT_BINARY_DIR})
else()
    set(CONFIG_FILE_OUTDIR ${CMAKE_CURRENT_LIST_DIR})
endif()

# only copy if different so that re-running cmake does not cause a rebuild
execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_BINARY_DIR}/temporary/defines.h
        ${CONFIG_FILE_OUTDIR}/defines.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    ERROR_VARIABLE ERR_MSG
    RESULT_VARIABLE RET
)

# only copy if different so that re-running cmake does not cause a rebuild
execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_BINARY_DIR}/temporary/version.h
        ${CONFIG_FILE_OUTDIR}/version.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    ERROR_VARIABLE ERR_MSG
    RESULT_VARIABLE RET
)

if(TIMEMORY_INSTALL_HEADERS)
    # install the configured files
    install(
        FILES
            ${CONFIG_FILE_OUTDIR}/defines.h
            ${CONFIG_FILE_OUTDIR}/version.h
        DESTINATION
            ${CMAKE_INSTALL_INCLUDEDIR}/timemory
        OPTIONAL)
endif()

unset(CONFIG_FILE_OUTDIR)

file(GLOB_RECURSE header_files
    ${CMAKE_CURRENT_SOURCE_DIR}/utility/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hash/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/environment/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/settings/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plotting/*.hpp)
file(GLOB_RECURSE source_files
    ${CMAKE_CURRENT_SOURCE_DIR}/utility/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hash/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/environment/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/settings/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/plotting/*.cpp)

build_intermediate_library(
    USE_INTERFACE
    INSTALL_SOURCE
    NAME                core
    TARGET              core
    CATEGORY            GLOBAL
    FOLDER              components
    HEADERS             ${header_files}
    SOURCES             ${source_files})

if(TIMEMORY_USE_COVERAGE)
    if(TARGET timemory-core-shared)
        target_link_libraries(timemory-core-shared PUBLIC timemory-analysis-tools)
    endif()
    if(TARGET timemory-core-static)
        target_link_libraries(timemory-core-static PUBLIC timemory-analysis-tools)
    endif()
endif()

# global
add_subdirectory(ert)

# do this here for IDE benefit
build_intermediate_library(
    USE_INTERFACE
    NAME                common
    TARGET              common
    CATEGORY            GLOBAL
    FOLDER              components
    PROPERTY_DEPENDS    GLOBAL)

add_subdirectory(manager)
add_subdirectory(storage)
add_subdirectory(operations)
add_subdirectory(variadic)
add_subdirectory(config)
add_subdirectory(backends)

# deprecated
add_subdirectory(deprecated)

# fake libraries
# add_subdirectory(mpl)
# add_subdirectory(data)
# add_subdirectory(general)
# add_subdirectory(compat)
# add_subdirectory(tpls)

# components
add_subdirectory(components)

add_subdirectory(runtime)
add_subdirectory(containers)

# add_subdirectory(compat)
# add_subdirectory(data)
# add_subdirectory(general)
# add_subdirectory(mpl)
# add_subdirectory(utility)

# tools
add_subdirectory(tools)
