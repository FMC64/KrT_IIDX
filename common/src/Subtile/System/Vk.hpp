#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include "../System.hpp"
#include "Glfw.hpp"
#include "Subtile/Shader.hpp"
#include "Subtile/RenderList.hpp"
#include "Subtile/Model.hpp"

#undef assert

namespace Subtile {

class InstanceBase;

class Vk : public System
{
public:
	Vk(InstanceBase &instance, const std::string &name, bool isDebug, bool isProfile, const sb::Queue::Set &queues);
	~Vk(void) override;

	void scanInputs(void) override;
	const std::map<std::string, System::Input&>& getInputs(void) override;

	const VkAllocationCallbacks* getAllocator(void) const;

	InstanceBase &m_sb_instance;

private:
	Glfw m_glfw;
	bool m_is_debug;
	bool m_is_profile;

	static const std::string& resultToString(VkResult res);
	static void assert(VkResult res);

	template <typename T>
	static std::vector<T> enumerateAbstract(VkResult (*callable)(uint32_t*, T*))
	{
		uint32_t size;

		assert(callable(&size, nullptr));
		std::vector<T> res(size);
		assert(callable(&size, res.data()));
		return res;
	}

	template <typename T, typename C, typename ...Args>
	static std::vector<T> enumerate(C &&callable, Args &&...args)
	{
		uint32_t size;

		assert(callable(std::forward<Args>(args)..., &size, nullptr));
		std::vector<T> res(size);
		assert(callable(std::forward<Args>(args)..., &size, res.data()));
		return res;
	}

	template <typename T, typename C, typename ...Args>
	static T create(C &&callable, Args &&...args)
	{
		T res;
		assert(callable(std::forward<Args>(args)..., &res));
		return res;
	}

	template <typename T, typename C, typename ...Args>
	static T get(C &&callable, Args &&...args)
	{
		T res;
		callable(std::forward<Args>(args)..., &res);
		return res;
	}

	template <typename T, typename C, typename ...Args>
	static std::vector<T> getCollection(C &&callable, Args &&...args)
	{
		uint32_t size;

		callable(std::forward<Args>(args)..., &size, nullptr);
		std::vector<T> res(size);
		callable(std::forward<Args>(args)..., &size, res.data());
		return res;
	}

	template <typename VkHandle>
	class Handle
	{
		void destroy(VkHandle handle);

	public:
		Handle(VkHandle handle) :
			m_handle(handle)
		{
		}

		Handle(const Handle&) = delete;

		Handle(Handle &&other) :
			m_handle(other.m_handle)
		{
			other.m_handle = VK_NULL_HANDLE;
		}

		~Handle(void)
		{
			if (m_handle != VK_NULL_HANDLE)
				destroy(m_handle);
		}

		operator const VkHandle&(void) const
		{
			return m_handle;
		}

	protected:
		VkHandle m_handle;
	};

	template <typename DepType, typename VkHandle>
	class HandleDep
	{
		void destroy(DepType &dep, VkHandle handle);

	public:
		HandleDep(DepType &dep, VkHandle handle) :
			m_dep(dep),
			m_handle(handle)
		{
		}

		HandleDep(const HandleDep&) = delete;

		HandleDep(HandleDep &&other) :
			m_dep(other.m_dep),
			m_handle(other.m_handle)
		{
			other.m_handle = VK_NULL_HANDLE;
		}

		~HandleDep(void)
		{
			if (m_handle != VK_NULL_HANDLE)
				destroy(m_dep, m_handle);
		}

		operator const VkHandle&(void) const
		{
			return m_handle;
		}

		VkHandle getHandle(void) const
		{
			return m_handle;
		}

		DepType& getDep(void) const
		{
			return m_dep;
		}

	private:
		DepType &m_dep;

	protected:
		VkHandle m_handle;

		operator DepType&(void)
		{
			return m_dep;
		}

		operator const DepType&(void) const
		{
			return m_dep;
		}
	};

	class PhysicalDevices;
	class Surface;
	class Device;

	class Instance : public Handle<VkInstance>
	{
		friend Handle<VkInstance>;
		friend Handle<VkDevice>;
		friend Device;

