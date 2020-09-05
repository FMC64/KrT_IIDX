#pragma once

#include "Track.hpp"

namespace Krt {

class Race : public sb::Event::Socket
{
public:
	Race(Instance &instance);
	~Race(void);

	void run(void);

private:
	Instance &instance;

	sb::Sampler::Handle m_sampler;
	sb::Sampler::Handle m_fb_sampler;

	decltype(instance.load(res.shaders().render_passes().color())) m_color_pass;
	decltype(instance.load(res.shaders().render_passes().post())) m_post_pass;
	sb::Image2D m_fb_color;
	sb::Image2D m_fb_depth_buffer;
	decltype(m_color_pass)::Framebuffer m_color_fb;
	std::vector<decltype(m_post_pass)::Framebuffer> m_post_fbs;
	decltype(m_post_fbs) createPostFramebuffers(void)
	{
		decltype(m_post_fbs) res;

		for (auto &img : instance.swapchain.images())
			res.emplace_back(m_post_pass.framebuffer({1600, 900}, 1, img));
		return res;
	}

	decltype(instance.semaphore()) m_swapchain_img_avail;

	decltype(instance.load(res.shaders().render_passes().depth_range())) m_depth_range_pass;
	decltype(instance.load(res.shaders().first_depth_range())) m_first_depth_range;
	decltype(instance.load(res.shaders().compute_depth_range())) m_compute_depth_range;
	sb::Image2D m_fb_depth_range;

	struct DepthRangeMip {
		sb::Image2D img;
		decltype(m_depth_range_pass)::Framebuffer fb;
	};

	std::vector<DepthRangeMip> m_depth_range_mips;
	decltype(m_depth_range_mips) getDepthRangeMips(void)
	{
		decltype(m_depth_range_mips) res;

		for (size_t i = 0; i < m_fb_depth_range.mipLevels(); i++) {
			auto img = m_fb_depth_range.view(sb::ComponentSwizzle::Identity, sb::Image::Aspect::Color, sb::Range(i, 1));
			auto fb = m_depth_range_pass.framebuffer(img.extent(), 1, img);

			res.emplace_back(DepthRangeMip{std::move(img), std::move(fb)});
		}
		return res;
	}

	decltype(m_first_depth_range.fb(instance.graphics)) m_first_depth_range_in_fb;
	std::vector<decltype(m_compute_depth_range.fb(instance.graphics))> m_compute_depth_range_in_fb;
	decltype(m_compute_depth_range_in_fb) getComputeDepthRangeInFb(void)
	{
		decltype(m_compute_depth_range_in_fb) res;

		auto end = m_depth_range_mips.end();
		for (auto it = m_depth_range_mips.begin() + 1; it != end; it++) {
			auto &cur = *it;
			auto in_fb = m_compute_depth_range.fb(instance.graphics);
			in_fb.up.bind(m_fb_sampler, cur.img, sb::Image::Layout::General);
			res.emplace_back(std::move(in_fb));
		}
		return res;
	}

	decltype(instance.semaphore()) m_render_done;
	decltype(instance.fence()) m_render_done_fence;

	decltype(res.shaders().lighting().loaded()) m_lighting_shader;
	sb::Render::Pass m_lighting_draw_list;
	decltype(m_lighting_shader.fb(instance.graphics)) m_lighting_samplers;

	decltype(instance.graphics.pool<true>()) m_cmd_pool;
	decltype(m_cmd_pool.primary()) m_cmd_prim;

	bool m_is_done = false;
	std::unique_ptr<Track> m_track;
};

}