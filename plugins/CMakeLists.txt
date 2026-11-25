# Every sub-folder that has a CMakeLists.txt is built and packaged into submods/.
file(GLOB _plugin_dirs LIST_DIRECTORIES true "${CMAKE_CURRENT_SOURCE_DIR}/*")
list(SORT _plugin_dirs)
foreach(_dir IN LISTS _plugin_dirs)
    if(IS_DIRECTORY "${_dir}" AND EXISTS "${_dir}/CMakeLists.txt")
        add_subdirectory("${_dir}")
    endif()
endforeach()