	public:
		Instance(Vk &vk, VkInstance instance);

		template <typename FunType>
		FunType getProcAddr(const char *name) const
		{
			auto res = vkGetInstanceProcAddr(m_handle, name);

			if (res == nullptr)
				throw std::runtime_error(std::string("Can't get proc '") + std::string(name) + std::string("'"));
			return reinterpret_cast<FunType>(res);
		}

		template <typename VkHandle>
		using Handle = HandleDep<Instance, VkHandle>;

		PhysicalDevices enumerateDevices(Vk::Surface &surface);

		template <typename T, typename C>
		auto createVk(VkResult (*fun)(VkInstance, const C *createInfo, const VkAllocationCallbacks *pAllocator, T *res), const C &createInfo)
		{
			T res;

			Vk::assert(fun(*this, &createInfo, m_vk.getAllocator(), &res));
			return res;
		}

		template <typename T, typename C, typename Val>
		// Val is convertible to C
		auto createVk(VkResult (*fun)(VkInstance, C *createInfo, const VkAllocationCallbacks *pAllocator, T *res), const Val &val)
		{
			T res;

			Vk::assert(fun(*this, val, m_vk.getAllocator(), &res));
			return res;
		}

		template <typename T, typename Fun, typename C>
		auto create(Fun &&fun, const C &createInfo)
		{
			return T(*this, createVk(std::forward<Fun>(fun), createInfo));
		}

		template <typename T>
		void destroy(void (*fun)(VkInstance, T obj, const VkAllocationCallbacks *pAllocator), T obj)
		{
			fun(*this, obj, m_vk.getAllocator());
		}

	private:
		Vk &m_vk;
	};

	Instance m_instance;
	Instance createInstance(void);

	class DebugMessenger : public Instance::Handle<VkDebugUtilsMessengerEXT>
	{
	public:
		DebugMessenger(Instance &instance, VkDebugUtilsMessengerEXT messenger);
	};

	std::optional<DebugMessenger> m_debug_messenger;
	std::optional<DebugMessenger> createDebugMessenger(void);

	class Surface : public Instance::Handle<VkSurfaceKHR>
	{
	public:
		Surface(Instance &instance, VkSurfaceKHR surface);
	};

	Surface m_surface;
	Surface createSurface(void);

	class PhysicalDevice
	{
	public:
		PhysicalDevice(VkPhysicalDevice device, Vk::Surface &surface);

		class Surface
		{
		public:
			Surface(PhysicalDevice &device, Vk::Surface &surface);

			const VkSurfaceCapabilitiesKHR& capabilities(void) const;
			const std::vector<VkSurfaceFormatKHR>& formats(void) const;
			const std::vector<VkPresentModeKHR>& presentModes(void) const;

			const VkSurfaceFormatKHR& chooseFormat(void) const;
			VkPresentModeKHR choosePresentMode(void) const;
			VkExtent2D chooseExtent(VkExtent2D baseExtent) const;

			operator VkSurfaceKHR(void) const;

		private:
			const VkSurfaceCapabilitiesKHR m_capabilities;
			const std::vector<VkSurfaceFormatKHR> m_formats;
			const std::vector<VkPresentModeKHR> m_present_modes;
			Vk::Surface &m_vk_surface;
		};

		operator VkPhysicalDevice(void) const;

		struct Properties : public VkPhysicalDeviceProperties
		{
			Properties(const VkPhysicalDeviceProperties &props);

			size_t getAlignment(sb::Shader::DescriptorType type) const;
		};

		static const VkPhysicalDeviceFeatures& requiredFeatures(void);

		const Properties& properties(void) const;
		const VkPhysicalDeviceFeatures& features(void) const;
		const Surface& surface(void) const;

		bool getSurfaceSupport(uint32_t queueFamilyIndex) const;
		bool isCompetent(const sb::Queue::Set &requiredQueues) const;
		size_t getScore(void) const;

		static const util::svec required_extensions;

		class QueueFamilies
		{
		public:
			QueueFamilies(PhysicalDevice &device);

