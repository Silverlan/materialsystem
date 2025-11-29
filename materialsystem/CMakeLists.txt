include(${CMAKE_SOURCE_DIR}/cmake/pr_common.cmake)

option(CONFIG_DISABLE_VMT_SUPPORT "Disable support for VMT/VTF files." OFF)
option(CONFIG_DISABLE_VTF_SUPPORT "Disable support for VMT/VTF files." OFF)
option(CONFIG_DISABLE_VTEX_SUPPORT "Disable support for VTex files." OFF)
option(CONFIG_DISABLE_VMAT_SUPPORT "Disable support for VMat files." OFF)

set(PROJ_NAME materialsystem)
pr_add_library(${PROJ_NAME} SHARED)

pr_add_dependency(${PROJ_NAME} datasystem TARGET PUBLIC)
pr_add_dependency(${PROJ_NAME} util_image TARGET PUBLIC)
pr_add_dependency(${PROJ_NAME} util_source2 TARGET PUBLIC)
pr_add_dependency(${PROJ_NAME} util_udm TARGET PUBLIC)
pr_add_dependency(${PROJ_NAME} lz4 TARGET)
pr_add_dependency(${PROJ_NAME} KVParser TARGET PUBLIC)
pr_add_dependency(${PROJ_NAME} VTFLib13 TARGET)

pr_add_external_dependency(${PROJ_NAME} zlib LIBRARY PRIVATE)
pr_add_external_dependency(${PROJ_NAME} libpng LIBRARY PRIVATE)
pr_add_external_dependency(${PROJ_NAME} libpng_build HEADER_ONLY PRIVATE)

pr_init_module(${PROJ_NAME})

pr_add_compile_definitions(${PROJ_NAME} -DMATSYS_DLL)

if(CONFIG_DISABLE_VMT_SUPPORT)
    pr_add_compile_definitions(${PROJ_NAME} -DDISABLE_VMT_SUPPORT)
endif()
if(CONFIG_DISABLE_VTF_SUPPORT)
    pr_add_compile_definitions(${PROJ_NAME} -DDISABLE_VTF_SUPPORT)
endif()
if(CONFIG_DISABLE_VTEX_SUPPORT)
    pr_add_compile_definitions(${PROJ_NAME} -DDISABLE_VTEX_SUPPORT)
endif()

if(MSVC)
    target_compile_options(${PROJ_NAME} PRIVATE "/bigobj")
endif()

pr_finalize(${PROJ_NAME})
