

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
VkSemaphore _vkImageReadySem;
VkSemaphore _vkRenderFinishSem;
VkSwapchainKHR _vkSwapChain;
uint32_t _graphicsQueueFamily = -1;
uint32_t _presentQueueFamily = -1;
VkCommandPool _vkCommandPool;
std::vector<VkCommandBuffer>  _vkPresentQueueCmdBuffers;
bool _pauseRender = true;
void GLFWErrorCallback(int error, const char * description)
{
	std::cout << "GLFW Error: " <<  error << description << std::endl;
}

bool CreateSwapChain()
{
	_pauseRender = true;
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
	_pauseRender = false;
	return true;
}

bool RecordCommandBuffers()
{
	uint32_t imageNum = _vkPresentQueueCmdBuffers.size();
	std::vector<VkImage> swapChainImg(imageNum);
	vkGetSwapchainImagesKHR(_vkDevice, _vkSwapChain, &imageNum, swapChainImg.data());
	VkCommandBufferBeginInfo beginInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		nullptr,
		VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
		nullptr
	};
	VkClearColorValue color = {
		{ 1.0f, 0.8f, 0.4f, 0.0f }
	};
	VkImageSubresourceRange imageSubRange =
	{
		VK_IMAGE_ASPECT_COLOR_BIT,
		0,1,
		0,1
	};
	for (uint32_t i = 0; i < _vkPresentQueueCmdBuffers.size(); ++i)
	{
		vkBeginCommandBuffer(_vkPresentQueueCmdBuffers[i], &beginInfo);
		vkCmdClearColorImage(_vkPresentQueueCmdBuffers[i], swapChainImg[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &imageSubRange);
		vkEndCommandBuffer(_vkPresentQueueCmdBuffers[i]);
	}
	return true;
}
bool CreateCommandBuffer()
{
	VkCommandPoolCreateInfo commandPoolCreateInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		nullptr,
		0,
		_presentQueueFamily
	};
	vkCreateCommandPool(_vkDevice, &commandPoolCreateInfo, nullptr, &_vkCommandPool);
	uint32_t numImage;
	if (vkGetSwapchainImagesKHR(_vkDevice, _vkSwapChain, &numImage, nullptr) != VK_SUCCESS)
	{
		return false;
	}
	_vkPresentQueueCmdBuffers.resize(numImage);
	VkCommandBufferAllocateInfo allocInfo = 
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		nullptr,
		_vkCommandPool,
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		numImage
	};
	if (vkAllocateCommandBuffers(_vkDevice, &allocInfo, _vkPresentQueueCmdBuffers.data()) != VK_SUCCESS)
	{
		return false;
	}
	if (!RecordCommandBuffers())
	{
		return false;
	}
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

	//step2 create surface
	if (glfwCreateWindowSurface(_vkInstance, MainWindow, nullptr, &_vkSurface) != VK_SUCCESS)
	{
		std::cout << "glfwCreateWindowSurface failed" << std::endl;
		return false;
	}

	//step3 create device with queue
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
		std::vector<VkBool32>                queueFamilyPresentSupport(numQueueFamily);
		vkGetPhysicalDeviceQueueFamilyProperties(phyDevices[i], &numQueueFamily, queueFamilyProps.data());
		//for (std::size_t j = 0; j < queueFamilyProps.size(); ++j)
		//{
		//	vkGetPhysicalDeviceSurfaceSupportKHR(_vkPhysicalDevice, j, _vkSurface, &queueFamilyPresentSupport[i]);
		//	if (queueFamilyProps[j].queueFlags & VK_QUEUE_GRAPHICS_BIT != 0 && queueFamilyPresentSupport[j] == VK_TRUE)
		//	{
		//		_graphicsQueueFamily = j;
		//		_presentQueueFamily = j;
		//		break;
		//	}
		//}
		if (_graphicsQueueFamily == -1)
		{
			for (std::size_t j = 0; j < queueFamilyProps.size(); ++j)
			{
				vkGetPhysicalDeviceSurfaceSupportKHR(phyDevices[i], j, _vkSurface, &queueFamilyPresentSupport[j]);
				if (queueFamilyProps[j].queueFlags & VK_QUEUE_GRAPHICS_BIT != 0 && _presentQueueFamily == -1)
				{
					_graphicsQueueFamily = j;
				}
				if (queueFamilyPresentSupport[j] == VK_TRUE && _presentQueueFamily == -1)
				{
					_presentQueueFamily = j;
				}
			}
		}
		if (_graphicsQueueFamily != -1 && _presentQueueFamily != -1)
		{
			phyDeviceIndex = i;
			break;
		}
	}
	if (_graphicsQueueFamily == -1 || phyDeviceIndex == -1)
	{
		std::cout << "can't find phydevices" << std::endl;
		return false;
	}
	_vkPhysicalDevice = phyDevices[phyDeviceIndex];
	float queuProp = 1.0f;
	std::vector< VkDeviceQueueCreateInfo> queueCreateInfos = {
	{
		VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		nullptr,
		0,
		_graphicsQueueFamily,
		1,
		&queuProp
	} };
	if (_graphicsQueueFamily != _presentQueueFamily)
	{
		queueCreateInfos.push_back({
			VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			nullptr,
			0,
			_presentQueueFamily,
			1,
			&queuProp
			});
	}
	std::vector<const char*> SwapChainextensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	VkDeviceCreateInfo deviceCreateInfo = 
	{
		VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		nullptr,
		0,
		1,
		queueCreateInfos.data(),
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
	vkGetDeviceQueue(_vkDevice, _graphicsQueueFamily, 0, &_vkGraphicsQueue);
	vkGetDeviceQueue(_vkDevice, _presentQueueFamily, 0, &_vkPresentQueue);
	if (_vkGraphicsQueue == nullptr || _vkPresentQueue == nullptr)
	{
		std::cout << "vk GetQueue Failed" << std::endl;
		return false;
	}

	//step3.1 create semaphore
	VkSemaphoreCreateInfo semaphoreCreateInfo = 
	{
		VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		nullptr,
		0
	};
	if (vkCreateSemaphore(_vkDevice, &semaphoreCreateInfo, nullptr, &_vkImageReadySem) != VK_SUCCESS
		|| vkCreateSemaphore(_vkDevice, &semaphoreCreateInfo, nullptr, &_vkRenderFinishSem) != VK_SUCCESS)
	{
		std::cout << "vkCreateSemaphore Failed" << std::endl;
		return false;
	}

	//step4 create swap chain
	if (!CreateSwapChain())
	{
		std::cout << " CreateSwapChain failed " << std::endl;
		return false;
	}

	//step5 create command buffer
	if (!CreateCommandBuffer())
	{
		std::cout << " CreateCommandBuffer failed " << std::endl;
		return false;
	}
	return true;
}
void UnInitVulkan()
{
	vkDeviceWaitIdle(_vkDevice);
	vkDestroyCommandPool(_vkDevice, _vkCommandPool, nullptr);
	vkDestroySwapchainKHR(_vkDevice, _vkSwapChain,nullptr);
	vkDestroySurfaceKHR(_vkInstance, _vkSurface,nullptr);
	vkDestroyDevice(_vkDevice, nullptr);
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
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(_vkDevice, _vkSwapChain, UINT64_MAX, _vkImageReadySem, VK_NULL_HANDLE, &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		CreateSwapChain();
		return;
	}
	else if (result == VK_SUCCESS)
	{


		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		VkSubmitInfo submitInfo =
		{
			VK_STRUCTURE_TYPE_SUBMIT_INFO,
			nullptr,
			1,
			&_vkImageReadySem,
			&waitStage,
			1,
			&_vkPresentQueueCmdBuffers[imageIndex],
			1,
			&_vkRenderFinishSem
		};
		if (vkQueueSubmit(_vkPresentQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
		{
			return;
		}
		VkPresentInfoKHR presentInfo =
		{
			VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			nullptr,
			1,
			&_vkRenderFinishSem,
			1,
			&_vkSwapChain,
			&imageIndex,
			nullptr
		};
		result = vkQueuePresentKHR(_vkPresentQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			CreateSwapChain();
			return;
		}
	}
}
int main( int argc, char * argv[ ] )
{
	InitGLFW();

	InitVulkan();

	while( glfwWindowShouldClose( MainWindow ) == 0 )
	{
		Update();
		if (!_pauseRender) {
			Render();
		}
		glfwPollEvents( );
	}

	UnInitVulkan();

	glfwDestroyWindow( MainWindow );
	glfwTerminate( );	
	return 0;
}