			const std::vector<VkQueueFamilyProperties>& properties(void) const;
			std::optional<uint32_t> indexOf(sb::Queue::Flag flags, size_t count = 1) const;

		private:
			PhysicalDevice &m_physical_device;
			std::vector<VkQueueFamilyProperties> m_queues;
		};

		const QueueFamilies& queues(void) const;

	private:
		VkPhysicalDevice m_device;
		Vk::Surface &m_surface;
		const Properties m_props;
		const VkPhysicalDeviceFeatures m_features;
		const QueueFamilies m_queue_families;
		Surface m_phys_surface;

		bool areExtensionsSupported(void) const;
	};

	class PhysicalDevices
	{
	public:
		PhysicalDevices(Vk::Instance &instance, Vk::Surface &surface);

		const PhysicalDevice& getBest(const sb::Queue::Set &requiredQueues) const;

	private:
		std::vector<PhysicalDevice> m_devices;

		std::vector<Vk::PhysicalDevice> enumerate(Vk::Instance &instance, Vk::Surface &surface);
	};

	class VmaBuffer;

	class Allocator : public Vk::Handle<VmaAllocator>
	{
	public:
		Allocator(Device &device);

		template <typename VkHandle>
		using Handle = HandleDep<Allocator, VkHandle>;

		VmaBuffer createBuffer(const VkBufferCreateInfo &bufferCreateInfo, const VmaAllocationCreateInfo &allocationCreateInfo);

	private:
		Device &m_device;

		VmaAllocator create(Device &device);
	};

	using VkQueueFamilyIndex = uint32_t;
	using sbQueueFamilyMapping = std::map<sb::Queue::Flag, VkQueueFamilyIndex>;
	using sbQueueIndex = size_t;
	using VkQueueIndex = uint32_t;
	using sbQueueMapping = std::map<std::pair<sb::Queue::Flag, sbQueueIndex>, std::pair<VkQueueFamilyIndex, VkQueueIndex>>;

	class Device : public Handle<VkDevice>
	{
		friend Handle<VkDevice>;

	public:
		Device(Instance &instance, const PhysicalDevice &physicalDevice, const sbQueueFamilyMapping& queueFamilyMapping, const sbQueueMapping &queueMapping, VkDevice device);

		Vk& vk(void)
		{
			return m_instance.m_vk;
		}

		const PhysicalDevice& physical(void) const;
		Allocator& allocator(void);
		std::pair<VkQueueFamilyIndex, VkQueue> getQueue(sb::Queue::Flag flags, size_t ndx);
		VkFormat sbFormatToVk(sb::Format format) const;

		template <typename VkHandle>
		using Handle = HandleDep<Device, VkHandle>;

		template <typename T, typename C>
		auto createVk(VkResult (*fun)(VkDevice, const C *createInfo, const VkAllocationCallbacks *pAllocator, T *res), const C &createInfo)
		{
			T res;

			Vk::assert(fun(*this, &createInfo, m_instance.m_vk.getAllocator(), &res));
			return res;
		}

		template <typename T, typename C, typename VkHandle1>
		auto createVk(VkResult (*fun)(VkDevice, VkHandle1, uint32_t createInfoCount, const C *createInfos, const VkAllocationCallbacks *pAllocator, T *res), const C &createInfo)
		{
			T res;

			Vk::assert(fun(*this, VK_NULL_HANDLE, 1, &createInfo, m_instance.m_vk.getAllocator(), &res));
			return res;
		}

		template <typename T, typename Fun, typename C>
		auto create(Fun &&fun, const C &createInfo)
		{
			return T(*this, createVk(std::forward<Fun>(fun), createInfo));
		}

		template <typename T, typename C>
		auto allocateVk(VkResult (*fun)(VkDevice, const C *allocInfo, T *res), const C &createInfo)
		{
			T res;

			Vk::assert(fun(*this, &createInfo, &res));
			return res;
		}

		template <typename T, typename C>
		void allocateVks(VkResult (*fun)(VkDevice, const C *allocInfo, T *res), const C &createInfo, T *dst)
		{
			Vk::assert(fun(*this, &createInfo, dst));
		}

