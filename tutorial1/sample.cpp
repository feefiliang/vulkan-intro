

#include "vulkan.h"
#include "vk_sdk_platform.h"
#include <iostream>
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
int Width = 1024;
int Height = 768;
GLFWwindow *			MainWindow;
VkInstance _vkInstance;
VkPhysicalDevice _vkPhysicalDevice;
VkSurfaceKHR _vkSurface;
VkDevice _vkDevice;
VkQueue _vkGraphicsQueue;
VkQueue _vkPresentQueue;
VkSwapchainKHR _vkSwapChain;
void GLFWErrorCallback(int error, const char * description)
{
	std::cout << "GLFW Error: " <<  error << description << std::endl;
}

bool CreateSwapChain()
{
	if (_vkDevice != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(_vkDevice);
	}

	VkSurfaceCapabilitiesKHR surfaceCap;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_vkPhysicalDevice, _vkSurface, &surfaceCap);
	uint32_t numSurfaceFormat;
	vkGetPhysicalDeviceSurfaceFormatsKHR(_vkPhysicalDevice, _vkSurface, &numSurfaceFormat, nullptr);
	std::vector<VkSurfaceFormatKHR> surfaceFormats(numSurfaceFormat);
	vkGetPhysicalDeviceSurfaceFormatsKHR(_vkPhysicalDevice, _vkSurface, &numSurfaceFormat, surfaceFormats.data());
	uint32_t numPresentModes;
	vkGetPhysicalDeviceSurfacePresentModesKHR(_vkPhysicalDevice, _vkSurface, &numPresentModes, nullptr);
	std::vector<VkPresentModeKHR> presentModes(numPresentModes);
	vkGetPhysicalDeviceSurfacePresentModesKHR(_vkPhysicalDevice, _vkSurface, &numPresentModes, presentModes.data());

	
	VkSwapchainKHR   old_swap_chain = _vkSwapChain;
	VkSwapchainCreateInfoKHR swapchainCreateInfo =
	{
		VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		nullptr,
		0,
		_vkSurface,
		surfaceCap.minImageCount,
		surfaceFormats[0].format,
		surfaceFormats[0].colorSpace,
		{ Width ,Height},
		1,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr,
		surfaceCap.currentTransform,
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_PRESENT_MODE_MAILBOX_KHR,
		VK_TRUE,
		old_swap_chain
	};
	if (vkCreateSwapchainKHR(_vkDevice, &swapchainCreateInfo, nullptr, &_vkSwapChain) != VK_SUCCESS)
	{
		return false;
	}
	vkDestroySwapchainKHR(_vkDevice,old_swap_chain, nullptr);
	return true;
}
bool InitVulkan()
{
	//step1 create instace 
	VkApplicationInfo appInfo =
	{
		VK_STRUCTURE_TYPE_APPLICATION_INFO,
		nullptr,
		"vulkan tutorial 1",
		100,
		"vulkan 1.0.1",
		1,
		VK_MAKE_VERSION(1,0,0)
	};

	std::vector<const char*> extensions = {
		"VK_KHR_surface",
		"VK_KHR_win32_surface"
	};

	VkInstanceCreateInfo instanceCreateinfo =
	{
		VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		nullptr,
		0,
		&appInfo,
		0,nullptr,
		2,
		extensions.data()
	};
	
	vkCreateInstance(&instanceCreateinfo, nullptr, &_vkInstance);
	if (_vkInstance == nullptr)
	{
		std::cout << "vkCreateInstance failed" << std::endl;
		return false;
	}
	//step2 create device with queue
	uint32_t numPhysicsDevice;
	if (vkEnumeratePhysicalDevices(_vkInstance, &numPhysicsDevice, nullptr) != VK_SUCCESS)
	{
		std::cout << "vkEnumeratePhysicalDevices failed" << std::endl;
		return false;
	}
	std::vector<VkPhysicalDevice> phyDevices(numPhysicsDevice);
	if (vkEnumeratePhysicalDevices(_vkInstance, &numPhysicsDevice, phyDevices.data()) != VK_SUCCESS)
	{
		std::cout << "vkEnumeratePhysicalDevices failed" << std::endl;
		return false;
	}
	uint32_t queueFamilyIndex = -1;
	uint32_t phyDeviceIndex = -1;
	for (std::size_t i = 0; i < phyDevices.size(); ++i)
	{
		VkPhysicalDeviceProperties prop;
		VkPhysicalDeviceFeatures feature;
		vkGetPhysicalDeviceProperties(phyDevices[i], &prop);
		vkGetPhysicalDeviceFeatures(phyDevices[i], &feature);
		std::cout <<"deviceName: " <<  prop.deviceName << std::endl;

		uint32_t numQueueFamily;
		vkGetPhysicalDeviceQueueFamilyProperties(phyDevices[i], &numQueueFamily, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilyProps(numQueueFamily);
		vkGetPhysicalDeviceQueueFamilyProperties(phyDevices[i], &numQueueFamily, queueFamilyProps.data());
		for (std::size_t j = 0; j < queueFamilyProps.size(); ++j)
		{
			if (queueFamilyProps[j].queueFlags & VK_QUEUE_GRAPHICS_BIT != 0)
			{
				queueFamilyIndex = j;
				break;
			}
		}
		if (queueFamilyIndex != -1)
		{
			phyDeviceIndex = i;
			break;
		}
	}
	if (queueFamilyIndex == -1 || phyDeviceIndex == -1)
	{
		std::cout << "can't find phydevices" << std::endl;
		return false;
	}
	_vkPhysicalDevice = phyDevices[phyDeviceIndex];
	float queuProp = 1.0f;
	VkDeviceQueueCreateInfo queueCreateInfo = 
	{
		VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		nullptr,
		0,
		queueFamilyIndex,
		1,
		&queuProp
	};
	std::vector<const char*> SwapChainextensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	VkDeviceCreateInfo deviceCreateInfo = 
	{
		VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		nullptr,
		0,
		1,
		&queueCreateInfo,
		0,
		nullptr,
		SwapChainextensions.size(),
		SwapChainextensions.data(),
		nullptr
	};
	if (vkCreateDevice(_vkPhysicalDevice, &deviceCreateInfo, nullptr, &_vkDevice) != VK_SUCCESS)
	{
		std::cout << "vkCreateDevice failed" << std::endl;
		return false;
	}
	vkGetDeviceQueue(_vkDevice, queueFamilyIndex, 0, &_vkGraphicsQueue);
	if (_vkGraphicsQueue == nullptr)
	{
		std::cout << "vk GetQueue Failed" << std::endl;
		return false;
	}

	//step3 create surface
	if (glfwCreateWindowSurface(_vkInstance, MainWindow, nullptr, &_vkSurface) != VK_SUCCESS)
	{
		std::cout << "glfwCreateWindowSurface failed" << std::endl;
		return false;
	}

	//step4 create swap chain
	if (!CreateSwapChain())
	{
		std::cout << " CreateSwapChain failed " << std::endl;
	}

	//step5 create command buffer
	return true;
}
void UnInitVulkan()
{
	vkDeviceWaitIdle(_vkDevice);
	vkDestroyDevice(_vkDevice,nullptr);
	vkDestroySurfaceKHR(_vkInstance, _vkSurface,nullptr);
	if (_vkInstance != nullptr)
	{
		vkDestroyInstance(_vkInstance, nullptr);
	}
}
void InitGLFW()
{
	glfwSetErrorCallback(GLFWErrorCallback);
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	MainWindow = glfwCreateWindow(Width, Height, "Vulkan Sample", NULL, NULL);

	uint32_t count;
	const char ** extensions = glfwGetRequiredInstanceExtensions(&count);

}

void Update()
{

}
void Render()
{

}
int main( int argc, char * argv[ ] )
{
	InitGLFW();

	InitVulkan();

	while( glfwWindowShouldClose( MainWindow ) == 0 )
	{
		Update();
		Render();
		glfwPollEvents( );
	}

	UnInitVulkan();

	glfwDestroyWindow( MainWindow );
	glfwTerminate( );	
	return 0;
}

