

#include "vulkan.h"
#include "vk_sdk_platform.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
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
VkPipeline _vkGraphicsPipeline;
VkRenderPass _vkRenderPass;
std::vector<VkImageView>	_vkImageViews;
std::vector<VkImage> _vkImages;
std::vector<VkFramebuffer>	_vkFrameBuffers;
VkFormat _swapChainFormat;

bool _pauseRender = true;

void GLFWErrorCallback(int error, const char * description)
{
	std::cout << "GLFW Error: " <<  error << description << std::endl;
}

// ************************************************************ //
// GetBinaryFileContents                                        //
//                                                              //
// Function reading binary contents of a file                   //
// ************************************************************ //
std::vector<char> GetBinaryFileContents(const std::string  &filename) {

	std::ifstream file(filename, std::ios::binary);
	if (file.fail()) {
		std::cout << "Could not open \"" << filename << "\" file!" << std::endl;
		return std::vector<char>();
	}

	std::streampos begin, end;
	begin = file.tellg();
	file.seekg(0, std::ios::end);
	end = file.tellg();

	std::vector<char> result(static_cast<size_t>(end - begin));
	file.seekg(0, std::ios::beg);
	file.read(&result[0], end - begin);
	file.close();

	return result;
}

std::shared_ptr<VkShaderModule_T> CreateShaderModule(const std::string& fileName)
{
	auto content = GetBinaryFileContents(fileName);
	if (content.empty())
	{
		return nullptr;
	}
	VkShaderModuleCreateInfo shaderModuleCreateInfo =
	{
		VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		nullptr,
		0,
		(size_t)content.size(),
		(uint32_t*)content.data()
	};
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(_vkDevice, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		return nullptr;
	}
	return std::shared_ptr<VkShaderModule_T>(shaderModule, [](VkShaderModule m) {vkDestroyShaderModule(_vkDevice, m, nullptr); });	
}
bool CreateImageViews()
{
	uint32_t numImage;
	if (vkGetSwapchainImagesKHR(_vkDevice, _vkSwapChain, &numImage, nullptr) != VK_SUCCESS)
	{
		return false;
	}
	_vkImages.resize(numImage);
	_vkImageViews.resize(numImage);
	vkGetSwapchainImagesKHR(_vkDevice, _vkSwapChain, &numImage, &_vkImages[0]);
	for (std::size_t i = 0; i < _vkImageViews.size(); ++i)
	{
		VkImageViewCreateInfo imgViewCreateInfo = 
		{
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			nullptr,
			0,
			_vkImages[i],
			VK_IMAGE_VIEW_TYPE_2D,
			_swapChainFormat,
			{
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY
			},
			{
				VK_IMAGE_ASPECT_COLOR_BIT,
				0,1,
				0,1
			}
		};
		if (vkCreateImageView(_vkDevice, &imgViewCreateInfo, nullptr, &_vkImageViews[i]) != VK_SUCCESS)
		{
			std::cout << "create img view failed";
			return false;
		}
	}
	return true;
}
bool CreateFrameBuffer();
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
	_swapChainFormat = surfaceFormats[0].format;
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
	if (!CreateImageViews())
	{
		return false;
	}
	if (!CreateFrameBuffer())
	{
		return false;
	}
	_pauseRender = false;
	return true;
}