		template <typename T>
		void destroy(void (*fun)(VkDevice, T obj, const VkAllocationCallbacks *pAllocator), T obj)
		{
			fun(*this, obj, m_instance.m_vk.getAllocator());
		}

	private:
		Instance &m_instance;
		PhysicalDevice m_physical;
		sbQueueFamilyMapping m_queue_family_mapping;
		sbQueueMapping m_queue_mapping;
		Allocator m_allocator;
		std::map<sb::Format, VkFormat> m_dynamic_formats;

		std::map<sb::Format, VkFormat> getDynamicFormats(void);
		bool isDepthFormatSupportedSplAtt(VkFormat format);
	};

	class Allocation : public Allocator::Handle<VmaAllocation>
	{
	public:
		Allocation(Allocator &allocator, VmaAllocation alloc);

		void* map(void);
		void unmap(void);
	};

	class Buffer : public Device::Handle<VkBuffer>
	{
	public:
		Buffer(Device &dev, VkBuffer buffer);
	};

	class VmaBuffer : public sb::Buffer, public Allocation, public Vk::Buffer
	{
	public:
		VmaBuffer(Device &dev, VkBuffer buffer, VmaAllocation allocation);
		~VmaBuffer(void) override;

		VmaBuffer(VmaBuffer&&) = default;

		void write(size_t off, size_t size, const void *data) override;
	};

	std::unique_ptr<sb::Buffer> createBuffer(size_t size, sb::Buffer::Location location, sb::Buffer::Usage usage, sb::Queue &queue) override;

	Device m_device;
	Device createDevice(const sb::Queue::Set &queues);

	class Image : private Allocation, public Device::Handle<VkImage>
	{
		static VkImageAspectFlags sbFormatToImageAspectFlags(sb::Format format);

	public:
		Image(Device &dev, VkImage image, VmaAllocation allocation);
	};

	class ImageView : public sb::Image, public Device::Handle<VkImageView>
	{
	public:
		ImageView(Device &dev, VkImageView view);
		~ImageView(void) override;

		ImageView(ImageView&&) = default;
	};

	class ImageAllocView : public Vk::Image, public ImageView
	{
	public:
		ImageAllocView(Vk::Image &&image, ImageView &&view);
		~ImageAllocView(void) override;
	};

	std::unique_ptr<sb::Image> createImage(sb::Image::Type type, Format format, sb::Image::Sample sample, const svec3 &extent, size_t layers, const sb::Image::MipmapLevels &mipLevels, sb::Image::Usage usage, sb::Queue &queue) override;

	static inline VkImageLayout sbImageLayoutToVk(sb::Image::Layout layout)
	{
		return static_cast<VkImageLayout>(static_cast<std::underlying_type_t<sb::Image::Layout>>(layout));
	}

	static VkDescriptorType descriptorType(sb::Shader::DescriptorType type);

	class DescriptorSetLayout : public sb::Shader::DescriptorSet::Layout, public Device::Handle<VkDescriptorSetLayout>
	{
	public:
		DescriptorSetLayout(Device &device, const sb::Shader::DescriptorSet::Layout::Description &layout);
		~DescriptorSetLayout(void) override;
		DescriptorSetLayout(DescriptorSetLayout&&) = default;

		const sb::Shader::DescriptorSet::Layout::Description& getDescription(void) const { return m_desc; }
		auto getBufferSize(void) const { return m_buffer_size; }
		auto getUniformOff(void) const { return m_uniform_off; }
		auto getUniformSize(void) const { return m_uniform_size; }
		auto getStorageOff(void) const { return m_storage_off; }
		auto getStorageSize(void) const { return m_storage_size; }

	private:
		const sb::Shader::DescriptorSet::Layout::Description m_desc;
		size_t m_buffer_size = 0;
		size_t m_uniform_off = 0;
		size_t m_uniform_size = 0;
		size_t m_storage_off = 0;
		size_t m_storage_size = 0;

		VkDescriptorSetLayout create(Device &device, const sb::Shader::DescriptorSet::Layout::Description &layout);
		VkDescriptorSetLayoutBinding bindingtoVk(const sb::Shader::DescriptorSet::Layout::DescriptionBinding &binding);
	};

