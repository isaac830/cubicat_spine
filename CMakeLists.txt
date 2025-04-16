get_filename_component(CMAKELISR_DIR "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
# js generation
execute_process(
    COMMAND python ${CMAKELISR_DIR}/tools/js_api_generator.py
    WORKING_DIRECTORY ${CMAKELISR_DIR}
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "JS export generation failed!")
endif()

set(INCLUDE_DIRS ".")
if(CONFIG_SPINE_VERSION_38)
    list(APPEND INCLUDE_DIRS "./spine-cpp_3.8/include")
    file(GLOB_RECURSE SRCS "./cubicat-port/*.cpp" "./js_binding/*.cpp" "./spine-cpp_3.8/*.cpp")
elseif(CONFIG_SPINE_VERSION_40)
    list(APPEND INCLUDE_DIRS "./spine-cpp_4.0/include")
    file(GLOB_RECURSE SRCS "./cubicat-port/*.cpp" "./js_binding/*.cpp" "./spine-cpp_4.0/*.cpp")
else()
    list(APPEND INCLUDE_DIRS "./spine-cpp_4.2/include")
    file(GLOB_RECURSE SRCS "./cubicat-port/*.cpp" "./js_binding/*.cpp" "./spine-cpp_4.2/*.cpp")
endif()

idf_component_register(SRCS ${SRCS}
                        PRIV_REQUIRES cubicat_s3
                       INCLUDE_DIRS ${INCLUDE_DIRS})
target_compile_options(${COMPONENT_LIB} PRIVATE -fexceptions -Wno-error=reorder -Wno-error=parentheses)