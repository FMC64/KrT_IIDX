
#include <iostream>
#include <exception>

#include "vk.hpp"

QueueFamilies::QueueFamilies()
{
}

void QueueFamilies::updateFamilies(void)
{
	uint32_t count = 0;

	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
	this->families.resize(count);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, this->families.data());
}

QueueFamilies::QueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) : physicalDevice(physicalDevice), surface(surface)
{
	updateFamilies();
}

QueueFamilies::QueueFamilies(Vk &vk) : physicalDevice(vk.physicalDevice), surface(vk.surface)
{
	updateFamilies();
}

QueueFamilies::~QueueFamilies(void)
{
}

VkQueueFlags QueueFamilies::getSupportedFlags(void)
{
	VkQueueFlags res = 0;

	for (auto family : this->families)
		res |= family.queueFlags;
	return res;
}

int QueueFamilies::isPresentationSupported(void)
{
	for (size_t i = 0; i < families.size(); i++)
		if (families[i].queueCount > 0) {
			VkBool32 res;
			vkAssert(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &res));
			if (res)
				return 1;
		}
	return 0;
}

int QueueFamilies::areQueuesSupported(void)
{
	return (this->getSupportedFlags() & VK_QUEUE_GRAPHICS_BIT) && this->isPresentationSupported();
}

uint32_t QueueFamilies::getIndex(VkQueueFlags flags)
{
	for (size_t i = 0; i < this->families.size(); i++)
		if (this->families[i].queueFlags & flags)
			return i;
	throw std::runtime_error("Can't find any queue family matching these flags: " + std::to_string(flags));
}


uint32_t QueueFamilies::getIndexPresent(void)
{
	for (size_t i = 0; i < families.size(); i++)
		if (families[i].queueCount > 0) {
			VkBool32 res;
			vkAssert(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &res));
			if (res)
				return i;
		}
	throw std::runtime_error("Can't find any queue family for presentation");
}