	std::unique_ptr<sb::Shader::DescriptorSet::Layout> createDescriptorSetLayout(const sb::Shader::DescriptorSet::Layout::Description &desc) override;

	class DescriptorSet : public sb::Shader::DescriptorSet
	{
	public:
		DescriptorSet(Device &dev, const DescriptorSetLayout &layout, sb::Queue *queue);	// ptr for queue buffer attribution

		sb::Buffer::Region uniformBufferRegion(void) override;
		sb::Buffer::Region storageBufferRegion(void) override;
		void bindSampler(size_t binding, sb::Sampler &sampler) override;
		void bindImage(size_t binding, sb::Image &image, sb::Image::Layout layout) override;
		void bindCombinedImageSampler(size_t binding, sb::Sampler &sampler, sb::Image &image, sb::Image::Layout layout) override;

		operator VkDescriptorSet(void) const;

	private:
		const DescriptorSetLayout &m_layout;
		Device::Handle<VkDescriptorPool> m_descriptor_pool;
		VkDescriptorSet m_descriptor_set;
		VmaBuffer m_buffer;

		VkDescriptorPool createPool(Device &dev, const DescriptorSetLayout &layout);
		VkDescriptorSet create(Device &dev, const DescriptorSetLayout &layout);
		VmaBuffer createBuffer(Device &dev, sb::Queue *queue);
	};

	class RenderPass;

	class Framebuffer : public sb::Framebuffer, public Device::Handle<VkFramebuffer>
	{
	public:
		Framebuffer(Device &dev, VkFramebuffer framebuffer, RenderPass &render_pass, std::vector<DescriptorSet> &&input_attachments);
		~Framebuffer(void) override;

		RenderPass& getRenderPass(void)
		{
			return m_render_pass;
		}

		std::vector<DescriptorSet>& getInputAttachments(void)
		{
			return m_input_attachments;
		}

	private:
		RenderPass &m_render_pass;
		std::vector<DescriptorSet> m_input_attachments;
	};

	using PipelineLayout = Device::Handle<VkPipelineLayout>;

	class RenderPass : public sb::RenderPass
	{
		static VkAttachmentLoadOp sbLoadOpToVk(sb::Image::LoadOp loadOp);
		static VkAttachmentStoreOp sbStoreOpToVk(sb::Image::StoreOp storeOp);

		static inline VkAttachmentReference sbAttachmentReferenceToVk(const sb::RenderPass::Layout::AttachmentReference &ref)
		{
			return {static_cast<uint32_t>(ref.ndx), sbImageLayoutToVk(ref.layout)};
		}

		static inline VkPipelineStageFlags sbPipelineStageToVk(sb::PipelineStage stage)
		{
			return static_cast<VkPipelineStageFlags>(static_cast<std::underlying_type_t<sb::PipelineStage>>(stage));
		}

		static inline VkAccessFlags sbAccessToVk(sb::Access access)
		{
			return static_cast<VkAccessFlags>(static_cast<std::underlying_type_t<sb::Access>>(access));
		}

		static inline VkDependencyFlags sbDependencyFlagToVk(sb::DependencyFlag flag)
		{
			return static_cast<VkDependencyFlags>(static_cast<std::underlying_type_t<sb::DependencyFlag>>(flag));
		}

	public:
		RenderPass(Device &dev, sb::rs::RenderPass &res);
		~RenderPass(void) override;

		std::unique_ptr<sb::Framebuffer> createFramebuffer(const svec2 &extent, size_t layers, size_t count, sb::Image **images) override;

		operator VkRenderPass(void) const
		{
			return m_handle;
		}

		auto& getLayout(void) const
		{
			return m_layout;
		}

		auto& getSubpassesDescriptorSetLayouts(void) const
		{
			return m_subpasses_descriptor_set_layouts;
		}

		auto& getSubpassesPipelineLayouts(void) const
		{
			return m_subpasses_pipeline_layouts;
		}

	private:
		sb::RenderPass::Layout m_layout;
		Device::Handle<VkRenderPass> m_handle;
		std::vector<DescriptorSetLayout> m_subpasses_descriptor_set_layouts;
		std::vector<PipelineLayout> m_subpasses_pipeline_layouts;

