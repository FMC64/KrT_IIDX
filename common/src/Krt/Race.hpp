#pragma once

#include "Track.hpp"

namespace Krt {

class Race : public sb::Event::Socket
{
public:
	Race(Instance &instance);
	~Race(void);

	void run(void);

	Instance &instance;

	sb::Sampler::Handle m_sampler;
	sb::Sampler::Handle m_sampler_clamp;
	sb::Sampler::Handle m_sampler_nearest;
	sb::Sampler::Handle m_fb_sampler;
	sb::Sampler::Handle m_fb_sampler_linear;

	decltype(instance.device.load(res.shaders().render_passes().opaque())) m_opaque_pass;
	decltype(instance.device.load(res.shaders().render_passes().lighting())) m_lighting_pass;

	decltype(instance.device.load(res.shaders().render_passes().depth_max())) m_depth_max_pass;

	decltype(instance.device.load(res.shaders().render_passes().depth_range())) m_depth_range_pass;
	decltype(instance.device.load(res.shaders().first_depth_range())) m_first_depth_range;
	decltype(instance.device.load(res.shaders().compute_depth_range())) m_compute_depth_range;

	decltype(instance.device.load(res.shaders().modules().depth_buffer())) m_depth_buffer_module;
	decltype(instance.device.load(res.shaders().render_passes().depth_to_fl())) m_depth_to_fl_pass;
	decltype(instance.device.load(res.shaders().depth_to_fl())) m_depth_to_fl_shader;

	decltype(instance.device.load(res.shaders().render_passes().diffuse_bounce())) m_diffuse_bounce_pass;
	decltype(instance.device.load(res.shaders().diffuse_bounce())) m_diffuse_bounce_shader;

	decltype(instance.device.load(res.shaders().render_passes().gather_bounces())) m_gather_bounces_pass;
	decltype(instance.device.load(res.shaders().gather_bounces())) m_gather_bounces_shader;

	decltype(instance.device.load(res.shaders().render_passes().buffer_to_wsi_screen())) m_buffer_to_wsi_screen;
	decltype(instance.device.load(res.shaders().diffuse_to_wsi_screen())) m_diffuse_to_wsi_screen;

	decltype(res.shaders().lighting().loaded()) m_lighting_shader;
	decltype(instance.graphics.pool<true>()) m_cmd_pool;
	struct Image;
	std::vector<Image> images;
	decltype(images) getImages(void);

	bool m_is_done = false;
	std::unique_ptr<Track> m_track;
};

struct Race::Image {

