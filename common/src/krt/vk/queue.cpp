
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

QueueFamilies::QueueFamilies(Vk &vk) :
	physicalDevice(vk.device.physicalDevice),
	surface(vk.context.surface)
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
	VkQueueFlags supported = getSupportedFlags();

	return (supported & VK_QUEUE_GRAPHICS_BIT) && (supported & VK_QUEUE_TRANSFER_BIT) && isPresentationSupported();
}

#include <optional>
#include <bitset>

uint32_t QueueFamilies::getIndex(VkQueueFlags flags)
{
	std::optional<uint32_t> res;
	std::optional<uint32_t> bestCount;

	for (size_t i = 0; i < this->families.size(); i++) {
		if (this->families[i].queueFlags & flags) {
			size_t bitCount = std::bitset<32>(this->families[i].queueFlags).count();
			if (!bestCount.has_value() || (bitCount < bestCount)) {
				res = i;
				bestCount = bitCount;
			}
		}
	}
	if (!res.has_value())
		throw std::runtime_error("Can't find any queue family matching these flags: " + std::to_string(flags));
	return *res;
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

Queues::Queues(Vk &vk)
{
	present = vk.device.getQueue(vk.device.queueFamilies.getIndexPresent(), 0);
	graphics = vk.device.getQueue(vk.device.queueFamilies.getIndex(VK_QUEUE_GRAPHICS_BIT), 0);
	transfer = vk.device.getQueue(vk.device.queueFamilies.getIndex(VK_QUEUE_TRANSFER_BIT), 0);
}

Queues::~Queues(void)
{
}
