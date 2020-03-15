#include "Device.hpp"

namespace Subtile {
namespace Vk {
namespace Dep {

Device::Device(VkDevice device) :
	m_device(device)
{
}

VkDevice Device::getDevice(void) const
{
	return m_device;
}

}
}
}