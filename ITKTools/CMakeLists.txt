find_package(ITK REQUIRED)
include(${ITK_USE_FILE})

add_executable(GetMultiVolumeStats GetMultiVolumeStats.cxx)
target_link_libraries(GetMultiVolumeStats ${ITK_LIBRARIES})
