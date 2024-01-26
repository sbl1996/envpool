# set(CONDA_PREFIX $ENV{CONDA_PREFIX})
set(PYTHON_EXECUTABLE ${CONDA_PREFIX}/bin/python3)
set(PYTHON_LIBRARY ${CONDA_PREFIX}/lib/libpython3.so)

execute_process(
    COMMAND ${PYTHON_EXECUTABLE} -V
    OUTPUT_VARIABLE python_version_output
    ERROR_VARIABLE python_version_error
    RESULT_VARIABLE python_version_result
)

# move the build target to site-packages
function(pybind_move_target target_name target_dir)
file(TO_CMAKE_PATH "${target_dir}" target_dir)
    set(install_target_path ${CMAKE_CURRENT_SOURCE_DIR}/${target_dir}/$<TARGET_FILE_NAME:${target_name}> CACHE INTERNAL "")
    add_custom_command(
        TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:${target_name}> ${install_target_path}
        COMMAND ${CMAKE_COMMAND} -E echo "Move ${target_name} to ${install_target_path}"
    )
endfunction()

# if(python_version_result EQUAL 0)
#     string(REGEX REPLACE "Python ([0-9]+\\.[0-9]+).*" "\\1" python_version "${python_version_output}")
#     message("Using Python version: ${python_version} from ${CONDA_PREFIX}")
#     set(PYC_PYTHON_VERSION ${python_version} CACHE STRING "Python version" FORCE)
# else()
#     message(FATAL_ERROR "Failed to get Python version: ${python_version_error}")
# endif()


# # move the build target to site-packages
# function(pybind_install_target target_name)
#     set(install_target_path ${CONDA_PREFIX}/lib/python${PYC_PYTHON_VERSION}/site-packages/$<TARGET_FILE_NAME:${target_name}> CACHE INTERNAL "")
#     add_custom_command(
#         TARGET ${target_name} POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy_if_different
#                 $<TARGET_FILE:${target_name}> ${install_target_path}
#         COMMAND ${CMAKE_COMMAND} -E echo "Installing ${target_name} to ${install_target_path}"
#     )
# endfunction()