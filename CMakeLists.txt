cmake_minimum_required( VERSION 2.6 )
project( vulkan-tutorial )
#add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)
add_subdirectory(External/glfw)
include_directories( External )
include_directories( External/vulkan )
include_directories( External/glfw/include)
find_library(Vulkan_LIBRARY NAMES vulkan-1 vulkan)
SUBDIRS(tutorial1)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/External/vulkan/vulkan-1.lib ${CMAKE_CURRENT_BINARY_DIR}/External/vulkanvulkan-1.lib)
		