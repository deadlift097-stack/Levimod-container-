# add_container_plugin(<target> SOURCES <files...>)
# Builds lib<target>.so as a Mod Container plugin and packs it into submods/ of the .levipack.
function(add_container_plugin target)
    cmake_parse_arguments(PLUGIN "" "" "SOURCES" ${ARGN})
    if(NOT PLUGIN_SOURCES)
        message(FATAL_ERROR "add_container_plugin(${target}) needs SOURCES")
    endif()

    add_library(${target} SHARED ${PLUGIN_SOURCES})
    set_target_properties(${target} PROPERTIES
        OUTPUT_NAME "${target}"
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)
    target_include_directories(${target} PRIVATE "${CMAKE_SOURCE_DIR}/sdk")
    target_link_options(${target} PRIVATE "-Wl,-z,max-page-size=16384")
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${CMAKE_STRIP}" --strip-unneeded "$<TARGET_FILE:${target}>"
        VERBATIM)

    set_property(GLOBAL APPEND PROPERTY LEVI_CONTAINER_PLUGINS ${target})
endfunction()