	Image(Race &race) :
		race(race),
		fb_albedo(race.instance.device.image2D(sb::Format::rgba8_unorm, race.instance.swapchain->extent(), 1, sb::Image::Usage::ColorAttachment | sb::Image::Usage::Sampled, race.instance.graphics)),
		fb_emissive(race.instance.device.image2D(sb::Format::rgba8_unorm, race.instance.swapchain->extent(), 1, sb::Image::Usage::ColorAttachment | sb::Image::Usage::Sampled, race.instance.graphics)),
		fb_normal(race.instance.device.image2D(sb::Format::rgba16_sfloat, race.instance.swapchain->extent(), 1, sb::Image::Usage::ColorAttachment | sb::Image::Usage::Sampled, race.instance.graphics)),
		fb_depth_buffer(race.instance.device.image2D(sb::Format::d24un_or_32sf_spl_att_sfb, race.instance.swapchain->extent(), 1, sb::Image::Usage::DepthStencilAttachment | sb::Image::Usage::Sampled | sb::Image::Usage::TransferSrc, race.instance.graphics)),
		fb_depth_buffer_max(race.instance.device.image2D(sb::Format::d24un_or_32sf_spl_att_sfb, race.instance.swapchain->extent(), 1, sb::Image::Usage::DepthStencilAttachment | sb::Image::Usage::Sampled | sb::Image::Usage::TransferSrc, race.instance.graphics)),
		depth_buffer_max_fb(race.m_depth_max_pass.framebuffer(race.instance.swapchain->extent(), 1, fb_depth_buffer_max)),
		depth_buffer_trace_res(0),
		fb_depth_buffer_raw_fl(race.instance.device.image2D(sb::Format::rg32_sfloat, {static_cast<size_t>(std::pow(2.0, std::ceil(std::log2(fb_albedo.extent().x)))), static_cast<size_t>(std::pow(2.0, std::ceil(std::log2(fb_albedo.extent().y))))}, sb::Image::allMipLevels, sb::Image::Usage::ColorAttachment | sb::Image::Usage::Sampled | sb::Image::Usage::TransferSrc | sb::Image::Usage::TransferDst, race.instance.graphics)),
		fb_depth_buffer_raw_fl_mips(getDepthBufferRawFlMips()),
		depth_to_fl_fb(race.m_depth_to_fl_pass.framebuffer(fb_depth_buffer_raw_fl_mips.at(0).extent(), 1, fb_depth_buffer_raw_fl_mips.at(0))),
		depth_to_fl_set(race.m_depth_to_fl_shader.fb(race.instance.graphics)),
		fb_depth_buffer_fl(race.instance.device.image2D(sb::Format::rg32_sfloat, sb::svec2(static_cast<size_t>(std::pow(2.0, std::ceil(std::log2(fb_albedo.extent().x)))), static_cast<size_t>(std::pow(2.0, std::ceil(std::log2(fb_albedo.extent().y))))) / sb::svec2(1 << depth_buffer_trace_res), sb::Image::allMipLevels, sb::Image::Usage::ColorAttachment | sb::Image::Usage::Sampled | sb::Image::Usage::TransferSrc | sb::Image::Usage::TransferDst, race.instance.graphics)),
		fb_depth_buffer_fl_mips(getDepthBufferFlMips()),
		opaque_fb(race.m_opaque_pass.framebuffer(race.instance.swapchain->extent(), 1, fb_albedo, fb_emissive, fb_normal, fb_depth_buffer)),
		primary(race.instance.device.image2D(sb::Format::rgba16_sfloat, race.instance.swapchain->extent(), 1, sb::Image::Usage::ColorAttachment | sb::Image::Usage::Sampled, race.instance.graphics)),
		diffuse(race.instance.device.image2D(sb::Format::rgba16_sfloat, race.instance.swapchain->extent(), 1, sb::Image::Usage::ColorAttachment | sb::Image::Usage::Sampled, race.instance.graphics)),
		lighting_fb(race.m_lighting_pass.framebuffer(race.instance.swapchain->extent(), 1, primary)),
		lighting_samplers(getLightingSamplers()),
		swapchain_img_avail(race.instance.device.semaphore()),
		first_depth_range_in_fb(getFirstDepthRangeInFb()),
		compute_depth_range_in_fb(getComputeDepthRangeInFb()),
		diffuse_bounce_random(race.m_diffuse_bounce_shader.random(race.instance.graphics)),
		diffuse_bounces(getDiffuseBounces(*this, 2)),
		gather_bounces_set(race.m_gather_bounces_shader.light(race.instance.graphics)),
		gather_bounces_fb(race.m_gather_bounces_pass.framebuffer(race.instance.swapchain->extent(), 1, diffuse)),
		buffer_to_wsi_screen_fbs(getBufferToWsiScreenFbs()),
		diffuse_to_wsi_screen_set(race.m_diffuse_to_wsi_screen.light(race.instance.graphics)),
		render_done(race.instance.device.semaphore()),
		render_done_fence(race.instance.device.fence(false)),
		ever_rendered(false),
		cmd_prim(race.m_cmd_pool.primary())
	{
		depth_to_fl_set.depth_buffer.bind(race.m_fb_sampler, fb_depth_buffer, sb::Image::Layout::ShaderReadOnlyOptimal);
		depth_to_fl_set.depth_buffer_max.bind(race.m_fb_sampler, fb_depth_buffer_max, sb::Image::Layout::ShaderReadOnlyOptimal);

		gather_bounces_set.primary.bind(race.m_fb_sampler, primary, sb::Image::Layout::ShaderReadOnlyOptimal);
		for (auto &b : gather_bounces_set.bounces)
			b.bind(race.m_fb_sampler, primary, sb::Image::Layout::ShaderReadOnlyOptimal);
		{
			size_t ndx = 0;
			for (auto &b : diffuse_bounces)
				gather_bounces_set.bounces.at(ndx++).bind(race.m_fb_sampler, b.img, sb::Image::Layout::ShaderReadOnlyOptimal);
		}
		gather_bounces_set.bounce_count = diffuse_bounces.size();
		race.instance.cur_img_res->uploadDescSet(gather_bounces_set);
		gather_bounces_set.depth_buffer.bind(race.m_fb_sampler_linear, fb_depth_buffer, sb::Image::Layout::ShaderReadOnlyOptimal);

		diffuse_to_wsi_screen_set.diffuse.bind(race.m_fb_sampler, diffuse, sb::Image::Layout::ShaderReadOnlyOptimal);

		auto cmd = race.m_cmd_pool.primary();
		cmd.record([&](auto &cmd){
			cmd.imageMemoryBarrier(sb::PipelineStage::BottomOfPipe, sb::PipelineStage::TopOfPipe, {},
				sb::Access::MemoryWrite, sb::Access::MemoryRead,
				sb::Image::Layout::Undefined, sb::Image::Layout::ShaderReadOnlyOptimal, fb_depth_buffer);
			cmd.imageMemoryBarrier(sb::PipelineStage::BottomOfPipe, sb::PipelineStage::TopOfPipe, {},
				sb::Access::MemoryWrite, sb::Access::MemoryRead,
				sb::Image::Layout::Undefined, sb::Image::Layout::ShaderReadOnlyOptimal, fb_depth_buffer_fl);
			cmd.imageMemoryBarrier(sb::PipelineStage::BottomOfPipe, sb::PipelineStage::TopOfPipe, {},
				sb::Access::MemoryWrite, sb::Access::MemoryRead,
				sb::Image::Layout::Undefined, sb::Image::Layout::ShaderReadOnlyOptimal, diffuse);
		});
		race.instance.graphics.submit(util::empty, cmd, util::empty);
		race.instance.graphics.waitIdle();
	}

