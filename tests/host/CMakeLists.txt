# Host-side tests (desktop compiler, no Android needed). Enabled with -DLEVI_HOST_TESTS=ON.
# The Mod Menu / log / launcher parts are stubbed in test_stubs.cpp; the plugin .so files are
# real shared libraries loaded with dlopen.

set(PLUGIN_TEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/plugins_under_test")
file(MAKE_DIRECTORY "${PLUGIN_TEST_DIR}")

function(add_test_plugin target)
    add_library(${target} SHARED ${ARGN})
    target_include_directories(${target} PRIVATE "${CMAKE_SOURCE_DIR}/sdk")
    set_target_properties(${target} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_TEST_DIR}"
        CXX_VISIBILITY_PRESET hidden)
    add_dependencies(levi_plugin_test ${target})
endfunction()

add_executable(levi_plugin_test
    test_main.cpp
    test_stubs.cpp
    "${CMAKE_SOURCE_DIR}/src/container/PluginLoader.cpp"
    "${CMAKE_SOURCE_DIR}/src/container/ExternalSubMod.cpp"
    "${CMAKE_SOURCE_DIR}/src/container/SubModRegistry.cpp"
)
target_compile_features(levi_plugin_test PRIVATE cxx_std_20)
target_include_directories(levi_plugin_test PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/sdk"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/stubs"
    "${LEVI_PRELOADER_INCLUDE_DIR}"
)
target_compile_definitions(levi_plugin_test PRIVATE
    PL_CONFIG_NO_RUNTIME
    PLUGIN_TEST_DIR="${PLUGIN_TEST_DIR}")
target_link_libraries(levi_plugin_test PRIVATE
    nlohmann_json::nlohmann_json Boost::pfr magic_enum::magic_enum fmt::fmt ${CMAKE_DL_LIBS})

# The real example plugin plus the broken ones.
add_test_plugin(hello_plugin "${CMAKE_SOURCE_DIR}/plugins/hello_plugin/hello_plugin.cpp")
add_test_plugin(bad_plugin_abi bad_plugin_abi.cpp)
add_test_plugin(bad_plugin_id bad_plugin_id.cpp)
add_test_plugin(dup_plugin dup_plugin.cpp)
add_test_plugin(failing_plugin failing_plugin.cpp)
add_test_plugin(helper_lib helper_lib.cpp)

add_test(NAME plugin_pipeline COMMAND levi_plugin_test)
