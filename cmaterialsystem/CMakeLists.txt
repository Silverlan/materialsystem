include(${CMAKE_SOURCE_DIR}/cmake/pr_common.cmake)

option(CONFIG_DISABLE_VTF_SUPPORT "Enable support for VMT/VTF files." OFF)

set(PROJ_NAME cmaterialsystem)
pr_add_library(${PROJ_NAME} SHARED)

pr_add_dependency(${PROJ_NAME} iglfw TARGET PUBLIC)
pr_add_dependency(${PROJ_NAME} prosper TARGET PUBLIC)
pr_add_dependency(${PROJ_NAME} materialsystem TARGET PUBLIC)
pr_add_dependency(${PROJ_NAME} lz4 TARGET)
pr_add_dependency(${PROJ_NAME} VTFLib13 TARGET)

pr_add_dependency(${PROJ_NAME} gli TARGET)
pr_add_external_dependency(${PROJ_NAME} zlib LIBRARY PRIVATE)
pr_add_external_dependency(${PROJ_NAME} libpng LIBRARY PRIVATE)
pr_add_external_dependency(${PROJ_NAME} libpng_build HEADER_ONLY PRIVATE)

pr_init_module(${PROJ_NAME})

pr_add_compile_definitions(${PROJ_NAME} -DDLLCMATSYS_EX -DUIMG_ENABLE_NVTT)

if(CONFIG_DISABLE_VMT_SUPPORT)
    pr_add_compile_definitions(${PROJ_NAME} -DDISABLE_VMT_SUPPORT)
endif()
if(CONFIG_DISABLE_VTF_SUPPORT)
    pr_add_compile_definitions(${PROJ_NAME} -DDISABLE_VTF_SUPPORT)
endif()
if(CONFIG_DISABLE_VTEX_SUPPORT)
    pr_add_compile_definitions(${PROJ_NAME} -DDISABLE_VTEX_SUPPORT)
endif()

pr_finalize(${PROJ_NAME})