		VkRenderPass create(Device &dev);
		std::vector<DescriptorSetLayout> createSubpassesDescriptorSetLayouts(void);
		std::vector<PipelineLayout> createSubpassesPipelineLayouts(void);
	};

	std::unique_ptr<sb::RenderPass> createRenderPass(sb::rs::RenderPass &renderpass) override;

	class Swapchain : public sb::Swapchain, public Device::Handle<VkSwapchainKHR>
	{
	public:
		Swapchain(Vk::Device &device, VkSwapchainKHR swapchain);
		~Swapchain(void) override;

		std::vector<sb::Swapchain::Image2D>& getImages(void) override;
		size_t acquireNextImage(sb::Semaphore &semaphore) override;

	private:
		std::vector<sb::Swapchain::Image2D> m_images;

		std::vector<sb::Swapchain::Image2D> queryImages(void);
	};

	std::unique_ptr<sb::Swapchain> createSwapchain(const svec2 &extent, sb::Image::Usage usage, sb::Queue &queue) override;

	const VkSurfaceFormatKHR &m_swapchain_format;

	class Semaphore : public sb::Semaphore, public Device::Handle<VkSemaphore>
	{
	public:
		Semaphore(Device &dev, VkSemaphore semaphore);
		~Semaphore(void) override;
	};

	std::unique_ptr<sb::Semaphore> createSemaphore(void) override;

	class Fence : public sb::Fence, public Device::Handle<VkFence>
	{
	public:
		Fence(Device &dev, VkFence fence);
		~Fence(void) override;

		void wait(void) override;
		void reset(void) override;
	};

	std::unique_ptr<sb::Fence> createFence(bool isSignaled) override;

	using ShaderModule = Device::Handle<VkShaderModule>;
	using Pipeline = Device::Handle<VkPipeline>;

	class Shader : public sb::Shader
	{
	public:
		static VkShaderStageFlagBits sbStageToVk(Subtile::Shader::Stage stage);
		static VkFormat vertexInputFormatToVk(sb::Shader::VertexInput::Format);

		Shader(Device &device, rs::Shader &shader);

		const sb::Shader::DescriptorSet::Layout& setLayout(size_t ndx) override;
		std::unique_ptr<sb::Shader::DescriptorSet> set(size_t ndx, sb::Queue &queue) override;

		PipelineLayout& getPipelineLayout(void);
		Pipeline& getPipeline(void);

		auto getDescriptorSetOffset(void) const
		{
			return m_descriptor_set_offset;
		}

	private:
		Device &m_device;

		std::optional<std::pair<sb::RenderPass::Cache::Ref, size_t>> m_render_pass;
		std::optional<std::pair<sb::RenderPass::Cache::Ref, size_t>> loadRenderPass(rs::Shader &shader);
		size_t m_descriptor_set_offset;

		rs::Shader::DescriptorSetLayouts m_layouts;
		PipelineLayout m_pipeline_layout;
		PipelineLayout createPipelineLayout(void);
		using ShaderModulesType = std::vector<std::pair<VkShaderStageFlagBits, ShaderModule>>;
		ShaderModulesType m_shader_modules;
		ShaderModulesType createShaderModules(Vk::Device &device, rs::Shader &shader);

		std::optional<Pipeline> m_pipeline;
		std::optional<Pipeline> createPipeline(Vk::Device &device, rs::Shader &shader);
	};

	std::unique_ptr<sb::Shader> createShader(rs::Shader &shader) override;

	class CommandPool;

	class CommandBuffer : public sb::CommandBuffer
	{
	public:
		CommandBuffer(CommandPool &pool, sb::CommandBuffer::Level level);
		~CommandBuffer(void) override;

		operator VkCommandBuffer(void) const
		{
			return m_handle;
		}

		void reset(bool releaseResources) override;
		void begin(Usage flags) override;
		void beginRender(Usage flags, sb::Framebuffer &fb, size_t subpass) override;
		void beginRender(Usage flags, sb::RenderPass &rp, size_t subpass) override;
		void end(void) override;