bool RecordCommandBuffers()
{
	uint32_t imageNum = _vkPresentQueueCmdBuffers.size();
	VkCommandBufferBeginInfo beginInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		nullptr,
		VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
		nullptr
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
		//trans from present to write
		VkImageMemoryBarrier barrierFromPresent2Draw =
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			nullptr,
			VK_ACCESS_MEMORY_READ_BIT,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			_presentQueueFamily,
			_graphicsQueueFamily,
			_vkImages[i],
			imageSubRange
		};
		vkCmdPipelineBarrier(_vkPresentQueueCmdBuffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0,nullptr, 0, nullptr, 1,&barrierFromPresent2Draw);
		//vkCmdClearColorImage(_vkPresentQueueCmdBuffers[i], swapChainImg[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &imageSubRange);
		VkClearValue clearValue = 
		{
			{1.0f,0.8f,0.4f,0.0f}
		};
		VkRenderPassBeginInfo renderPassBeginInfo = 
		{
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			nullptr,
			_vkRenderPass,
			_vkFrameBuffers[i],
			{{0,0},{Width,Height}},
			1,
			&clearValue
		};
		vkCmdBeginRenderPass(_vkPresentQueueCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(_vkPresentQueueCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _vkGraphicsPipeline);
		vkCmdDraw(_vkPresentQueueCmdBuffers[i], 3, 1, 0, 0);
		vkCmdEndRenderPass(_vkPresentQueueCmdBuffers[i]);
		//trans from write to present
		VkImageMemoryBarrier barrierFromClear2present =
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			nullptr,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_MEMORY_READ_BIT,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			_graphicsQueueFamily,
			_presentQueueFamily,
			_vkImages[i],
			imageSubRange
		};
		vkCmdPipelineBarrier(_vkPresentQueueCmdBuffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierFromClear2present);
		if (vkEndCommandBuffer(_vkPresentQueueCmdBuffers[i]) != VK_SUCCESS)
		{
			std::cout << "vkEndCommandBuffer error";
			return false;
		}
	}
	return true;
}
void ClearCommandBuffer();
bool CreateCommandBuffer()
{
	ClearCommandBuffer();
	VkCommandPoolCreateInfo commandPoolCreateInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		nullptr,
		0,
		_graphicsQueueFamily
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
bool CreateFrameBuffer()
{
	_vkFrameBuffers.resize(_vkImageViews.size());
	for (std::size_t i = 0; i < _vkFrameBuffers.size(); ++i)
	{
		VkFramebufferCreateInfo framebufferCreateInfo =
		{
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			nullptr,
			0,
			_vkRenderPass,
			1,
			&_vkImageViews[i],
			Width,
			Height,
			1
		};
		if (vkCreateFramebuffer(_vkDevice, &framebufferCreateInfo, nullptr, &_vkFrameBuffers[i]) != VK_SUCCESS)
		{
			std::cout << "create frame buffer error";
			return false;
		}
	}
	return true;
}
bool CreatePipeline()
{
	//step1 create render pass
	VkAttachmentDescription attachmentDesc =
	{
		0,
		_swapChainFormat,
		VK_SAMPLE_COUNT_1_BIT,
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	};
	VkAttachmentReference attachRef =
	{
		0,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	VkSubpassDescription subpassDesc = 
	{
		0,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		0,
		nullptr,
		1,
		&attachRef,
		nullptr,
		nullptr,
		0,
		nullptr
	};
	VkRenderPassCreateInfo renderPassCreateInfo =
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		nullptr,
		0,
		1,
		&attachmentDesc,
		1,
		&subpassDesc,
		0,
		nullptr
	};
	vkCreateRenderPass(_vkDevice, &renderPassCreateInfo, nullptr, &_vkRenderPass);

	//step2 shader stage
	auto vshader = CreateShaderModule("../data/shader.vert.spv");
	auto fshader = CreateShaderModule("../data/shader.frag.spv");
	if (vshader == nullptr || fshader == nullptr)
	{
		return false;
	}
	VkPipelineShaderStageCreateInfo shaderStageCreateInfo[] =
	{
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			nullptr,
			0,
			VK_SHADER_STAGE_VERTEX_BIT,
			vshader.get(),
			"main",
			nullptr
		},
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			nullptr,
			0,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			fshader.get(),
			"main",
			nullptr
		},
	};

	//step3 state info
	VkPipelineVertexInputStateCreateInfo inputStateCreateInfo = 
	{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		nullptr,
		0,
		0,nullptr,
		0,nullptr,
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		nullptr,
		0,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_FALSE
	};

	VkViewport viewport =
	{
		0,0,
		Width,Height,
		0,1.0f
	};
	VkRect2D scissors =
	{
		{0,0},
		{Width,Height}
	};
	VkPipelineViewportStateCreateInfo viewportStateCreateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		nullptr,
		0,
		1,&viewport,
		1,&scissors
	};
	VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo = 
	{
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		nullptr,
		0,
		VK_FALSE,
		VK_FALSE,
		VK_POLYGON_MODE_FILL,
		VK_CULL_MODE_BACK_BIT,
		VK_FRONT_FACE_COUNTER_CLOCKWISE,
		VK_FALSE,
		0,0,0,1.0f
	};
	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = 
	{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		nullptr,
		0,
		VK_SAMPLE_COUNT_1_BIT,
		VK_FALSE,
		1,
		nullptr,
		VK_FALSE,
		VK_FALSE,
	};
	VkPipelineColorBlendAttachmentState blendState =
	{
		VK_FALSE,
		VK_BLEND_FACTOR_ONE,
		VK_BLEND_FACTOR_ONE,
		VK_BLEND_OP_SUBTRACT,
		VK_BLEND_FACTOR_ONE,
		VK_BLEND_FACTOR_ONE,
		VK_BLEND_OP_ADD,
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |         // VkColorComponentFlags                          colorWriteMask
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};
	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		nullptr,
		0,
		VK_FALSE,
		VK_LOGIC_OP_NO_OP,
		1,
		&blendState
	};
	VkPipelineLayoutCreateInfo layoutCreateInfo = 
	{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		0,
		nullptr,
		0,
		nullptr,
	};
	VkPipelineLayout layouto;
	vkCreatePipelineLayout(_vkDevice, &layoutCreateInfo, nullptr, &layouto);
	std::shared_ptr<VkPipelineLayout_T> layout(layouto, [](VkPipelineLayout l) {vkDestroyPipelineLayout(_vkDevice,l,nullptr); });



	VkGraphicsPipelineCreateInfo createInfo = 
	{
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		nullptr,
		0,
		2,
		shaderStageCreateInfo,
		&inputStateCreateInfo,
		&inputAssemblyStateCreateInfo,
		nullptr,
		&viewportStateCreateInfo,
		&rasterizationCreateInfo,
		&multisampleStateCreateInfo,
		nullptr,
		&colorBlendStateCreateInfo,
		nullptr,
		layout.get(),
		_vkRenderPass,
		0,
		VK_NULL_HANDLE,
		-1
	};
	if (vkCreateGraphicsPipelines(_vkDevice, VK_NULL_HANDLE, 1, &createInfo, nullptr, &_vkGraphicsPipeline) != VK_SUCCESS)
	{
		std::cout << " crate pipe line error" << std::endl;
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

	//step5 create pipleline
	if (!CreatePipeline())
	{
		return false;
	}

	//step6 create command buffer
	if (!CreateCommandBuffer())
	{
		std::cout << " CreateCommandBuffer failed " << std::endl;
		return false;
	}


	return true;
}
void ClearCommandBuffer()
{
	if (_vkDevice != VK_NULL_HANDLE)
	{
		vkDeviceWaitIdle(_vkDevice);
		if (!_vkPresentQueueCmdBuffers.empty() && _vkCommandPool != VK_NULL_HANDLE)
		{
			vkFreeCommandBuffers(_vkDevice, _vkCommandPool, _vkPresentQueueCmdBuffers.size(), _vkPresentQueueCmdBuffers.data());
			_vkPresentQueueCmdBuffers.clear();
			vkDestroyCommandPool(_vkDevice, _vkCommandPool, nullptr);
		}
	}
}
void UnInitVulkan()
{
	ClearCommandBuffer();
	vkDeviceWaitIdle(_vkDevice);
	vkDestroyRenderPass(_vkDevice, _vkRenderPass, nullptr);
	vkDestroyPipeline(_vkDevice, _vkGraphicsPipeline,nullptr);
	for (auto f : _vkFrameBuffers)
	{
		vkDestroyFramebuffer(_vkDevice, f, nullptr);
	}

	for (auto v : _vkImageViews)
	{
		vkDestroyImageView(_vkDevice, v, nullptr);
	}
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

