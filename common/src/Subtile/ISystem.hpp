#pragma once

#include <vector>
#include <string>
#include <map>
#include "../Subtile.hpp"
#include "System/IInput.hpp"
#include "Shader.hpp"
#include "Queue.hpp"

namespace Subtile {

class ISystem
{
public:
	virtual ~ISystem(void) = default;

	virtual void scanInputs(void) = 0;
	virtual const std::map<std::string, System::IInput&>& getInputs(void) = 0;

	virtual std::unique_ptr<RenderPass> createRenderPass(rs::RenderPass &renderpass) = 0;
	virtual std::unique_ptr<Shader> createShader(rs::Shader &shader) = 0;
	virtual std::unique_ptr<Shader::DescriptorSet::Layout> createDescriptorSetLayout(const Shader::DescriptorSet::Layout::Description &desc) = 0;

	virtual std::unique_ptr<Queue> getQueue(Queue::Flag flags, size_t index) = 0;

	virtual void acquireNextImage(void) = 0;
	virtual void presentImage(void) = 0;
};

}