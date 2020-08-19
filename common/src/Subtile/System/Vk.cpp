#include <iostream>
#include <set>
#include <cstring>
#include <sstream>
#include "Vk.hpp"

namespace Subtile {
namespace System {

Vk::Vk(sb::InstanceBase &instance, const std::string &name, bool isDebug, bool isProfile, const sb::Queue::Set &queues) :
	m_sb_instance(instance),
	m_glfw(GLFW_NO_API, name),
	m_is_debug(isDebug),
	m_is_profile(isProfile),
	m_instance(createInstance()),
	m_debug_messenger(createDebugMessenger()),
	m_surface(createSurface()),
	m_device(createDevice(queues)),
	//m_graphics_queue(m_device.getQueue(*m_device.physical().queues().indexOf(VK_QUEUE_GRAPHICS_BIT), 0)),
	//m_present_queue(m_device.getQueue(*m_device.physical().queues().presentation(), 0)),
	//m_transfer_queue(m_device.getQueue(*m_device.physical().queues().indexOf(VK_QUEUE_TRANSFER_BIT), 0)),
	//m_transfer(m_device, m_transfer_queue),
	m_swapchain_format(m_device.physical().surface().chooseFormat()),
	m_swapchain(createSwapchain()),
	m_default_render_pass(createDefaultRenderPass()),
	m_clear_render_pass(createClearRenderPass()),
	m_swapchain_images(createSwapchainImages()),
	m_swapchain_image_ndx(~0ULL),
	m_acquire_image_semaphore(m_device)
{
}

Vk::~Vk(void)
{
}

void Vk::scanInputs(void)
{
	return m_glfw.scanInputs();
}

const std::map<std::string, System::IInput&>& Vk::getInputs(void)
{
	return m_glfw.getInputs();
}

const VkAllocationCallbacks* Vk::getAllocator(void) const
{
	return nullptr;
}

const std::string& Vk::resultToString(VkResult res)
{
	static const std::map<VkResult, std::string> table {
		{VK_SUCCESS, "VK_SUCCESS"},
		{VK_NOT_READY, "VK_NOT_READY"},
		{VK_TIMEOUT, "VK_TIMEOUT"},
		{VK_EVENT_SET, "VK_EVENT_SET"},
		{VK_EVENT_RESET, "VK_EVENT_RESET"},
		{VK_INCOMPLETE, "VK_INCOMPLETE"},
		{VK_ERROR_OUT_OF_HOST_MEMORY, "VK_ERROR_OUT_OF_HOST_MEMORY"},
		{VK_ERROR_OUT_OF_DEVICE_MEMORY, "VK_ERROR_OUT_OF_DEVICE_MEMORY"},
		{VK_ERROR_INITIALIZATION_FAILED, "VK_ERROR_INITIALIZATION_FAILED"},
		{VK_ERROR_DEVICE_LOST, "VK_ERROR_DEVICE_LOST"},
		{VK_ERROR_MEMORY_MAP_FAILED, "VK_ERROR_MEMORY_MAP_FAILED"},
		{VK_ERROR_LAYER_NOT_PRESENT, "VK_ERROR_LAYER_NOT_PRESENT"},
		{VK_ERROR_EXTENSION_NOT_PRESENT, "VK_ERROR_EXTENSION_NOT_PRESENT"},
		{VK_ERROR_FEATURE_NOT_PRESENT, "VK_ERROR_FEATURE_NOT_PRESENT"},
		{VK_ERROR_INCOMPATIBLE_DRIVER, "VK_ERROR_INCOMPATIBLE_DRIVER"},
		{VK_ERROR_TOO_MANY_OBJECTS, "VK_ERROR_TOO_MANY_OBJECTS"},
		{VK_ERROR_FORMAT_NOT_SUPPORTED, "VK_ERROR_FORMAT_NOT_SUPPORTED"},
		{VK_ERROR_FRAGMENTED_POOL, "VK_ERROR_FRAGMENTED_POOL"},
		{VK_ERROR_UNKNOWN, "VK_ERROR_UNKNOWN"},
		{VK_ERROR_OUT_OF_POOL_MEMORY, "VK_ERROR_OUT_OF_POOL_MEMORY"},
		{VK_ERROR_INVALID_EXTERNAL_HANDLE, "VK_ERROR_INVALID_EXTERNAL_HANDLE"},
		{VK_ERROR_FRAGMENTATION, "VK_ERROR_FRAGMENTATION"},
		{VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS, "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS"},
		{VK_ERROR_SURFACE_LOST_KHR, "VK_ERROR_SURFACE_LOST_KHR"},
		{VK_ERROR_NATIVE_WINDOW_IN_USE_KHR, "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR"},
		{VK_SUBOPTIMAL_KHR, "VK_SUBOPTIMAL_KHR"},
		{VK_ERROR_OUT_OF_DATE_KHR, "VK_SUBOPTIMAL_KHR"},
		{VK_ERROR_INCOMPATIBLE_DISPLAY_KHR, "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR"},
		{VK_ERROR_VALIDATION_FAILED_EXT, "VK_ERROR_VALIDATION_FAILED_EXT"},
		{VK_ERROR_INVALID_SHADER_NV, "VK_ERROR_INVALID_SHADER_NV"},
		{VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT, "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT"},
		{VK_ERROR_NOT_PERMITTED_EXT, "VK_ERROR_NOT_PERMITTED_EXT"},
		{VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT, "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT"}
	};

	return table.at(res);
}

void Vk::assert(VkResult res)
{
	if (res == VK_SUCCESS)
		return;
	throw std::runtime_error(std::string("Vk::assert failed: ") + resultToString(res));
}

Vk::Instance::Instance(Vk &vk, VkInstance instance) :
	Vk::Handle<VkInstance>(instance),
	m_vk(vk)
{
}

template <>
void Vk::Handle<VkInstance>::destroy(VkInstance handle)
{
	vkDestroyInstance(handle, static_cast<Instance&>(*this).m_vk.getAllocator());
}

Vk::PhysicalDevices Vk::Instance::enumerateDevices(Vk::Surface &surface)
{
	return PhysicalDevices(*this, surface);
}

Vk::Instance Vk::createInstance(void)
{
	util::svec layers;
	util::svec exts = m_glfw.getRequiredVkInstanceExts();

	if (m_is_debug) {
		layers.emplace_back("VK_LAYER_KHRONOS_validation");
		exts.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	if (m_is_profile) {
		layers.emplace_back("VK_LAYER_RENDERDOC_Capture");
		layers.emplace_back("VK_LAYER_LUNARG_monitor");
	}

	VkApplicationInfo ai {};

	ai.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	ai.pApplicationName = "SUBTILE® Application";
	ai.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
	ai.pEngineName = "SUBTILE®";
	ai.engineVersion = VK_MAKE_VERSION(0, 0, 0);
	ai.apiVersion = VK_API_VERSION_1_1;

	VkInstanceCreateInfo createInfo {};

	createInfo.pApplicationInfo = &ai;
	auto clayers = layers.c_strs();
	createInfo.enabledLayerCount = clayers.size();
	createInfo.ppEnabledLayerNames = clayers.data();
	auto cexts = exts.c_strs();
	createInfo.enabledExtensionCount = cexts.size();
	createInfo.ppEnabledExtensionNames = cexts.data();

	return Vk::Instance(*this, create<VkInstance>(vkCreateInstance, &createInfo, getAllocator()));
}

Vk::DebugMessenger::DebugMessenger(Instance &instance, VkDebugUtilsMessengerEXT messenger) :
	Instance::Handle<VkDebugUtilsMessengerEXT>(instance, messenger)
{
}

template <>
void Vk::Instance::Handle<VkDebugUtilsMessengerEXT>::destroy(Instance &instance, VkDebugUtilsMessengerEXT handle)
{
	instance.destroy(instance.getProcAddr<PFN_vkDestroyDebugUtilsMessengerEXT>("vkDestroyDebugUtilsMessengerEXT"), handle);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_cb(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void*)
{
	static const std::map<VkDebugUtilsMessageSeverityFlagBitsEXT, std::string> sever_table {
		{VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT, "VERBOSE"},
		{VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT, "INFO"},
		{VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT, "WARNING"},
		{VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT, "ERROR"}
	};

	static const std::map<VkDebugUtilsMessageTypeFlagBitsEXT, std::string> type_table {
		{VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT, "General"},
		{VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT, "Validation"},
		{VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT, "Performance"}
	};

	if (messageSeverity != VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT &&
	messageSeverity != VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
		auto comma = "";
		std::cerr << "[";
		for (auto &p : sever_table)
			if (p.first & messageSeverity) {
				std::cerr << p.second;
				comma = ", ";
			}
		std::cerr << "] -> {";
		comma = "";
		for (auto &p : type_table)
			if (p.first & messageType) {
				std::cerr << p.second;
				comma = ", ";
			}
		std::cerr << "}" << std::endl;

		std::cerr << pCallbackData->pMessage << std::endl;
	}

	return VK_FALSE;
}

std::optional<Vk::DebugMessenger> Vk::createDebugMessenger(void)
{
	if (!m_is_debug)
		return std::nullopt;

	VkDebugUtilsMessengerCreateInfoEXT ci {};
	ci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	ci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	ci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	ci.pfnUserCallback = debug_messenger_cb;

	return m_instance.create<DebugMessenger>(m_instance.getProcAddr<PFN_vkCreateDebugUtilsMessengerEXT>("vkCreateDebugUtilsMessengerEXT"), ci);
}

Vk::Surface::Surface(Instance &instance, VkSurfaceKHR surface) :
	Instance::Handle<VkSurfaceKHR>(instance, surface)
{
}

template <>
void Vk::Instance::Handle<VkSurfaceKHR>::destroy(Vk::Instance &instance, VkSurfaceKHR surface)
{
	instance.destroy(vkDestroySurfaceKHR, surface);
}

Vk::Surface Vk::createSurface(void)
{
	return m_instance.create<Surface>(glfwCreateWindowSurface, m_glfw.getWindow());
}

Vk::PhysicalDevice::PhysicalDevice(VkPhysicalDevice device, Vk::Surface &surface) :
	m_device(device),
	m_surface(surface),
	m_props(get<VkPhysicalDeviceProperties>(vkGetPhysicalDeviceProperties, m_device)),
	m_features(get<VkPhysicalDeviceFeatures>(vkGetPhysicalDeviceFeatures, m_device)),
	m_queue_families(*this),
	m_phys_surface(*this, surface)
{
}

Vk::PhysicalDevice::operator VkPhysicalDevice(void) const
{
	return m_device;
}

Vk::PhysicalDevice::Properties::Properties(const VkPhysicalDeviceProperties &props) :
	VkPhysicalDeviceProperties(props)
{
}

size_t Vk::PhysicalDevice::Properties::getAlignment(sb::Shader::DescriptorType type) const
{
	if (type == sb::Shader::DescriptorType::UniformBuffer)
		return limits.minUniformBufferOffsetAlignment;
	else
		throw std::runtime_error("Can't align such descriptor type");
}

const Vk::PhysicalDevice::Properties& Vk::PhysicalDevice::properties(void) const
{
	return m_props;
}

const VkPhysicalDeviceFeatures& Vk::PhysicalDevice::features(void) const
{
	return m_features;
}

const Vk::PhysicalDevice::Surface& Vk::PhysicalDevice::surface(void) const
{
	return m_phys_surface;
}

bool Vk::PhysicalDevice::getSurfaceSupport(uint32_t queueFamilyIndex) const
{
	return create<VkBool32>(vkGetPhysicalDeviceSurfaceSupportKHR, m_device, queueFamilyIndex, m_surface);
}

const VkPhysicalDeviceFeatures& Vk::PhysicalDevice::requiredFeatures(void)
{
	static const VkPhysicalDeviceFeatures res {
		.geometryShader = true
	};

	return res;
}

bool Vk::PhysicalDevice::isCompetent(const sb::Queue::Set &requiredQueues) const
{
	auto &req = requiredFeatures();
	auto req_arr_size = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);
	for (size_t i = 0; i < req_arr_size; i++)
		if (reinterpret_cast<const VkBool32*>(&req)[i] && !reinterpret_cast<const VkBool32*>(&m_features)[i])
			return false;

	for (auto &qp : requiredQueues)
		if (!m_queue_families.indexOf(qp.first, qp.second.size()))
			return false;
	if (surface().formats().size() == 0)
		return false;
	if (surface().presentModes().size() == 0)
		return false;
	if (!areExtensionsSupported())
		return false;
	return true;
}

const util::svec Vk::PhysicalDevice::required_extensions {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

bool Vk::PhysicalDevice::areExtensionsSupported(void) const
{
	std::map<std::string, bool> avail;
	for (auto &a : enumerate<VkExtensionProperties>(vkEnumerateDeviceExtensionProperties, *this, nullptr))
		avail[a.extensionName] = true;

	for (auto &w : required_extensions)
		if (!avail[w])
			return false;
	return true;
}

size_t Vk::PhysicalDevice::getScore(void) const
{
	size_t res = 0;

	if (m_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		res += 1;
	return res;
}

const Vk::PhysicalDevice::QueueFamilies& Vk::PhysicalDevice::queues(void) const
{
	return m_queue_families;
}

Vk::PhysicalDevice::QueueFamilies::QueueFamilies(Vk::PhysicalDevice &device) :
	m_physical_device(device),
	m_queues(getCollection<VkQueueFamilyProperties>(vkGetPhysicalDeviceQueueFamilyProperties, device))
{
}

const std::vector<VkQueueFamilyProperties>& Vk::PhysicalDevice::QueueFamilies::properties(void) const
{
	return m_queues;
}

std::optional<uint32_t> Vk::PhysicalDevice::QueueFamilies::indexOf(sb::Queue::Flag flags, size_t count) const
{
	VkQueueFlags vkFlags = static_cast<std::underlying_type_t<decltype(flags)>>(flags & ~sb::Queue::Flag::Present);
	bool isPresent = !!(flags & sb::Queue::Flag::Present);
	struct BestAttr {
		uint32_t ndx;
		size_t count;
		size_t flag_count;

		bool isBetterThan(const BestAttr &other, size_t want_count) const
		{
			if (count >= want_count && other.count >= want_count) // irrelevant if derired count is reached on both sides
				return flag_count < other.flag_count; // return most specialized
			else {
				if (count < other.count)
					return false;
				if (count == other.count)
					return flag_count < other.flag_count;
				return true; // more queues than other
			}
		}
	};
	std::optional<BestAttr> best;
	uint32_t n = 0;

	for (auto &q : m_queues) {
		auto ndx = n++;
		auto present_support = m_physical_device.getSurfaceSupport(ndx);
		if (q.queueCount > 0 && q.queueFlags & vkFlags && (isPresent && present_support)) {
			BestAttr attr {ndx, q.queueCount, 0};
			for (size_t i = 0; i < 32; i++)
				if (q.queueFlags & static_cast<uint32_t>(1 << i))
					attr.flag_count++;
			if (present_support)
				attr.flag_count++;

			if (best) {
				if (attr.isBetterThan(*best, count))
					best = attr;
			} else
				best = attr;
		}
	}
	if (best)
		return best->ndx;
	else
		return std::nullopt;
}

Vk::PhysicalDevice::Surface::Surface(PhysicalDevice &device, Vk::Surface &surface) :
	m_capabilities(create<VkSurfaceCapabilitiesKHR>(vkGetPhysicalDeviceSurfaceCapabilitiesKHR, device, surface)),
	m_formats(enumerate<VkSurfaceFormatKHR>(vkGetPhysicalDeviceSurfaceFormatsKHR, device, surface)),
	m_present_modes(enumerate<VkPresentModeKHR>(vkGetPhysicalDeviceSurfacePresentModesKHR, device, surface)),
	m_vk_surface(surface)
{
}

const VkSurfaceCapabilitiesKHR& Vk::PhysicalDevice::Surface::capabilities(void) const
{
	return m_capabilities;
}

const std::vector<VkSurfaceFormatKHR>& Vk::PhysicalDevice::Surface::formats(void) const
{
	return m_formats;
}

const std::vector<VkPresentModeKHR>& Vk::PhysicalDevice::Surface::presentModes(void) const
{
	return m_present_modes;
}

const VkSurfaceFormatKHR& Vk::PhysicalDevice::Surface::chooseFormat(void) const
{
	for (auto &f : m_formats)
		if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
			return f;
	return *m_formats.begin();
}

VkPresentModeKHR Vk::PhysicalDevice::Surface::choosePresentMode(void) const
{
	for (auto &p : m_present_modes)
		if (p == VK_PRESENT_MODE_FIFO_KHR)
			return p;
	return VK_PRESENT_MODE_IMMEDIATE_KHR;
}

VkExtent2D Vk::PhysicalDevice::Surface::chooseExtent(VkExtent2D baseExtent) const
{
	if (m_capabilities.currentExtent.width != UINT32_MAX)
		return m_capabilities.currentExtent;
	else {
		VkExtent2D res = baseExtent;

		res.width = std::clamp(res.width, m_capabilities.minImageExtent.width, m_capabilities.maxImageExtent.width);
		res.height = std::clamp(res.width, m_capabilities.minImageExtent.height, m_capabilities.maxImageExtent.height);

		return res;
	}
}

Vk::PhysicalDevice::Surface::operator VkSurfaceKHR(void) const
{
	return m_vk_surface;
}

Vk::PhysicalDevices::PhysicalDevices(Vk::Instance &instance, Vk::Surface &surface) :
	m_devices(enumerate(instance, surface))
{
}

const Vk::PhysicalDevice& Vk::PhysicalDevices::getBest(const sb::Queue::Set &requiredQueues) const
{
	std::map<size_t, std::reference_wrapper<const PhysicalDevice>> cands;

	for (auto &p : m_devices)
		if (p.isCompetent(requiredQueues))
			cands.emplace(p.getScore(), p);
	if (cands.size() == 0)
		throw std::runtime_error("No compatible GPU found");
	return cands.rbegin()->second.get();
}

std::vector<Vk::PhysicalDevice> Vk::PhysicalDevices::enumerate(Instance &instance, Vk::Surface &surface)
{
	auto devices = Vk::enumerate<VkPhysicalDevice>(vkEnumeratePhysicalDevices, instance);

	std::vector<Vk::PhysicalDevice> res;
	res.reserve(devices.size());

	for (auto &d : devices)
		res.emplace_back(d, surface);
	return res;
}

Vk::Allocator::Allocator(Vk::Device &device) :
	Vk::Handle<VmaAllocator>(create(device)),
	m_device(device)
{
}

Vk::VmaBuffer Vk::Allocator::createBuffer(const VkBufferCreateInfo &bufferCreateInfo, const VmaAllocationCreateInfo &allocationCreateInfo)
{
	VmaAllocation resalloc;
	VkBuffer resbuffer;
	Vk::assert(vmaCreateBuffer(*this, &bufferCreateInfo, &allocationCreateInfo, &resbuffer, &resalloc, nullptr));

	return VmaBuffer(m_device, resbuffer, resalloc);
}

VmaAllocator Vk::Allocator::create(Device &device)
{
	VmaAllocatorCreateInfo createInfo {};

	createInfo.physicalDevice = device.physical();
	createInfo.device = device;

	return Vk::create<VmaAllocator>(vmaCreateAllocator, &createInfo);
}

template <>
void Vk::Handle<VmaAllocator>::destroy(VmaAllocator allocator)
{
	vmaDestroyAllocator(allocator);
}

Vk::Device::Device(Instance &instance, const PhysicalDevice &physicalDevice, const sbQueueFamilyMapping& queueFamilyMapping, const sbQueueMapping &queueMapping, VkDevice device) :
	Vk::Handle<VkDevice>(device),
	m_instance(instance),
	m_physical(physicalDevice),
	m_queue_family_mapping(queueFamilyMapping),
	m_queue_mapping(queueMapping),
	m_allocator(*this),
	m_dynamic_formats(getDynamicFormats())
{
}

VkFormat Vk::Device::sbFormatToVk(sb::Format format) const
{
	static const std::map<sb::Format, VkFormat> table {
		{sb::Format::bgra4_unorm_pack16, VK_FORMAT_B4G4R4A4_UNORM_PACK16},
		{sb::Format::r5g6b5_unorm_pack16, VK_FORMAT_R5G6B5_UNORM_PACK16},
		{sb::Format::a1rgb5_unorm_pack16, VK_FORMAT_A1R5G5B5_UNORM_PACK16},

		{sb::Format::r8_unorm, VK_FORMAT_R8_UNORM},
		{sb::Format::r8_snorm, VK_FORMAT_R8_SNORM},
		{sb::Format::r8_uint, VK_FORMAT_R8_UINT},
		{sb::Format::r8_sint, VK_FORMAT_R8_SINT},

		{sb::Format::rg8_unorm, VK_FORMAT_R8G8_UNORM},
		{sb::Format::rg8_snorm, VK_FORMAT_R8G8_SNORM},
		{sb::Format::rg8_uint, VK_FORMAT_R8G8_UINT},
		{sb::Format::rg8_sint, VK_FORMAT_R8G8_SINT},

		{sb::Format::rgba8_unorm, VK_FORMAT_R8G8B8A8_UNORM},
		{sb::Format::rgba8_snorm, VK_FORMAT_R8G8B8A8_SNORM},
		{sb::Format::rgba8_uint, VK_FORMAT_R8G8B8A8_UINT},
		{sb::Format::rgba8_sint, VK_FORMAT_R8G8B8A8_SINT},
		{sb::Format::rgba8_srgb, VK_FORMAT_R8G8B8A8_SRGB},

		{sb::Format::bgra8_unorm, VK_FORMAT_B8G8R8A8_UNORM},
		{sb::Format::bgra8_srgb, VK_FORMAT_B8G8R8A8_SRGB},

		{sb::Format::abgr8_unorm_pack32, VK_FORMAT_A8B8G8R8_UNORM_PACK32},
		{sb::Format::abgr8_snorm_pack32, VK_FORMAT_A8B8G8R8_SNORM_PACK32},
		{sb::Format::abgr8_uint_pack32, VK_FORMAT_A8B8G8R8_UINT_PACK32},
		{sb::Format::abgr8_sint_pack32, VK_FORMAT_A8B8G8R8_SINT_PACK32},
		{sb::Format::abgr8_srgb_pack32, VK_FORMAT_A8B8G8R8_SRGB_PACK32},

		{sb::Format::a2bgr10_unorm_pack32, VK_FORMAT_A2B10G10R10_UNORM_PACK32},
		{sb::Format::a2bgr10_uint_pack32, VK_FORMAT_A2B10G10R10_UINT_PACK32},

		{sb::Format::r16_uint, VK_FORMAT_R16_UINT},
		{sb::Format::r16_sint, VK_FORMAT_R16_SINT},
		{sb::Format::r16_sfloat, VK_FORMAT_R16_SFLOAT},

		{sb::Format::rg16_uint, VK_FORMAT_R16G16_UINT},
		{sb::Format::rg16_sint, VK_FORMAT_R16G16_SINT},
		{sb::Format::rg16_sfloat, VK_FORMAT_R16G16_SFLOAT},

		{sb::Format::rgba16_uint, VK_FORMAT_R16G16B16A16_UINT},
		{sb::Format::rgba16_sint, VK_FORMAT_R16G16B16A16_SINT},
		{sb::Format::rgba16_sfloat, VK_FORMAT_R16G16B16A16_SFLOAT},

		{sb::Format::r32_uint, VK_FORMAT_R32_UINT},
		{sb::Format::r32_sint, VK_FORMAT_R32_SINT},
		{sb::Format::r32_sfloat, VK_FORMAT_R32_SFLOAT},

		{sb::Format::rg32_uint, VK_FORMAT_R32G32_UINT},
		{sb::Format::rg32_sint, VK_FORMAT_R32G32_SINT},
		{sb::Format::rg32_sfloat, VK_FORMAT_R32G32_SFLOAT},

		{sb::Format::rgba32_uint, VK_FORMAT_R32G32B32A32_UINT},
		{sb::Format::rgba32_sint, VK_FORMAT_R32G32B32A32_SINT},
		{sb::Format::rgba32_sfloat, VK_FORMAT_R32G32B32A32_SFLOAT},

		{sb::Format::d16_unorm, VK_FORMAT_D16_UNORM},
		{sb::Format::d32_sfloat, VK_FORMAT_D32_SFLOAT}
	};

	auto got = table.find(format);
	if (got != table.end())
		return got->second;
	return m_dynamic_formats.at(format);
}

bool Vk::Device::isDepthFormatSupportedSplAtt(VkFormat format)
{
	VkImageFormatProperties props;
	auto res = vkGetPhysicalDeviceImageFormatProperties(m_physical, format,
	VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
	VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
	0, &props);

	return res == VK_SUCCESS;
}

std::map<sb::Format, VkFormat> Vk::Device::getDynamicFormats(void)
{
	std::map<sb::Format, VkFormat> res {
		{sb::Format::d24un_or_32sf_spl_att, VK_FORMAT_UNDEFINED},
		{sb::Format::d32sf_or_24un_spl_att, VK_FORMAT_UNDEFINED},
		{sb::Format::d24un_or_32sf_spl_att_sfb, VK_FORMAT_UNDEFINED},
		{sb::Format::d32sf_or_24un_spl_att_sfb, VK_FORMAT_UNDEFINED},
		{sb::Format::d24un_or_32sf_spl_att_s8_uint, VK_FORMAT_UNDEFINED},
		{sb::Format::d32sf_or_24un_spl_att_s8_uint, VK_FORMAT_UNDEFINED}
	};

	auto x8_d24_unorm = isDepthFormatSupportedSplAtt(VK_FORMAT_X8_D24_UNORM_PACK32);
	auto d32_sfloat = isDepthFormatSupportedSplAtt(VK_FORMAT_D32_SFLOAT);
	auto x8_d24_unorm_s8_uint = isDepthFormatSupportedSplAtt(VK_FORMAT_D24_UNORM_S8_UINT);
	auto d32_sfloat_s8_uint = isDepthFormatSupportedSplAtt(VK_FORMAT_D32_SFLOAT_S8_UINT);

	if (x8_d24_unorm)
		res.at(sb::Format::d24un_or_32sf_spl_att) = VK_FORMAT_X8_D24_UNORM_PACK32;
	else if (d32_sfloat)
		res.at(sb::Format::d24un_or_32sf_spl_att) = VK_FORMAT_D32_SFLOAT;

	if (d32_sfloat)
		res.at(sb::Format::d32sf_or_24un_spl_att) = VK_FORMAT_D32_SFLOAT;
	else if (x8_d24_unorm)
		res.at(sb::Format::d32sf_or_24un_spl_att) = VK_FORMAT_X8_D24_UNORM_PACK32;


	if (x8_d24_unorm_s8_uint)
		res.at(sb::Format::d24un_or_32sf_spl_att_s8_uint) = VK_FORMAT_D24_UNORM_S8_UINT;
	else if (d32_sfloat_s8_uint)
		res.at(sb::Format::d24un_or_32sf_spl_att_s8_uint) = VK_FORMAT_D32_SFLOAT_S8_UINT;

	if (d32_sfloat_s8_uint)
		res.at(sb::Format::d32sf_or_24un_spl_att_s8_uint) = VK_FORMAT_D32_SFLOAT_S8_UINT;
	else if (x8_d24_unorm_s8_uint)
		res.at(sb::Format::d32sf_or_24un_spl_att_s8_uint) = VK_FORMAT_D24_UNORM_S8_UINT;


	res.at(sb::Format::d24un_or_32sf_spl_att_sfb) = res.at(sb::Format::d24un_or_32sf_spl_att);
	if (res.at(sb::Format::d24un_or_32sf_spl_att_sfb) == VK_FORMAT_UNDEFINED)
		res.at(sb::Format::d24un_or_32sf_spl_att_sfb) = res.at(sb::Format::d24un_or_32sf_spl_att_s8_uint);

	res.at(sb::Format::d32sf_or_24un_spl_att_sfb) = res.at(sb::Format::d32sf_or_24un_spl_att);
	if (res.at(sb::Format::d32sf_or_24un_spl_att_sfb) == VK_FORMAT_UNDEFINED)
		res.at(sb::Format::d32sf_or_24un_spl_att_sfb) = res.at(sb::Format::d32sf_or_24un_spl_att_s8_uint);

	return res;
}

template <>
void Vk::Handle<VkDevice>::destroy(VkDevice device)
{
	vkDeviceWaitIdle(device);
	vkDestroyDevice(device, static_cast<Device&>(*this).m_instance.m_vk.getAllocator());
}

const Vk::PhysicalDevice& Vk::Device::physical(void) const
{
	return m_physical;
}

Vk::Allocator& Vk::Device::allocator(void)
{
	return m_allocator;
}

VkQueue Vk::Device::getQueue(sb::Queue::Flag flags, size_t ndx)
{
	auto queue = m_queue_mapping.at(std::make_pair(flags, ndx));
	VkQueue res;
	vkGetDeviceQueue(*this, queue.first, queue.second, &res);
	return res;
}

Vk::Device Vk::createDevice(const sb::Queue::Set &requiredQueues)
{
	auto devs = m_instance.enumerateDevices(m_surface);
	auto &phys = devs.getBest(requiredQueues);

	sbQueueFamilyMapping queueFamilyMapping;
	sbQueueMapping queueMapping;
	struct QueueAlloc {
		size_t count = 0;
		std::vector<float> prios;
		bool is_saturated = false;
	};
	std::map<VkQueueFamilyIndex, QueueAlloc> allocated;

	for (auto &qp : requiredQueues) {
		auto family_ndx = *phys.queues().indexOf(qp.first, qp.second.size());
		auto &alloc = allocated[family_ndx];
		auto &qf_prop = phys.queues().properties().at(family_ndx);
		queueFamilyMapping.emplace(std::piecewise_construct, std::forward_as_tuple(qp.first), std::forward_as_tuple(family_ndx));
		sbQueueIndex n = 0;
		for (auto &prio : qp.second) {
			auto sb_ndx = n++;
			auto vk_ndx = alloc.count++;
			if (!alloc.is_saturated)
				alloc.prios.emplace_back(prio);
			if (alloc.count == qf_prop.queueCount) {
				alloc.count = 0;
				alloc.is_saturated = true;
			}
			queueMapping.emplace(std::piecewise_construct, std::forward_as_tuple(qp.first, sb_ndx), std::forward_as_tuple(family_ndx, vk_ndx));
		}
	}

	std::vector<VkDeviceQueueCreateInfo> queues;
	for (auto &ap : allocated) {
		VkDeviceQueueCreateInfo ci {};
		ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		ci.queueFamilyIndex = ap.first;
		ci.queueCount = ap.second.prios.size();
		ci.pQueuePriorities = ap.second.prios.data();
		queues.emplace_back(ci);
	}

	VkDeviceCreateInfo createInfo {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = queues.size();
	createInfo.pQueueCreateInfos = queues.data();
	createInfo.pEnabledFeatures = &Vk::PhysicalDevice::requiredFeatures();

	auto cexts = PhysicalDevice::required_extensions.c_strs();
	createInfo.enabledExtensionCount = cexts.size();
	createInfo.ppEnabledExtensionNames = cexts.data();

	return Device(m_instance, phys, queueFamilyMapping, queueMapping, Vk::create<VkDevice>(vkCreateDevice, phys, &createInfo, getAllocator()));
}

Vk::Allocation::Allocation(Allocator &allocator, VmaAllocation alloc) :
	Allocator::Handle<VmaAllocation>(allocator, alloc)
{
}

template <>
void Vk::Allocator::Handle<VmaAllocation>::destroy(Allocator &allocator, VmaAllocation alloc)
{
	vmaFreeMemory(allocator, alloc);
}

void* Vk::Allocation::map(void)
{
	void *res;

	Vk::assert(vmaMapMemory(static_cast<Allocator&>(*this), *this, &res));
	return res;
}

void Vk::Allocation::unmap(void)
{
	vmaUnmapMemory(static_cast<Allocator&>(*this), *this);
}

Vk::Buffer::Buffer(Vk::Device &dev, VkBuffer buffer) :
	Device::Handle<VkBuffer>(dev, buffer)
{
}

template <>
void Vk::Device::Handle<VkBuffer>::destroy(Vk::Device &device, VkBuffer buffer)
{
	device.destroy(vkDestroyBuffer, buffer);
}

Vk::VmaBuffer::VmaBuffer(Device &dev, VkBuffer buffer, VmaAllocation allocation) :
	Allocation(dev.allocator(), allocation),
	Buffer(dev, buffer)
{
}

/*Vk::Transfer::Transfer(Device &dev, VkQueue transferQueue) :
	m_dev(dev),
	m_transfer_queue(transferQueue),
	m_staging_buffer_size(32000000),
	m_staging_buffer(createStagingBuffer(m_staging_buffer_size))
{
}

Vk::VmaBuffer Vk::Transfer::createStagingBuffer(size_t size)
{
	std::vector<uint32_t> queues {*m_dev.physical().queues().indexOf(VK_QUEUE_TRANSFER_BIT)};

	VkBufferCreateInfo bci {};
	bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bci.size = size;
	bci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bci.sharingMode = queues.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
	bci.queueFamilyIndexCount = queues.size();
	bci.pQueueFamilyIndices = queues.data();

	VmaAllocationCreateInfo aci {};
	aci.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	return m_dev.allocator().createBuffer(bci, aci);
}

void Vk::Transfer::write(VkBuffer buf, size_t offset, size_t range, const void *data)
{
	if (range == 0)
		return;

	if (range < m_staging_buffer_size)
		write_w_staging(buf, offset, range, data, m_staging_buffer);
	else {
		auto sbuf = createStagingBuffer(range);
		write_w_staging(buf, offset, range, data, sbuf);
	}
}

void Vk::Transfer::write_w_staging(VkBuffer buf, size_t offset, size_t range, const void *data, VmaBuffer &staging)
{
	auto mapped = staging.map();
	std::memcpy(mapped, data, range);
	staging.unmap();

	auto cmd = CommandBuffer::Transfer(m_dev);

	VkBufferCopy r {};
	r.srcOffset = 0;
	r.dstOffset = offset;
	r.size = range;

	vkCmdCopyBuffer(cmd, staging, buf, 1, &r);

	cmd.submit();
	vkQueueWaitIdle(m_transfer_queue);
}*/

template <>
void Vk::Device::Handle<VkImageView>::destroy(Vk::Device &device, VkImageView imageView)
{
	device.destroy(vkDestroyImageView, imageView);
}

VkAttachmentLoadOp Vk::RenderPass::sbLoadOpToVk(sb::Image::LoadOp loadOp)
{
	static const std::map<sb::Image::LoadOp, VkAttachmentLoadOp> table {
		{sb::Image::LoadOp::DontCare, VK_ATTACHMENT_LOAD_OP_DONT_CARE},
		{sb::Image::LoadOp::Load, VK_ATTACHMENT_LOAD_OP_LOAD},
		{sb::Image::LoadOp::Clear, VK_ATTACHMENT_LOAD_OP_CLEAR}
	};

	return table.at(loadOp);
}

VkAttachmentStoreOp Vk::RenderPass::sbStoreOpToVk(sb::Image::StoreOp storeOp)
{
	static const std::map<sb::Image::StoreOp, VkAttachmentStoreOp> table {
		{sb::Image::StoreOp::DontCare, VK_ATTACHMENT_STORE_OP_DONT_CARE},
		{sb::Image::StoreOp::Store, VK_ATTACHMENT_STORE_OP_STORE},
	};

	return table.at(storeOp);
}

Vk::RenderPass::RenderPass(Vk::Device &dev, VkRenderPass renderPass) :
	Device::Handle<VkRenderPass>(dev, renderPass)
{
}

Vk::RenderPass::RenderPass(Vk::Device &dev, sb::rs::RenderPass &res) :
	RenderPass(dev, create(dev, res))
{
}

VkRenderPass Vk::RenderPass::create(Vk::Device &dev, sb::rs::RenderPass &res)
{
	auto layout = res.layout();

	std::vector<VkAttachmentDescription> attachments;
	attachments.reserve(layout.attachments.size());
	for (auto &a : layout.attachments) {
		VkAttachmentDescription att {};
		att.format = dev.sbFormatToVk(a.format);
		att.samples = VK_SAMPLE_COUNT_1_BIT;
		att.loadOp = sbLoadOpToVk(a.loadOp);
		att.storeOp = sbStoreOpToVk(a.storeOp);
		att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		att.initialLayout = sbImageLayoutToVk(a.inLayout);
		att.finalLayout = sbImageLayoutToVk(a.outLayout);
		attachments.emplace_back(att);
	}

	std::vector<VkSubpassDescription> subpasses;
	std::vector<VkAttachmentReference> subpasses_inputs;
	size_t subpasses_inputs_off = 0;
	std::vector<VkAttachmentReference> subpasses_color_atts;
	size_t subpasses_color_atts_off = 0;
	std::vector<VkAttachmentReference> subpasses_resolves;
	std::vector<VkAttachmentReference> subpasses_depth_stencils;
	size_t subpasses_depth_stencils_off = 0;
	std::vector<uint32_t> subpasses_preserve_atts;
	size_t subpasses_preserve_atts_off = 0;
	for (auto &s : layout.subpasses) {
		subpasses_inputs_off += s.inputAttachments.size();
		subpasses_color_atts_off += s.colorAttachments.size();
		if (s.depthStencilAttachment)
			subpasses_depth_stencils_off++;
		subpasses_preserve_atts_off += s.preserveAttachments.size();
	}
	subpasses_inputs.reserve(subpasses_inputs_off);
	subpasses_color_atts.reserve(subpasses_color_atts_off);
	subpasses_resolves.reserve(subpasses_color_atts_off);
	subpasses_depth_stencils.reserve(subpasses_depth_stencils_off);
	subpasses_preserve_atts.reserve(subpasses_preserve_atts_off);
	subpasses_inputs_off = 0;
	subpasses_color_atts_off = 0;
	subpasses_depth_stencils_off = 0;
	subpasses_preserve_atts_off = 0;

	subpasses.reserve(layout.subpasses.size());
	for (auto &s : layout.subpasses) {
		for (auto &in : s.inputAttachments)
			subpasses_inputs.emplace_back(sbAttachmentReferenceToVk(in));
		for (auto &color_att : s.colorAttachments) {
			subpasses_color_atts.emplace_back(sbAttachmentReferenceToVk(color_att));
			if (color_att.resolve)
				subpasses_resolves.emplace_back(sbAttachmentReferenceToVk(*color_att.resolve));
			else
				subpasses_resolves.emplace_back(VkAttachmentReference{VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_UNDEFINED});
		}
		if (s.depthStencilAttachment)
			subpasses_depth_stencils.emplace_back(sbAttachmentReferenceToVk(*s.depthStencilAttachment));
		for (auto &pre : s.preserveAttachments)
			subpasses_preserve_atts.emplace_back(pre);

		VkSubpassDescription sub {};
		sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		sub.inputAttachmentCount = s.inputAttachments.size();
		sub.pInputAttachments = &subpasses_inputs[subpasses_inputs_off];
		sub.colorAttachmentCount = s.colorAttachments.size();
		sub.pColorAttachments = &subpasses_color_atts[subpasses_color_atts_off];
		sub.pResolveAttachments = &subpasses_resolves[subpasses_color_atts_off];
		sub.pDepthStencilAttachment = s.depthStencilAttachment ? &subpasses_depth_stencils[subpasses_depth_stencils_off] : nullptr;
		sub.preserveAttachmentCount = s.preserveAttachments.size();
		sub.pPreserveAttachments = &subpasses_preserve_atts[subpasses_preserve_atts_off];

		subpasses_inputs_off += s.inputAttachments.size();
		subpasses_color_atts_off += s.colorAttachments.size();
		if (s.depthStencilAttachment)
			subpasses_depth_stencils_off++;
		subpasses_preserve_atts_off += s.preserveAttachments.size();
		subpasses.emplace_back(sub);
	}

	std::vector<VkSubpassDependency> dependencies;
	dependencies.reserve(layout.dependencies.size());
	for (auto &d : layout.dependencies) {
		VkSubpassDependency dep {};
		dep.srcSubpass = d.srcSubpass;
		dep.dstSubpass = d.dstSubpass;
		dep.srcStageMask = sbPipelineStageToVk(d.srcStageMask);
		dep.dstStageMask = sbPipelineStageToVk(d.dstStageMask);
		dep.srcAccessMask = sbAccessToVk(d.srcAccessMask);
		dep.dstAccessMask = sbAccessToVk(d.dstAccessMask);
		dep.dependencyFlags = sbDependencyFlagToVk(d.flags);
		dependencies.emplace_back(dep);
	}

	VkRenderPassCreateInfo ci {};
	ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	ci.attachmentCount = attachments.size();
	ci.pAttachments = attachments.data();
	ci.subpassCount = subpasses.size();
	ci.pSubpasses = subpasses.data();
	ci.dependencyCount = dependencies.size();
	ci.pDependencies = dependencies.data();

	return dev.createVk(vkCreateRenderPass, ci);
}

Vk::RenderPass::~RenderPass(void)
{
}

std::unique_ptr<sb::RenderPass> Vk::createRenderPass(sb::rs::RenderPass &renderpass)
{
	return std::make_unique<RenderPass>(m_device, renderpass);
}

Vk::Swapchain::Swapchain(Vk::Device &device, VkSwapchainKHR swapchain) :
	Device::Handle<VkSwapchainKHR>(device, swapchain)
{
}

template <>
void Vk::Device::Handle<VkSwapchainKHR>::destroy(Vk::Device &device, VkSwapchainKHR swapchain)
{
	device.destroy(vkDestroySwapchainKHR, swapchain);
}

Vk::Swapchain Vk::createSwapchain(void)
{
	auto &phys = m_device.physical();
	auto &surface = phys.surface();

	VkSwapchainCreateInfoKHR ci {};
	ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	ci.surface = surface;
	ci.minImageCount = surface.capabilities().minImageCount + 1;
	if (surface.capabilities().maxImageCount > 0)
		ci.minImageCount = std::min(ci.minImageCount, surface.capabilities().maxImageCount);
	ci.imageFormat = m_swapchain_format.format;
	ci.imageColorSpace = m_swapchain_format.colorSpace;
	auto winsize = m_glfw.getWindow().getSize();
	ci.imageExtent = surface.chooseExtent(VkExtent2D{static_cast<uint32_t>(winsize.x), static_cast<uint32_t>(winsize.y)});
	ci.imageArrayLayers = 1;
	ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	//auto &queues = phys.queues();
	//std::set<uint32_t> unique_queues {*queues.indexOf(VK_QUEUE_GRAPHICS_BIT), *queues.presentation()};
	std::set<uint32_t> unique_queues {};
	std::vector<uint32_t> queues_ndx(unique_queues.begin(), unique_queues.end());
	ci.imageSharingMode = queues_ndx.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
	ci.queueFamilyIndexCount = queues_ndx.size();
	ci.pQueueFamilyIndices = queues_ndx.data();
	ci.preTransform = surface.capabilities().currentTransform;
	ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	ci.presentMode = surface.choosePresentMode();
	ci.clipped = VK_TRUE;
	ci.oldSwapchain = VK_NULL_HANDLE;

	return m_device.create<Swapchain>(vkCreateSwapchainKHR, ci);
}

Vk::Swapchain::Image::Image(Vk::Device &dev, VkImageView imageView, VkFramebuffer defaultFramebuffer, VkFramebuffer clearFramebuffer) :
	m_image_view(dev, imageView),
	m_default_framebuffer(dev, defaultFramebuffer),
	m_clear_framebuffer(dev, clearFramebuffer)
{
}

template <>
void Vk::Device::Handle<VkRenderPass>::destroy(Vk::Device &device, VkRenderPass renderPass)
{
	device.destroy(vkDestroyRenderPass, renderPass);
}

template <>
void Vk::Device::Handle<VkFramebuffer>::destroy(Vk::Device &device, VkFramebuffer framebuffer)
{
	device.destroy(vkDestroyFramebuffer, framebuffer);
}

Vk::Framebuffer& Vk::Swapchain::Image::getDefaultFramebuffer(void)
{
	return m_default_framebuffer;
}

Vk::Framebuffer& Vk::Swapchain::Image::getClearFramebuffer(void)
{
	return m_clear_framebuffer;
}

Vk::RenderPass Vk::createDefaultRenderPass(void)
{
	std::vector<VkAttachmentDescription> atts;
	VkAttachmentDescription att {};
	att.format = m_swapchain_format.format;
	att.samples = VK_SAMPLE_COUNT_1_BIT;
	att.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	att.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	att.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	atts.emplace_back(att);

	std::vector<VkAttachmentReference> colorAtts;
	VkAttachmentReference colorAtt;
	colorAtt.attachment = 0;
	colorAtt.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAtts.emplace_back(colorAtt);

	std::vector<VkSubpassDescription> subs;
	VkSubpassDescription sub {};
	sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	sub.colorAttachmentCount = colorAtts.size();
	sub.pColorAttachments = colorAtts.data();
	subs.emplace_back(sub);

	VkRenderPassCreateInfo ci {};
	ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	ci.attachmentCount = atts.size();
	ci.pAttachments = atts.data();
	ci.subpassCount = subs.size();
	ci.pSubpasses = subs.data();
	return m_device.create<RenderPass>(vkCreateRenderPass, ci);
}

Vk::RenderPass Vk::createClearRenderPass(void)
{
	std::vector<VkAttachmentDescription> atts;
	VkAttachmentDescription att {};
	att.format = m_swapchain_format.format;
	att.samples = VK_SAMPLE_COUNT_1_BIT;
	att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	att.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	atts.emplace_back(att);

	std::vector<VkAttachmentReference> colorAtts;
	VkAttachmentReference colorAtt;
	colorAtt.attachment = 0;
	colorAtt.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAtts.emplace_back(colorAtt);

	std::vector<VkSubpassDescription> subs;
	VkSubpassDescription sub {};
	sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	sub.colorAttachmentCount = colorAtts.size();
	sub.pColorAttachments = colorAtts.data();
	subs.emplace_back(sub);

	VkRenderPassCreateInfo ci {};
	ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	ci.attachmentCount = atts.size();
	ci.pAttachments = atts.data();
	ci.subpassCount = subs.size();
	ci.pSubpasses = subs.data();
	return m_device.create<RenderPass>(vkCreateRenderPass, ci);
}

Vk::RenderPass& Vk::getDefaultRenderPass(void)
{
	return m_default_render_pass;
}

std::vector<Vk::Swapchain::Image> Vk::createSwapchainImages(void)
{
	std::vector<Swapchain::Image> res;

	VkImageViewCreateInfo viewCi {};

	viewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCi.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewCi.format = m_device.physical().surface().chooseFormat().format;
	viewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewCi.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	viewCi.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

	VkFramebufferCreateInfo framebufferCi {};

	framebufferCi.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferCi.width = 1600;
	framebufferCi.height = 900;
	framebufferCi.layers = 1;

	for (auto &i : enumerate<VkImage>(vkGetSwapchainImagesKHR, m_device, m_swapchain)) {
		viewCi.image = i;
		auto view = m_device.createVk(vkCreateImageView, viewCi);
		framebufferCi.attachmentCount = 1;
		framebufferCi.pAttachments = &view;

		framebufferCi.renderPass = m_default_render_pass;
		auto default_fb = m_device.createVk(vkCreateFramebuffer, framebufferCi);

		framebufferCi.renderPass = m_clear_render_pass;
		auto clear_fb = m_device.createVk(vkCreateFramebuffer, framebufferCi);

		res.emplace_back(m_device, view, default_fb, clear_fb);
	}
	return res;
}

Vk::Swapchain::Image& Vk::getSwapchainImage(void)
{
	return m_swapchain_images.at(m_swapchain_image_ndx);
}

Vk::Semaphore::Semaphore(Device &dev, VkSemaphore semaphore) :
	Device::Handle<VkSemaphore>(dev, semaphore)
{
}

Vk::Semaphore::Semaphore(Device &dev) :
	Semaphore(dev, create(dev))
{
}

VkSemaphore Vk::Semaphore::create(Vk::Device &dev)
{
	VkSemaphoreCreateInfo ci {};

	ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	return dev.createVk(vkCreateSemaphore, ci);
}

template <>
void Vk::Device::Handle<VkSemaphore>::destroy(Vk::Device &device, VkSemaphore semaphore)
{
	device.destroy(vkDestroySemaphore, semaphore);
}

void Vk::Semaphore::wait(uint64_t value)
{
	VkSemaphore sem = *this;

	VkSemaphoreWaitInfo wi {};
	wi.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
	wi.semaphoreCount = 1;
	wi.pSemaphores = &sem;
	wi.pValues = &value;

	Vk::assert(vkWaitSemaphores(getDep().vk().m_device, &wi, ~0ULL));
}

VkDescriptorType Vk::descriptorType(sb::Shader::DescriptorType type)
{
	static const std::map<sb::Shader::DescriptorType, VkDescriptorType> table {
		{sb::Shader::DescriptorType::UniformBuffer, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
		{sb::Shader::DescriptorType::CombinedImageSampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER}
	};

	return table.at(type);
}

Vk::DescriptorSetLayout::DescriptorSetLayout(Vk::Device &device, const sb::Shader::DescriptorSet::Layout::Description &desc) :
	Device::Handle<VkDescriptorSetLayout>(device, create(device, desc)),
	m_desc(desc)
{
}

Vk::DescriptorSetLayout::~DescriptorSetLayout(void)
{
}

const sb::Shader::DescriptorSet::Layout::Description& Vk::DescriptorSetLayout::getDescription(void) const
{
	return m_desc;
}

VkDescriptorSetLayout Vk::DescriptorSetLayout::create(Device &device, const sb::Shader::DescriptorSet::Layout::Description &desc)
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	for (auto &b : desc)
		bindings.emplace_back(bindingtoVk(b));

	VkDescriptorSetLayoutCreateInfo ci {};
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	ci.bindingCount = bindings.size();
	ci.pBindings = bindings.data();

	if (ci.bindingCount == 0)
		return VK_NULL_HANDLE;
	else
		return device.createVk(vkCreateDescriptorSetLayout, ci);
}

VkDescriptorSetLayoutBinding Vk::DescriptorSetLayout::bindingtoVk(const sb::Shader::DescriptorSet::Layout::DescriptionBinding &b)
{
	VkDescriptorSetLayoutBinding r {};
	r.binding = b.binding;
	r.descriptorType = descriptorType(b.descriptorType);
	r.descriptorCount = b.descriptorCount;
	if (b.isMapped())
		r.descriptorCount = 1;
	for (auto &s : b.stages)
		r.stageFlags |= Vk::Shader::sbStageToVk(s);
	return r;
}

template <>
void Vk::Device::Handle<VkDescriptorSetLayout>::destroy(Vk::Device &device, VkDescriptorSetLayout layout)
{
	device.destroy(vkDestroyDescriptorSetLayout, layout);
}

std::unique_ptr<sb::Shader::DescriptorSet::Layout> Vk::createDescriptorSetLayout(const sb::Shader::DescriptorSet::Layout::Description &desc)
{
	return std::make_unique<DescriptorSetLayout>(m_device, desc);
}

Vk::DescriptorSet::DescriptorSet(Vk::Device &dev, const Vk::DescriptorSetLayout &layout) :
	m_descriptor_pool(dev, createPool(dev, layout)),
	m_descriptor_set(create(dev, layout)),
	m_buffer(createBuffer(dev, layout))
{
	size_t mapped_count = 0;
	for (auto &b : layout.getDescription())
		if (b.isMapped())
			mapped_count++;

	std::vector<VkWriteDescriptorSet> writes;
	writes.reserve(mapped_count);
	std::vector<VkDescriptorBufferInfo> bufferInfos;
	bufferInfos.reserve(mapped_count);

	size_t size = 0;
	for (auto &b : layout.getDescription()) {
		if (!b.isMapped())
			continue;

		VkWriteDescriptorSet w {};

		w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		w.dstSet = m_descriptor_set;
		w.dstBinding = b.binding;
		w.dstArrayElement = 0;
		w.descriptorCount = 1;
		w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		size = util::align_dyn(size, dev.physical().properties().getAlignment(b.descriptorType));

		VkDescriptorBufferInfo buf_info;
		buf_info.buffer = m_buffer;
		buf_info.offset = size;
		buf_info.range = b.descriptorCount;
		w.pBufferInfo = &bufferInfos.emplace_back(buf_info);

		writes.emplace_back(w);
		size += b.descriptorCount;
	}

	if (writes.size() > 0)
		vkUpdateDescriptorSets(dev, writes.size(), writes.data(), 0, nullptr);
}

void Vk::DescriptorSet::write(size_t offset, size_t range, const void *data)
{
	static_cast<void>(offset);
	static_cast<void>(range);
	static_cast<void>(data);
	//m_descriptor_pool.getDep().vk().m_transfer.write(m_buffer, offset, range, data);
}

Vk::DescriptorSet::operator VkDescriptorSet(void) const
{
	return m_descriptor_set;
}

VkDescriptorPool Vk::DescriptorSet::createPool(Device &dev, const DescriptorSetLayout &layout)
{
	std::map<VkDescriptorType, size_t> typeCount;
	for (auto &b : layout.getDescription())
		typeCount[descriptorType(b.descriptorType)] += b.descriptorCount;
	std::vector<VkDescriptorPoolSize> sizes;
	for (auto &t : typeCount) {
		VkDescriptorPoolSize s;
		s.type = t.first;
		s.descriptorCount = t.second;
		sizes.emplace_back(s);
	}

	VkDescriptorPoolCreateInfo ci {};
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	ci.maxSets = 1;
	ci.poolSizeCount = sizes.size();
	ci.pPoolSizes = sizes.data();

	if (ci.poolSizeCount == 0)
		return VK_NULL_HANDLE;
	else
		return dev.createVk(vkCreateDescriptorPool, ci);
}

VkDescriptorSet Vk::DescriptorSet::create(Device &dev, const DescriptorSetLayout &layout)
{
	VkDescriptorSetAllocateInfo ai {};
	ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	ai.descriptorPool = m_descriptor_pool;
	ai.descriptorSetCount = 1;
	ai.pSetLayouts = &static_cast<const VkDescriptorSetLayout&>(layout);

	if (ai.descriptorPool == VK_NULL_HANDLE)
		return VK_NULL_HANDLE;
	else
		return dev.allocateVk(vkAllocateDescriptorSets, ai);
}

Vk::VmaBuffer Vk::DescriptorSet::createBuffer(Device &dev, const DescriptorSetLayout &layout)
{
	//std::vector<uint32_t> queues {*dev.physical().queues().indexOf(VK_QUEUE_GRAPHICS_BIT)};
	std::vector<uint32_t> queues {};
	size_t size = 0;
	for (auto &b : layout.getDescription())
		if (b.isMapped()) {
			size = util::align_dyn(size, dev.physical().properties().getAlignment(b.descriptorType));
			size += b.descriptorCount;
		}

	VkBufferCreateInfo bci {};
	bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bci.size = size;
	bci.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	bci.sharingMode = queues.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
	bci.queueFamilyIndexCount = queues.size();
	bci.pQueueFamilyIndices = queues.data();

	VmaAllocationCreateInfo aci {};
	aci.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	return dev.allocator().createBuffer(bci, aci);
}

Vk::Model::Model(Vk::Device &dev, size_t count, size_t stride, const void *data) :
	m_count(count),
	m_buffer(createBuffer(dev, count * stride))
{
	static_cast<void>(m_count);
	static_cast<void>(data);
	//dev.vk().m_transfer.write(m_buffer, 0, count * stride, data);
}

Vk::VmaBuffer Vk::Model::createBuffer(Device &dev, size_t size)
{
	//std::vector<uint32_t> queues {*dev.physical().queues().indexOf(VK_QUEUE_GRAPHICS_BIT)};
	std::vector<uint32_t> queues {};

	VkBufferCreateInfo bci {};
	bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bci.size = size;
	bci.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	bci.sharingMode = queues.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
	bci.queueFamilyIndexCount = queues.size();
	bci.pQueueFamilyIndices = queues.data();

	VmaAllocationCreateInfo aci {};
	aci.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	if (size == 0)
		return Vk::VmaBuffer(dev, VK_NULL_HANDLE, VK_NULL_HANDLE);
	else
		return dev.allocator().createBuffer(bci, aci);
}

/*void Vk::Model::draw(CommandBuffer &cmd) const
{
	VkBuffer buffer = m_buffer;
	VkDeviceSize off = 0;

	vkCmdBindVertexBuffers(cmd, 0, 1, &buffer, &off);
	vkCmdDraw(cmd, m_count, 1, 0, 0);
}*/

template <>
void Vk::Device::Handle<VkDescriptorPool>::destroy(Vk::Device &device, VkDescriptorPool pool)
{
	device.destroy(vkDestroyDescriptorPool, pool);
}

template <>
void Vk::Device::Handle<VkPipelineLayout>::destroy(Vk::Device &device, VkPipelineLayout pipelineLayout)
{
	device.destroy(vkDestroyPipelineLayout, pipelineLayout);
}

template <>
void Vk::Device::Handle<VkShaderModule>::destroy(Vk::Device &device, VkShaderModule shaderModule)
{
	device.destroy(vkDestroyShaderModule, shaderModule);
}

template <>
void Vk::Device::Handle<VkPipeline>::destroy(Vk::Device &device, VkPipeline pipeline)
{
	device.destroy(vkDestroyPipeline, pipeline);
}

VkShaderStageFlagBits Vk::Shader::sbStageToVk(Subtile::Shader::Stage stage)
{
	static const std::map<Subtile::Shader::Stage, VkShaderStageFlagBits> table {
		{Subtile::Shader::Stage::TesselationControl, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT},
		{Subtile::Shader::Stage::TesselationEvaluation, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT},
		{Subtile::Shader::Stage::Geometry, VK_SHADER_STAGE_GEOMETRY_BIT},
		{Subtile::Shader::Stage::Vertex, VK_SHADER_STAGE_VERTEX_BIT},
		{Subtile::Shader::Stage::Fragment, VK_SHADER_STAGE_FRAGMENT_BIT},
		{Subtile::Shader::Stage::All, VK_SHADER_STAGE_ALL}
	};

	return table.at(stage);
}

VkFormat Vk::Shader::vertexInputFormatToVk(sb::Shader::VertexInput::Format format)
{
	static const std::map<sb::Shader::VertexInput::Format, VkFormat> table {
		{sb::Shader::VertexInput::Format::Bool, VK_FORMAT_R32_UINT},
		{sb::Shader::VertexInput::Format::Int, VK_FORMAT_R32_SINT},
		{sb::Shader::VertexInput::Format::Uint, VK_FORMAT_R32_UINT},
		{sb::Shader::VertexInput::Format::Float, VK_FORMAT_R32_SFLOAT},
		{sb::Shader::VertexInput::Format::Double, VK_FORMAT_R64_SFLOAT},

		{sb::Shader::VertexInput::Format::Vec2, VK_FORMAT_R32G32_SFLOAT},
		{sb::Shader::VertexInput::Format::Vec3, VK_FORMAT_R32G32B32_SFLOAT},
		{sb::Shader::VertexInput::Format::Vec4, VK_FORMAT_R32G32B32A32_SFLOAT},

		{sb::Shader::VertexInput::Format::Bvec2, VK_FORMAT_R32G32_UINT},
		{sb::Shader::VertexInput::Format::Bvec3, VK_FORMAT_R32G32B32_UINT},
		{sb::Shader::VertexInput::Format::Bvec4, VK_FORMAT_R32G32B32A32_UINT},

		{sb::Shader::VertexInput::Format::Ivec2, VK_FORMAT_R32G32_SINT},
		{sb::Shader::VertexInput::Format::Ivec3, VK_FORMAT_R32G32B32_SINT},
		{sb::Shader::VertexInput::Format::Ivec4, VK_FORMAT_R32G32B32A32_SINT},

		{sb::Shader::VertexInput::Format::Uvec2, VK_FORMAT_R32G32_UINT},
		{sb::Shader::VertexInput::Format::Uvec3, VK_FORMAT_R32G32B32_UINT},
		{sb::Shader::VertexInput::Format::Uvec4, VK_FORMAT_R32G32B32A32_UINT},

		{sb::Shader::VertexInput::Format::Dvec2, VK_FORMAT_R64G64_SFLOAT},
		{sb::Shader::VertexInput::Format::Dvec3, VK_FORMAT_R64G64B64_SFLOAT},
		{sb::Shader::VertexInput::Format::Dvec4, VK_FORMAT_R64G64B64A64_SFLOAT}
	};

	return table.at(format);
}

Vk::Shader::Shader(Vk::Device &device, rs::Shader &shader) :
	m_device(device),
	m_layouts(shader.loadDescriptorSetLayouts(device.vk().m_sb_instance)),
	m_pipeline_layout(shader.isModule() ? PipelineLayout(device, VK_NULL_HANDLE) : createPipelineLayout()),
	m_shader_modules(shader.isModule() ? ShaderModulesType{} : createShaderModules(device, shader)),
	m_pipeline(shader.isModule() ? Pipeline(device, VK_NULL_HANDLE) : createPipeline(device, shader))
{
	static_cast<void>(shader);
}

Vk::PipelineLayout Vk::Shader::createPipelineLayout(void)
{
	std::vector<VkDescriptorSetLayout> layouts;

	for (auto &l : m_layouts)
		layouts.emplace_back(reinterpret_cast<const DescriptorSetLayout&>(l.get()->resolve()));

	VkPipelineLayoutCreateInfo ci {};
	ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	ci.setLayoutCount = layouts.size();
	ci.pSetLayouts = layouts.data();
	return m_device.create<PipelineLayout>(vkCreatePipelineLayout, ci);
}

std::vector<std::pair<VkShaderStageFlagBits, Vk::ShaderModule>> Vk::Shader::createShaderModules(Vk::Device &device, rs::Shader &shader)
{
	std::vector<std::pair<VkShaderStageFlagBits, Vk::ShaderModule>> res;

	for (auto &sp : shader.getStages()) {
		std::stringstream ss;
		ss << sp.second.getVk().getCompiled().read().rdbuf();
		auto bin = ss.str();

		VkShaderModuleCreateInfo ci {};
		ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		ci.codeSize = bin.size();
		ci.pCode = reinterpret_cast<uint32_t*>(bin.data());

		res.emplace_back(sbStageToVk(sp.first), device.create<ShaderModule>(vkCreateShaderModule, ci));
	}
	return res;
}

Vk::Pipeline Vk::Shader::createPipeline(Vk::Device &device, rs::Shader &shader)
{
	std::vector<VkPipelineShaderStageCreateInfo> stages;
	for (auto &sp : m_shader_modules) {
		VkPipelineShaderStageCreateInfo ci {};
		ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		ci.stage = sp.first;
		ci.module = sp.second;
		ci.pName = "main";
		stages.emplace_back(ci);
	}

	auto rsVertexInput = shader.vertexInput();
	std::vector<VkVertexInputBindingDescription> vertexBindings;
	VkVertexInputBindingDescription vertexBinding;
	vertexBinding.binding = 0;
	vertexBinding.stride = rsVertexInput.size;
	vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vertexBindings.emplace_back(vertexBinding);
	std::vector<VkVertexInputAttributeDescription> vertexAttributes;
	for (auto &a : rsVertexInput.attributes) {
		VkVertexInputAttributeDescription attr;
		attr.location = a.location;
		attr.binding = 0;
		attr.format = vertexInputFormatToVk(a.format);
		attr.offset = a.offset;
		vertexAttributes.emplace_back(attr);
	}
	VkPipelineVertexInputStateCreateInfo vertexInput {};
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInput.vertexBindingDescriptionCount = vertexBindings.size();
	vertexInput.pVertexBindingDescriptions = vertexBindings.data();
	vertexInput.vertexAttributeDescriptionCount = vertexAttributes.size();
	vertexInput.pVertexAttributeDescriptions = vertexAttributes.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	std::vector<VkViewport> viewports {	// x, y, w, h, minDepth, maxDepth
		{0.0f, 0.0f, 1600.0f, 900.0f, 0.0f, 1.0f}	// NOTE: temporary, use dynamic states
	};
	std::vector<VkRect2D> scissors {
		{{0, 0}, {1600, 900}}	// NOTE: temporary, use dynamic states
	};
	VkPipelineViewportStateCreateInfo viewport {};
	viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport.viewportCount = viewports.size();
	viewport.pViewports = viewports.data();
	viewport.scissorCount = scissors.size();
	viewport.pScissors = scissors.data();

	VkPipelineRasterizationStateCreateInfo rasterization {};
	rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterization.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisample {};
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
	VkPipelineColorBlendAttachmentState colorBlendAttachment {};
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachments.emplace_back(colorBlendAttachment);
	VkPipelineColorBlendStateCreateInfo colorBlend {};
	colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlend.attachmentCount = colorBlendAttachments.size();
	colorBlend.pAttachments = colorBlendAttachments.data();
	for (size_t i = 0; i < 4; i++)
		colorBlend.blendConstants[i] = 1.0f;

	VkGraphicsPipelineCreateInfo ci {};
	ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	ci.stageCount = stages.size();
	ci.pStages = stages.data();
	ci.pVertexInputState = &vertexInput;
	ci.pInputAssemblyState = &inputAssembly;
	ci.pViewportState = &viewport;
	ci.pRasterizationState = &rasterization;
	ci.pMultisampleState = &multisample;
	ci.pColorBlendState = &colorBlend;
	ci.layout = m_pipeline_layout;
	ci.renderPass = device.vk().getDefaultRenderPass();
	ci.subpass = 0;

	return device.create<Pipeline>(vkCreateGraphicsPipelines, ci);
}

const sb::Shader::DescriptorSet::Layout& Vk::Shader::setLayout(size_t ndx)
{
	return m_layouts.at(ndx).get()->resolve();
}

std::unique_ptr<sb::Shader::DescriptorSet> Vk::Shader::set(size_t ndx)
{
	return std::make_unique<Vk::DescriptorSet>(m_device, reinterpret_cast<const DescriptorSetLayout&>(m_layouts.at(ndx)->resolve()));
}

std::unique_ptr<sb::Shader::Model> Vk::Shader::model(size_t count, size_t stride, const void *data)
{
	return std::make_unique<Vk::Model>(m_device, count, stride, data);
}

Vk::PipelineLayout& Vk::Shader::getPipelineLayout(void)
{
	return m_pipeline_layout;
}

Vk::Pipeline& Vk::Shader::getPipeline(void)
{
	return m_pipeline;
}

std::unique_ptr<sb::Shader> Vk::createShader(rs::Shader &shader)
{
	return std::make_unique<Shader>(m_device, shader);
}

/*Vk::CommandBuffer::CommandBuffer(Vk::Device &dev, uint32_t queue_type) :
	m_command_pool(dev, createPool(dev, queue_type)),
	m_command_buffer(allocCommandBuffer(dev))
{
	beginCommandBuffer();
}

Vk::CommandBuffer Vk::CommandBuffer::Graphics(Device &dev)
{
	//return CommandBuffer(dev, *dev.physical().queues().indexOf(VK_QUEUE_GRAPHICS_BIT));
	return CommandBuffer(dev, 0);
}

Vk::CommandBuffer Vk::CommandBuffer::Present(Device &dev)
{
	//return CommandBuffer(dev, *dev.physical().queues().presentation());
	return CommandBuffer(dev, 0);
}

Vk::CommandBuffer Vk::CommandBuffer::Transfer(Device &dev)
{
	//return CommandBuffer(dev, *dev.physical().queues().indexOf(VK_QUEUE_TRANSFER_BIT));
	return CommandBuffer(dev, 0);
}

VkCommandPool Vk::CommandBuffer::createPool(Device &dev, uint32_t queueIndex)
{
	VkCommandPoolCreateInfo ci {};

	ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	ci.queueFamilyIndex = queueIndex;

	return dev.createVk(vkCreateCommandPool, ci);
}

VkCommandBuffer Vk::CommandBuffer::allocCommandBuffer(Vk::Device &dev)
{
	VkCommandBufferAllocateInfo ai {};

	ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	ai.commandPool = m_command_pool;
	ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	ai.commandBufferCount = 1;

	return dev.allocateVk(vkAllocateCommandBuffers, ai);
}

void Vk::CommandBuffer::beginCommandBuffer(void)
{
	VkCommandBufferBeginInfo be {};

	be.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	be.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	Vk::assert(vkBeginCommandBuffer(m_command_buffer, &be));
}

Vk::CommandBuffer::~CommandBuffer(void)
{
}

template <>
void Vk::Device::Handle<VkCommandPool>::destroy(Vk::Device &device, VkCommandPool pool)
{
	device.destroy(vkDestroyCommandPool, pool);
}

VkCommandBuffer Vk::CommandBuffer::getHandle(void) const
{
	return m_command_buffer;
}

Vk::CommandBuffer::operator VkCommandBuffer(void) const
{
	return m_command_buffer;
}

void Vk::CommandBuffer::beginRenderPass(void)
{
	auto &vk = m_command_pool.getDep().vk();

	VkRenderPassBeginInfo bi {};

	bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	bi.renderPass = vk.m_default_render_pass;
	bi.framebuffer = vk.getSwapchainImage().getDefaultFramebuffer();
	bi.renderArea = {{0, 0}, {1600, 900}};

	vkCmdBeginRenderPass(m_command_buffer, &bi, VK_SUBPASS_CONTENTS_INLINE);
}

void Vk::CommandBuffer::endRenderPass(void)
{
	vkCmdEndRenderPass(m_command_buffer);
}

void Vk::CommandBuffer::bindShader(sb::Shader &shader)
{
	auto &sha = reinterpret_cast<Vk::Shader&>(shader);

	vkCmdBindPipeline(m_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, sha.getPipeline());
}

void Vk::CommandBuffer::bindDescriptorSet(sb::Shader &shader, sb::Shader::DescriptorSet &set, size_t ndx)
{
	auto &sha = reinterpret_cast<Vk::Shader&>(shader);
	auto &s = reinterpret_cast<Vk::DescriptorSet&>(set);

	VkDescriptorSet desc_set = s;
	vkCmdBindDescriptorSets(m_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, sha.getPipelineLayout(), ndx, 1, &desc_set, 0, nullptr);
}

void Vk::CommandBuffer::draw(const sb::Shader::Model &model)
{
	auto &mod = reinterpret_cast<const Vk::Model&>(model);

	mod.draw(*this);
}

void Vk::CommandBuffer::submit(void)
{
	Vk::assert(vkEndCommandBuffer(m_command_buffer));

	VkSubmitInfo s {};
	s.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	s.commandBufferCount = 1;
	s.pCommandBuffers = &m_command_buffer;

	Vk::assert(vkQueueSubmit(m_command_pool.getDep().vk().m_graphics_queue, 1, &s, VK_NULL_HANDLE));
	Vk::assert(vkQueueWaitIdle(m_command_pool.getDep().vk().m_graphics_queue));
}

std::unique_ptr<sb::Render::CommandBuffer> Vk::createRenderCommandBuffer(void)
{
	return std::make_unique<CommandBuffer>(CommandBuffer::Graphics(m_device));
}*/

void Vk::acquireNextImage(void)
{
	/*uint32_t ndx;

	Vk::assert(vkAcquireNextImageKHR(m_device, m_swapchain, ~0ULL, m_acquire_image_semaphore, VK_NULL_HANDLE, &ndx));
	m_swapchain_image_ndx = ndx;

	auto cmd = CommandBuffer::Present(m_device);
	VkCommandBuffer cmd_handle = cmd;
	Vk::assert(vkEndCommandBuffer(cmd));
	VkSemaphore sem = m_acquire_image_semaphore;
	VkPipelineStageFlags dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo s {};
	s.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	s.waitSemaphoreCount = 1;
	s.pWaitSemaphores = &sem;
	s.pWaitDstStageMask = &dst_stage_mask;
	s.commandBufferCount = 1;
	s.pCommandBuffers = &cmd_handle;
	Vk::assert(vkQueueSubmit(m_present_queue, 1, &s, VK_NULL_HANDLE));
	Vk::assert(vkQueueWaitIdle(m_present_queue));

	auto cmd2 = CommandBuffer::Graphics(m_device);
	VkClearValue cv;
	for (size_t i = 0; i < 4; i++)
		cv.color.float32[i] = 0.0f;
	cv.color.float32[0] = 44.0f / 255.0f;
	cv.color.float32[1] = 99.0f / 255.0f;
	cv.color.float32[2] = 59.0f / 255.0f;
	for (size_t i = 0; i < 4; i++)
		cv.color.float32[i] = std::pow(cv.color.float32[i], 2.2f);	// srgb to linear

	VkRenderPassBeginInfo bi {};
	bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	bi.renderPass = m_clear_render_pass;
	bi.framebuffer = getSwapchainImage().getClearFramebuffer();
	bi.renderArea = {{0, 0}, {1600, 900}};
	bi.clearValueCount = 1;
	bi.pClearValues = &cv;
	vkCmdBeginRenderPass(cmd2, &bi, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdEndRenderPass(cmd2);

	VkCommandBuffer cmd_handle2 = cmd2;
	Vk::assert(vkEndCommandBuffer(cmd_handle2));

	VkSubmitInfo s2 {};
	s2.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	//s2.waitSemaphoreCount = 1;
	//s2.pWaitSemaphores = &sem;
	s2.pWaitDstStageMask = &dst_stage_mask;
	s2.commandBufferCount = 1;
	s2.pCommandBuffers = &cmd_handle2;
	Vk::assert(vkQueueSubmit(m_graphics_queue, 1, &s2, VK_NULL_HANDLE));
	Vk::assert(vkQueueWaitIdle(m_graphics_queue));*/
}

void Vk::presentImage(void)
{
	/*VkSwapchainKHR swapchain = m_swapchain;
	uint32_t ndx = m_swapchain_image_ndx;

	VkPresentInfoKHR pi {};
	pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	pi.swapchainCount = 1;
	pi.pSwapchains = &swapchain;
	pi.pImageIndices = &ndx;

	Vk::assert(vkQueuePresentKHR(m_present_queue, &pi));
	Vk::assert(vkQueueWaitIdle(m_present_queue));*/
}

Vk::Queue::Queue(VkQueue queue) :
	m_handle(queue)
{
}

Vk::Queue::~Queue(void)
{
}

Vk::Queue::operator VkQueue(void) const
{
	return m_handle;
}

std::unique_ptr<sb::Queue> Vk::getQueue(sb::Queue::Flag flags, size_t index)
{
	return std::make_unique<Queue>(m_device.getQueue(flags, index));
}

}
}