	Race &race;
	sb::Image2D fb_albedo;
	sb::Image2D fb_emissive;
	sb::Image2D fb_normal;
	sb::Image2D fb_depth_buffer;
	sb::Image2D fb_depth_buffer_max;
	decltype(Race::m_depth_max_pass)::Framebuffer depth_buffer_max_fb;
	size_t depth_buffer_trace_res;
	sb::Image2D fb_depth_buffer_raw_fl;
	std::vector<sb::Image2D> fb_depth_buffer_raw_fl_mips;
	decltype(fb_depth_buffer_raw_fl_mips) getDepthBufferRawFlMips(void)
	{
		decltype(fb_depth_buffer_raw_fl_mips) res;

		for (size_t i = 0; i < fb_depth_buffer_raw_fl.mipLevels(); i++)
			res.emplace_back(fb_depth_buffer_raw_fl.view(sb::ComponentSwizzle::Identity, sb::Image::Aspect::Color, sb::Range(i, 1)));
		return res;
	}
	decltype(Race::m_depth_to_fl_pass)::Framebuffer depth_to_fl_fb;
	decltype(race.m_depth_to_fl_shader.fb(race.instance.graphics)) depth_to_fl_set;
	sb::Image2D fb_depth_buffer_fl;
	struct DepthBufferMip {
		sb::Image2D img;
		decltype(Race::m_depth_range_pass)::Framebuffer fb;
	};
	std::vector<DepthBufferMip> fb_depth_buffer_fl_mips;
	decltype(fb_depth_buffer_fl_mips) getDepthBufferFlMips(void)
	{
		decltype(fb_depth_buffer_fl_mips) res;

		for (size_t i = 0; i < fb_depth_buffer_fl.mipLevels(); i++) {
			auto img = fb_depth_buffer_fl.view(sb::ComponentSwizzle::Identity, sb::Image::Aspect::Color, sb::Range(i, 1));
			auto fb = race.m_depth_range_pass.framebuffer(img.extent(), 1, img);
			res.emplace_back(DepthBufferMip{std::move(img), std::move(fb)});
		}
		return res;
	}

	decltype(m_opaque_pass)::Framebuffer opaque_fb;
	sb::Image2D primary;
	sb::Image2D diffuse;
	decltype(Race::m_lighting_pass)::Framebuffer lighting_fb;
	decltype(race.m_lighting_shader.fb(race.instance.graphics)) lighting_samplers;
	decltype(lighting_samplers) getLightingSamplers(void)
	{
		auto res = race.m_lighting_shader.fb(race.instance.graphics);

		res.albedo.bind(race.m_fb_sampler, fb_albedo, sb::Image::Layout::ShaderReadOnlyOptimal);
		res.emissive.bind(race.m_fb_sampler, fb_emissive, sb::Image::Layout::ShaderReadOnlyOptimal);
		res.normal.bind(race.m_fb_sampler, fb_normal, sb::Image::Layout::ShaderReadOnlyOptimal);
		res.depth_buffer.bind(race.m_fb_sampler_linear, fb_depth_buffer, sb::Image::Layout::ShaderReadOnlyOptimal);
		res.depth_buffer_fl.bind(race.m_sampler_clamp, fb_depth_buffer_fl, sb::Image::Layout::ShaderReadOnlyOptimal);
		res.depth_buffer_fl_size = (glm::vec2(1.0) / glm::vec2(fb_depth_buffer_fl.extent())) * glm::vec2(1.0 / static_cast<double>(1 << depth_buffer_trace_res));
		res.depth_buffer_trace_res = depth_buffer_trace_res;
		res.depth_buffer_max_it = 200 / (1 << depth_buffer_trace_res);
		race.instance.cur_img_res->uploadDescSet(res);
		return res;
	}

	decltype(Race::instance.device.semaphore()) swapchain_img_avail;