		void executeCommands(size_t count, sb::CommandBuffer **cmds) override;
		void bindPipeline(sb::Shader &shader) override;
		void bindDescriptorSets(sb::Shader &shader, size_t first_set, size_t count, sb::Shader::DescriptorSet **sets) override;

		void beginRenderPass(bool isInline, sb::Framebuffer &fb, const srect2 &renderArea, size_t clearValueCount, ClearValue *clearValues) override;
		void nextSubpass(bool isInline) override;
		void endRenderPass(void) override;

		void copy(const sb::Buffer::Region &src, const sb::Buffer::Region &dst) override;
		void memoryBarrier(PipelineStage srcStageMask, PipelineStage dstStageMask, Access srcAccessMask, Access dstAccessMask, DependencyFlag flags) override;

	private:
		CommandPool &m_pool;
		VkCommandBuffer m_handle;
		size_t m_render_pass_subpass_ndx = 0;
		Framebuffer *m_render_pass_fb;

		VkCommandBuffer createCommandBuffer(sb::CommandBuffer::Level level);
	};

	class CommandPool : public sb::CommandPool, public Device::Handle<VkCommandPool>
	{
	public:
		CommandPool(Device &dev, VkQueueFamilyIndex familyIndex, bool isReset);
		~CommandPool(void) override;

		std::unique_ptr<sb::CommandBuffer> commandBuffer(sb::CommandBuffer::Level level) override;

	private:
		VkCommandPool create(Device &dev, VkQueueFamilyIndex familyIndex, bool isReset);
	};

	class Queue : public sb::Queue
	{
	public:
		Queue(Device &dev, VkQueueFamilyIndex familyIndex, VkQueue queue);
		~Queue(void) override;

		std::unique_ptr<sb::CommandPool> commandPool(bool isReset) override;
		void submit(size_t submitCount, SubmitInfo *submits, sb::Fence *fence) override;
		void present(size_t waitSemaphoreCount, sb::Semaphore **waitSemaphores, sb::Swapchain::Image2D &image) override;
		void waitIdle(void) override;

		operator VkQueue(void) const
		{
			return m_handle;
		}

		auto getFamilyIndex(void) const
		{
			return m_family_index;
		}

	private:
		Device &m_device;
		VkQueueFamilyIndex m_family_index;
		VkQueue m_handle;
	};

	std::unique_ptr<sb::Queue> getQueue(sb::Queue::Flag flags, size_t index) override;

	class Model : public sb::Model
	{
	public:
		Model(VmaBuffer &buffer, size_t vertexCount);
		~Model(void) override;

		void draw(sb::CommandBuffer &cmd) override;

	private:
		VmaBuffer &m_buffer;
		size_t m_vertex_count;
	};

	std::unique_ptr<sb::Model> createModel(sb::Buffer &vertexBuffer, size_t vertexCount) override;

	class ModelIndexed : public sb::Model
	{
	public:
		ModelIndexed(VmaBuffer &buffer, VmaBuffer &indexBuffer, VkIndexType indexType, size_t indexCount);
		~ModelIndexed(void) override;

		void draw(sb::CommandBuffer &cmd) override;

	private:
		VmaBuffer &m_buffer;
		VmaBuffer &m_index_buffer;
		VkIndexType m_index_type;
		size_t m_index_count;
	};

	std::unique_ptr<sb::Model> createModelIndexed(sb::Buffer &vertexBuffer, sb::Buffer &indexBuffer, sb::Model::IndexType indexType, size_t indexCount) override;

	class Sampler : public sb::Sampler, public Device::Handle<VkSampler>
	{
	public:
		Sampler(Device &dev, VkSampler sampler);
		~Sampler(void) override;
	};

	std::unique_ptr<sb::Sampler> createSampler(Filter magFilter, Filter minFilter, bool normalizedCoordinates, const sb::Sampler::AddressModeUVW &addressMode, BorderColor borderColor, const std::optional<CompareOp> &compare, sb::Sampler::MipmapMode mipmapMode, float minLod, float maxLod, float mipLodBias, const std::optional<float> &anisotropy) override;
};

}