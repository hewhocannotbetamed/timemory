
if(TIMEMORY_BUILD_PYTHON AND TIMEMORY_BUILD_PYTHON_LINE_PROFILER)
    checkout_git_submodule(RECURSIVE
        RELATIVE_PATH external/line-profiler
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        REPO_URL https://github.com/jrmadsen/line_profiler.git
        REPO_BRANCH timemory)

    message(STATUS "Adding external/line-profiler...")
    add_subdirectory(line-profiler)

    if(TARGET libpytimemory AND TARGET _line_profiler)
        add_dependencies(libpytimemory _line_profiler)
    endif()
endif()

if(TIMEMORY_BUILD_PYTHON AND TIMEMORY_BUILD_PYTHON_HATCHET)
    checkout_git_submodule(RECURSIVE
        RELATIVE_PATH external/hatchet
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        REPO_URL https://github.com/jrmadsen/hatchet.git
        REPO_BRANCH timemory)

    # line_profiler might have already found Cython
    if(CYTHON_EXECUTABLE)
        find_package(Cython QUIET)
    endif()

    if(CYTHON_EXECUTABLE OR Cython_FOUND)
        message(STATUS "Adding external/hatchet...")
        add_subdirectory(hatchet)
    else()
        message(WARNING "Internal Hatchet could not built because Cython could not be found")
        set(TIMEMORY_BUILD_PYTHON_HATCHET OFF PARENT_SCOPE)
    endif()

    if(TARGET libpytimemory AND TARGET hatchet)
        add_dependencies(libpytimemory hatchet)
        if(CMAKE_INSTALL_PYTHONDIR)
            set(HATCHET_PYDIR "${CMAKE_CURRENT_LIST_DIR}/hatchet/hatchet")
            file(GLOB_RECURSE hatchet_files
                "${HATCHET_PYDIR}/*.py"
                "${HATCHET_PYDIR}/*.js")
            file(GLOB_RECURSE hatchet_test_files
                "${HATCHET_PYDIR}/tests/*")

            set(HATCHET_CYTHONDIR
                "${HATCHET_PYDIR}/cython_modules/libs")

            set(hatchet_install_files ${hatchet_files} ${hatchet_test_files})
            list(REMOVE_DUPLICATES hatchet_install_files)
            foreach(_FILE ${hatchet_install_files})
                get_filename_component(_SUBDIR "${_FILE}" PATH)
                string(REPLACE "${CMAKE_CURRENT_LIST_DIR}/hatchet/" "" _SUBDIR "${_SUBDIR}")
                install(FILES ${_FILE}
                    DESTINATION ${CMAKE_INSTALL_PYTHONDIR}/timemory/${_SUBDIR}
                    OPTIONAL)
            endforeach()

            # cython build modules
            install(FILES
                "${HATCHET_CYTHONDIR}/graphframe_modules${PYTHON_MODULE_EXTENSION}"
                "${HATCHET_CYTHONDIR}/reader_modules${PYTHON_MODULE_EXTENSION}"
                DESTINATION ${CMAKE_INSTALL_PYTHONDIR}/timemory/hatchet/cython_modules/libs
                OPTIONAL)
        endif()
    endif()
endif()
