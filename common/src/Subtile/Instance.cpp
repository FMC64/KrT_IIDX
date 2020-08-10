#include "Instance.hpp"
#include "System/Input/IButton.hpp"
#include "System/Input/IKeyboard.hpp"
#include "System/Vk.hpp"

namespace Subtile {

Instance::Instance(bool isDebug, const std::string &name) :
	m_system(new System::Vk(*this, isDebug, name)),
	m_events(*m_system)
{
}

Instance::~Instance(void)
{
}

void Instance::setInputs(const std::function<void (const Event::System::Observer::Input::Setter &setter)> &binder)
{
	return m_events.input.set(binder);
}

ISystem& Instance::system(void)
{
	return *m_system;
}

Shader::UniqueRef Instance::loadShaderRef(rs::Shader &shaderres)
{
	auto got = m_shaders.find(shaderres);
	if (got == m_shaders.end())
		return m_shaders.emplace(shaderres, m_system->loadShader(shaderres));
	else
		return got->second.new_ref();
}

}