	decltype(m_first_depth_range.fb(race.instance.graphics)) first_depth_range_in_fb;
	decltype(first_depth_range_in_fb) getFirstDepthRangeInFb(void)
	{
		auto res = race.m_first_depth_range.fb(race.instance.graphics);

		res.up.bind(race.m_fb_sampler, fb_depth_buffer_raw_fl_mips.at(depth_buffer_trace_res), sb::Image::Layout::ShaderReadOnlyOptimal);
		return res;
	}
	std::vector<decltype(m_compute_depth_range.fb(race.instance.graphics))> compute_depth_range_in_fb;
	decltype(compute_depth_range_in_fb) getComputeDepthRangeInFb(void)
	{
		decltype(compute_depth_range_in_fb) res;

		auto end = fb_depth_buffer_fl_mips.end();
		for (auto it = fb_depth_buffer_fl_mips.begin() + 1; it != end; it++) {
			auto in_fb = race.m_compute_depth_range.fb(race.instance.graphics);
			in_fb.up.bind(race.m_fb_sampler, (it - 1)->img, sb::Image::Layout::ShaderReadOnlyOptimal);
			res.emplace_back(std::move(in_fb));
		}
		return res;
	}

	decltype(race.m_diffuse_bounce_shader.random(race.instance.graphics)) diffuse_bounce_random;
	struct DiffuseBounce {
		sb::Image2D img;
		decltype(race.m_diffuse_bounce_pass)::Framebuffer fb;
		decltype(race.m_diffuse_bounce_shader.fb(race.instance.graphics)) set;
	};
	std::vector<DiffuseBounce> diffuse_bounces;
	std::vector<DiffuseBounce> getDiffuseBounces(Image &image, size_t count)
	{
		std::vector<DiffuseBounce> res;

		for (size_t i = 0; i < count; i++) {
			auto img = race.instance.device.image2D(sb::Format::rgba16_sfloat, race.instance.swapchain->extent(), 1, sb::Image::Usage::ColorAttachment | sb::Image::Usage::Sampled, race.instance.graphics);
			auto fb = race.m_diffuse_bounce_pass.framebuffer(race.instance.swapchain->extent(), 1, img);
			auto set = race.m_diffuse_bounce_shader.fb(race.instance.graphics);
			set.albedo.bind(race.m_fb_sampler, fb_albedo, sb::Image::Layout::ShaderReadOnlyOptimal);
			set.normal.bind(race.m_fb_sampler, fb_normal, sb::Image::Layout::ShaderReadOnlyOptimal);
			set.last_diffuse.bind(race.m_fb_sampler, i == 0 ? static_cast<sb::Image2D&>(primary) : res.at(i - 1).img, sb::Image::Layout::ShaderReadOnlyOptimal);
			set.depth_buffer.bind(race.m_fb_sampler_linear, fb_depth_buffer, sb::Image::Layout::ShaderReadOnlyOptimal);
			set.depth_buffer_fl.bind(race.m_sampler_clamp, fb_depth_buffer_fl, sb::Image::Layout::ShaderReadOnlyOptimal);
			set.depth_buffer_fl_size = image.lighting_samplers.depth_buffer_fl_size;
			set.depth_buffer_trace_res = image.lighting_samplers.depth_buffer_trace_res;
			set.depth_buffer_max_it = image.lighting_samplers.depth_buffer_max_it;
			set.index = i;
			race.instance.cur_img_res->uploadDescSet(set);
			res.emplace_back(DiffuseBounce{std::move(img), std::move(fb), std::move(set)});
		}
		return res;
	}

	decltype(race.m_gather_bounces_shader.light(instance.graphics)) gather_bounces_set;
	decltype(race.m_gather_bounces_pass)::Framebuffer gather_bounces_fb;

	std::vector<decltype(race.m_buffer_to_wsi_screen)::Framebuffer> buffer_to_wsi_screen_fbs;
	decltype(race.m_diffuse_to_wsi_screen.light(race.instance.graphics)) diffuse_to_wsi_screen_set;
	decltype(buffer_to_wsi_screen_fbs) getBufferToWsiScreenFbs(void)
	{
		decltype(buffer_to_wsi_screen_fbs) res;

		for (auto &img : race.instance.swapchain->images())
			res.emplace_back(race.m_buffer_to_wsi_screen.framebuffer(race.instance.swapchain->extent(), 1, img));
		return res;
	}

	decltype(race.instance.device.semaphore()) render_done;
	decltype(race.instance.device.fence()) render_done_fence;
	bool ever_rendered;
	decltype(m_cmd_pool.primary()) cmd_prim;
};

inline decltype(Race::images) Race::getImages(void)
{
	decltype(images) res;

	for (size_t i = 0; i < instance.img_count; i++)
		res.emplace_back(*this);
	{
		size_t ndx = 0;
		for (auto &i : res) {
			res.at((ndx + 1) % res.size()).gather_bounces_set.last_diffuse.bind(m_fb_sampler_linear, i.diffuse, sb::Image::Layout::ShaderReadOnlyOptimal);
			ndx++;
		}
	}
	return res;
}

}