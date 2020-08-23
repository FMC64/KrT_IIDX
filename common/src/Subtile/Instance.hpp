#pragma once

#include <memory>
#include "../Subtile.hpp"
#include "System.hpp"
#include "Event/System/Observer.hpp"
#include "Shader.hpp"
#include "Image.hpp"
#include "Queue.hpp"

namespace Subtile {

class WorldBase;
class SessionBase;
namespace Render {
class Pass;
}

class InstanceBase
{
	Shader::Cache m_shaders;
	RenderPass::Cache m_render_passes;

public:
	InstanceBase(const std::string &name, bool isDebug, bool isProfile, const sb::Queue::Set &queues);
	~InstanceBase(void);

	void setInputs(const std::function<void (const Event::System::Observer::Input::Setter &setter)> &binder);

	class Getter
	{
	public:
		Getter(InstanceBase &ins) :
			m_ins(ins)
		{
		}

		auto& system(void)
		{
			return *m_ins.m_system;
		}
		auto& system(void) const
		{
			return *m_ins.m_system;
		}

		template <typename ...Args>
		decltype(auto) loadShader(Args &&...args)
		{
			return m_ins.loadShader(std::forward<Args>(args)...);
		}

		template <typename ...Args>
		decltype(auto) loadRenderPassRef(Args &&...args)
		{
			return m_ins.loadRenderPassRef(std::forward<Args>(args)...);
		}

	private:
		InstanceBase &m_ins;
	};

private:
	friend WorldBase;
	friend SessionBase;
	friend Render::Pass;
	friend Getter;

	std::unique_ptr<System> m_system;
	Event::System::Observer m_events;

	System& system(void);

	Shader::Cache::Ref loadShaderRef(rs::Shader &shaderres);
	RenderPass::Cache::Ref loadRenderPassRef(rs::RenderPass &renderpassres);

	template <typename S>
	decltype(auto) loadShader(S &&shaderres)
	{
		return Shader::Loaded<std::remove_cv_t<std::remove_reference_t<S>>>(loadShaderRef(std::forward<S>(shaderres)));
	}
	template <typename R>
	decltype(auto) loadRenderPass(R &&renderpassres)
	{
		return RenderPass::Loaded<std::remove_cv_t<std::remove_reference_t<R>>>(loadRenderPassRef(std::forward<R>(renderpassres)));
	}


public:
	template <typename ResType>
	decltype(auto) load(ResType &&res)
	{
		static_cast<void>(res);

		if constexpr (std::is_base_of_v<rs::Shader, std::remove_reference_t<ResType>>) {
			return loadShader(std::forward<ResType>(res));
		} else if constexpr (std::is_base_of_v<rs::RenderPass, std::remove_reference_t<ResType>>) {
			return loadRenderPass(std::forward<ResType>(res));
		} else
			static_assert(!std::is_same_v<ResType, ResType>, "Unsupported resource type");
	}

	template <sb::Queue::Flag Flags>
	auto queue(size_t index)
	{
		return Queue::Handle<Flags>(system().getQueue(Flags, index));
	}

	template <sb::Queue::Flag Q>
	auto image2D(Format format, Image::Sample sampleCount, svec2 extent, Image::Usage usage, sb::Queue::Handle<Q> &queue)
	{
		using Getter = typename sb::Queue::Handle<Q>::Getter;
		return Image2D(system().createImage(Image::Type::Image2D, format, sampleCount, svec3(extent.x, extent.y, 1), 1, usage, Getter().get(queue)));
	}

	template <sb::Queue::Flag Q>
	auto image2DArray(Format format, Image::Sample sampleCount, svec2 extent, size_t layers, Image::Usage usage, sb::Queue::Handle<Q> &queue)
	{
		using Getter = typename sb::Queue::Handle<Q>::Getter;
		return Image2DArray(system().createImage(Image::Type::Image2DArray, format, sampleCount, svec3(extent.x, extent.y, 1), layers, usage, Getter().get(queue)));
	}
};

template <typename ShaderRes>
class Shader::DescriptorSet::Layout::Resolver::Foreign : public Shader::DescriptorSet::Layout::Resolver
{
public:
	Foreign(InstanceBase &instance, ShaderRes &shaderres, size_t set_ndx) :
		m_loaded(InstanceBase::Getter(instance).loadShader(shaderres)),
		m_layout((**RefGetter<CacheRefHolder>(m_loaded).get()).setLayout(set_ndx))
	{
	}
	~Foreign(void) override
	{
	}

	const Layout& resolve(void) const override
	{
		return m_layout;
	}

private:
	Shader::Loaded<ShaderRes> m_loaded;
	const Layout &m_layout;
};

}

#include "Session.hpp"
#include "World.hpp"

namespace Subtile {

template <typename InstanceType>
class Instance : public InstanceBase
{
public:
	Instance(const std::string &name, bool isDebug, bool isProfile, const sb::Queue::Set &queues) :
		InstanceBase(name, isDebug, isProfile, queues)
	{
	}

	template <typename WorldType>
	using World = sb::World<InstanceType, WorldType>;
	using Session = sb::Session<InstanceType>;

	template <typename Type, typename ...Args>
	decltype(auto) create(Args &&...args)
	{
		if constexpr (std::is_base_of_v<SessionBase, Type>)
			return createSession<Type>(std::forward<Args>(args)...);
		else if constexpr (std::is_base_of_v<WorldBase, Type>)
			return createWorld<Type>(std::forward<Args>(args)...);
		else
			static_assert(!std::is_same_v<Type, Type>, "Unknown primitive type to create");
	}

private:
	template <typename SessionType, typename ...ArgsTypes>
	std::unique_ptr<SessionType> createSession(ArgsTypes &&...args)
	{
		static_assert(std::is_base_of_v<typename SessionType::instance_type, InstanceType>, "Incompatible session");

		auto res = SessionBase::getCtx().emplace_frame(std::function([&](){
			return std::make_unique<SessionType>(std::forward<ArgsTypes>(args)...);
		}), *this);
		SessionBase::getSessionStack().pop();
		return res;
	}

	template <typename WorldType, typename ...ArgsTypes>
	std::unique_ptr<WorldType> createWorld(ArgsTypes &&...args)
	{
		static_assert(std::is_base_of_v<typename WorldType::instance_type, InstanceType>, "Incompatible world");

		auto res = WorldBase::getInstanceStack().emplace_frame(std::function([&]() -> auto {
			return EntityBase::getCtx().emplace_frame(std::function([&]() -> auto {
				return std::make_unique<WorldType>(std::forward<ArgsTypes>(args)...);
			}), nullptr, nullptr);
		}), *this);
		EntityBase::getEntityStack().pop();
		return res;
	}
};

}