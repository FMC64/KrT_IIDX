#include <chrono>
#include <thread>
#include "Race.hpp"
#include "res.resdecl.hpp"
#include <glm/gtx/transform.hpp>

namespace Krt {

Race::Race(Instance &instance) :
	instance(instance),

	m_sampler(instance.device.sampler(sb::Filter::Linear, sb::Filter::Nearest, sb::Sampler::AddressMode::MirroredRepeat, sb::BorderColor::FloatOpaqueWhite, std::nullopt, sb::Sampler::MipmapMode::Linear, 0.0f, 64.0f, 0.0f, std::nullopt)),
	m_sampler_clamp(instance.device.sampler(sb::Filter::Linear, sb::Filter::Linear, sb::Sampler::AddressMode::ClampToEdge, sb::BorderColor::FloatOpaqueWhite, std::nullopt, sb::Sampler::MipmapMode::Nearest, 0.0f, 64.0f, 0.0f, std::nullopt)),
	m_sampler_nearest(instance.device.sampler(sb::Filter::Nearest, sb::Filter::Nearest, sb::Sampler::AddressMode::ClampToEdge, sb::BorderColor::FloatOpaqueWhite, std::nullopt, sb::Sampler::MipmapMode::Nearest, 0.0f, 64.0f, 0.0f, std::nullopt)),
	m_fb_sampler(instance.device.samplerUnnormalized(sb::Filter::Nearest, sb::Sampler::AddressMode::ClampToEdge, sb::BorderColor::FloatOpaqueWhite, 0.0f)),
	m_fb_sampler_linear(instance.device.samplerUnnormalized(sb::Filter::Linear, sb::Sampler::AddressMode::ClampToEdge, sb::BorderColor::FloatOpaqueWhite, 0.0f)),
	m_opaque_pass(instance.device.load(res.shaders().render_passes().opaque())),
	m_depth_max_pass(instance.device.load(res.shaders().render_passes().depth_max())),
	m_depth_max_shader(instance.device.load(res.shaders().depth_max())),
	m_depth_inter_front_shader(instance.device.load(res.shaders().depth_inter_front())),
	m_depth_inter_back_shader(instance.device.load(res.shaders().depth_inter_back())),
	m_depth_range_pass(instance.device.load(res.shaders().render_passes().depth_range())),
	m_first_depth_range(instance.device.load(res.shaders().first_depth_range())),
	m_compute_depth_range(instance.device.load(res.shaders().compute_depth_range())),
	m_depth_buffer_module(instance.device.load(res.shaders().modules().depth_buffer())),
	m_depth_to_fl_pass(instance.device.load(res.shaders().render_passes().depth_to_fl())),
	m_depth_to_fl_shader(instance.device.load(res.shaders().depth_to_fl())),
	m_buffer_to_wsi_screen(instance.device.load(res.shaders().render_passes().buffer_to_wsi_screen())),
	m_diffuse_to_wsi_screen(instance.device.load(res.shaders().diffuse_to_wsi_screen())),
	m_env_shader(instance.device.load(res.shaders().modules().env())),
	m_rt_shader(instance.device.load(res.shaders().modules().rt())),
	m_scheduling_pass(instance.device.load(res.shaders().render_passes().scheduling())),
	m_scheduling_fb_shader(instance.device.load(res.shaders().modules().scheduling_fb())),
	m_scheduling_shader(instance.device.load(res.shaders().scheduling())),
	m_cube_depth_pass(instance.device.load(res.shaders().render_passes().cube_depth())),
	m_cube_depth_shader(instance.device.load(res.shaders().cube_depth())),
	m_cmd_pool(instance.graphics.pool<true>()),
	m_rt_quality(4),
	env(instance.loadImageCube_srgb("res/env/consul")),
	m_opaque_env_shader(instance.device.load(res.shaders().opaque_env())),
	images(getImages()),

	m_track(instance.create<Track>())
{
	bind(m_track->done, [this](){
		m_is_done = true;
	});
}

Race::~Race(void)
{
	instance.graphics.waitIdle();
}

void Race::run(void)
{
	Image *last_frame = nullptr;
	auto &esc = *instance.surface->buttonsId().at("KB_ESCAPE");
	auto &f5 = *instance.surface->buttonsId().at("KB_F5");
	auto &f6 = *instance.surface->buttonsId().at("KB_F6");
	auto &f8 = *instance.surface->buttonsId().at("KB_F8");
	auto &f9 = *instance.surface->buttonsId().at("KB_F9");
	auto &f11 = *instance.surface->buttonsId().at("KB_F11");
	bool cursor_mode = false;
	std::optional<size_t> monitor;
	size_t video_mode = 0;
	size_t step = 0;
	while (!m_is_done) {
		auto t_start = std::chrono::high_resolution_clock::now();

		instance.scanInputs();
		esc.update();
		if (esc.released()) {
			cursor_mode = !cursor_mode;
			m_track->render.base_cursor = instance.surface->cursor();
			instance.surface->cursorMode(cursor_mode);
			if (!cursor_mode)
				m_track->render.base_cursor = instance.surface->cursor();
		}
		if (instance.surface->shouldClose())
			break;

		bool shouldRecreateSc = false;

		f5.update();
		if (f5.released() && m_rt_quality > 0) {
			instance.graphics.waitIdle();
			m_rt_quality--;
			std::cout << "RT_QUALITY: " << m_rt_quality << std::endl;
			for (auto &i : images)
				i.update_depth_buffer_trace_res(m_rt_quality);
			last_frame = nullptr;
		}
		f6.update();
		if (f6.released() && m_rt_quality < images.at(0).fb_depth_buffer_fl_mips.size()) {
			instance.graphics.waitIdle();
			m_rt_quality++;
			std::cout << "RT_QUALITY: " << m_rt_quality << std::endl;
			for (auto &i : images)
				i.update_depth_buffer_trace_res(m_rt_quality);
			last_frame = nullptr;
		}
		f11.update();
		if (f11.released()) {
			if (monitor) {
				instance.surface->setWindowed();
				monitor.reset();
			} else {
				video_mode = 0;
				monitor = 0;
				auto &mon = instance.monitors().at(*monitor);
				auto& modes = mon.videoModes();
				instance.surface->setMonitor(mon, modes.at(video_mode));
			}
			shouldRecreateSc = true;
		}
		f8.update();
		f9.update();
		if (monitor) {
			if (f8.released() && video_mode > 0) {
				video_mode--;
				monitor = 0;
				auto &mon = instance.monitors().at(*monitor);
				auto& modes = mon.videoModes();
				instance.surface->setMonitor(mon, modes.at(video_mode));
				shouldRecreateSc = true;
			}
			if (f9.released()) {
				monitor = 0;
				auto &mon = instance.monitors().at(*monitor);
				auto& modes = mon.videoModes();
				video_mode = std::min(modes.size() - 1, video_mode + 1);
				instance.surface->setMonitor(mon, modes.at(video_mode));
				shouldRecreateSc = true;
			}
		}
		if (shouldRecreateSc) {
			instance.graphics.waitIdle();
			images.clear();
			instance.swapchain.reset();
			instance.swapchain = instance.device.swapchain(*instance.surface, instance.surface->extent(), instance.swap_imgs, sb::Image::Usage::ColorAttachment, instance.graphics);
			images = getImages();
			last_frame = nullptr;
		}

		auto resized = instance.surface->resized();
		if (resized) {
			instance.graphics.waitIdle();
			images.clear();
			instance.swapchain.reset();
			/*instance.surface.reset();
			instance.surface = static_cast<sb::InstanceBase&>(instance).surface(*resized, "SUNREN®");
			instance.device.newSurface(*instance.surface);*/
			while (resized->x * resized->y == 0) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				instance.scanInputs();
				resized = instance.surface->resized();
			}
			instance.swapchain = instance.device.swapchain(*instance.surface, instance.surface->extent(), instance.swap_imgs, sb::Image::Usage::ColorAttachment, instance.graphics);
			images = getImages();
			last_frame = nullptr;
		}
		auto &img = images.at(instance.cur_img);

		auto t_before_ac = std::chrono::high_resolution_clock::now();
		auto swapchain_img = instance.swapchain->acquireNextImage(img.swapchain_img_avail);
		auto t_begin_record = std::chrono::high_resolution_clock::now();

		instance.scanInputs();
		m_track->events.updateEvents();

		img.cmd_prim.record([&](auto &cmd){
			cmd.memoryBarrier(sb::PipelineStage::Transfer, sb::PipelineStage::AllGraphics, {},
				sb::Access::TransferWrite, sb::Access::MemoryRead);

			/*auto srgb_lin = [](double val){
				constexpr float unorm = 1.0 / 255.0;

				return std::pow(val * unorm, 2.2);
			};*/

			auto viewport = sb::rect2({0.0f, 0.0f}, {instance.swapchain->extent().x, instance.swapchain->extent().y});

			cmd.setViewport(viewport, 0.0f, 1.0f);
			cmd.setScissor({{0, 0}, instance.swapchain->extent()});
			cmd.render(img.opaque_fb, {{0, 0}, instance.swapchain->extent()},
				sb::Color::f32(0.0f),	// albedo
				sb::Color::f32(0.0f),	// emissive
				sb::Color::f32(0.0f, 0.0f, 1.0f, 0.0f),	// normal
				sb::Color::f32(0.0f),	// refl
				1.0f,	// depth_buffer

				[&](auto &cmd){
					auto &cam = m_track->render.camera;
					auto &s_rt = img.rt_set;
					auto &s_sche = img.scheduling_set;

					glm::mat4 last_view = cam.view;
					glm::vec3 last_pos = m_track->render.camera_pos;
					m_track->render.render(cmd, cursor_mode);
					glm::mat4 view = cam.view;

					auto view_to_last = last_view * glm::inverse(view);
					auto view_to_last_normal = view_to_last;
					for (size_t i = 0; i < 3; i++)
						view_to_last_normal[3][i] = 0.0f;
					glm::vec3 cur_pos = m_track->render.camera_pos;

					s_rt.cur_cam_a = cam.a;
					s_rt.cur_cam_b = cam.b;
					s_rt.cur_cam_far = cam.far;
					s_rt.cur_cam_near = cam.near;
					s_rt.cur_cam_ratio = cam.ratio;
					s_rt.last_cam_proj = cam.proj;
					s_rt.view_normal_inv = cam.view_normal_inv;
					instance.cur_img_res->uploadDescSet(s_rt);

					s_sche.view_normal = cam.view_normal;
					s_sche.cur_cam_to_last = view_to_last;
					s_sche.cur_cam_to_last_normal = view_to_last_normal;
					s_sche.cur_cam_inv = glm::inverse(view);
					glm::mat4 cur_cam_inv_normal = glm::inverse(view);
					for (size_t i = 0; i < 3; i++)
						cur_cam_inv_normal[3][i] = 0.0f;
					s_sche.cur_cam_inv_normal = cur_cam_inv_normal;
					s_sche.last_cam_inv = glm::inverse(last_view);
					glm::mat4 last_cam_inv_normal = glm::inverse(last_view);
					for (size_t i = 0; i < 3; i++)
						last_cam_inv_normal[3][i] = 0.0f;
					s_sche.last_cam_inv_normal = last_cam_inv_normal;
					s_sche.path = step;
					instance.cur_img_res->uploadDescSet(s_sche);

					img.diffuse_to_wsi_screen_set.cam_a = cam.a;
					img.diffuse_to_wsi_screen_set.cam_b = cam.b;
					instance.cur_img_res->uploadDescSet(img.diffuse_to_wsi_screen_set);

					for (size_t i = 0; i < 6; i++) {
						auto &set = img.cube_depth_set;
						set.cur_view = view;
						set.cur_view_inv = glm::inverse(view);
						static const std::array<glm::vec3, 6> dir_table = {
							glm::vec3(1.0f, 0.0f, 0.0),
							glm::vec3(-1.0f, 0.0f, 0.0),
							glm::vec3(0.0f, 1.0f, 0.0),
							glm::vec3(0.0f, -1.0f, 0.0),
							glm::vec3(0.0f, 0.0f, 1.0),
							glm::vec3(0.0f, 0.0f, -1.0)
						};
						static const std::array<glm::vec3, 6> up_table = {
							glm::vec3(0.0f, 1.0f, 0.0),
							glm::vec3(0.0f, 1.0f, 0.0),
							glm::vec3(1.0f, 0.0f, 0.0),
							glm::vec3(1.0f, 0.0f, 0.0),
							glm::vec3(0.0f, 1.0f, 0.0),
							glm::vec3(0.0f, 1.0f, 0.0),
						};
						auto dir = glm::lookAtLH(glm::vec3(0.0f), dir_table.at(i), up_table.at(i));
						auto cube_view = dir * glm::translate(-cur_pos);
						auto last_cube_view = dir * glm::translate(-last_pos);
						set.cube_view.at(i) = cube_view;
						set.cube_view_inv.at(i) = glm::inverse(cube_view);
						set.last_cube_view.at(i) = last_cube_view;
						set.last_cube_view_inv.at(i) = glm::inverse(last_cube_view);
						set.cube_proj = glm::perspectiveLH_ZO<float>(sb::pi * 0.5, 1.0f, cam.near, cam.far);

						set.cur_cam_a = cam.a;
						set.cur_cam_b = cam.b;
						set.cur_cam_ratio = cam.ratio;
						set.cur_cam_proj = cam.proj;
						auto fb_ex = img.fb_depth_buffer.extent();
						set.fb_size = glm::vec2(fb_ex.x, fb_ex.y);

						instance.cur_img_res->uploadDescSet(set);
					}

					cmd.bind(m_opaque_env_shader);
					cmd.bind(m_opaque_env_shader, m_track->render.camera, 0);
					cmd.bind(m_opaque_env_shader, img.env_set, 1);
					cmd.draw(instance.screen_quad);
				}
			);

			cmd.render(img.depth_buffer_max_fb, {{0, 0}, instance.swapchain->extent()},
				0.0f,

				[&](auto &cmd){
					cmd.bind(m_depth_max_shader);
					cmd.bind(m_depth_max_shader, m_track->render.camera, 0);
					cmd.bind(m_depth_max_shader, m_track->entity.m_depth_object, 1);
					cmd.draw(m_track->entity.m_model);
				}
			);

			cmd.memoryBarrier(sb::PipelineStage::ColorAttachmentOutput | sb::PipelineStage::LateFragmentTests, sb::PipelineStage::FragmentShader, {},
				sb::Access::ColorAttachmentWrite | sb::Access::DepthStencilAttachmentWrite, sb::Access::ShaderRead);

			cmd.render(img.depth_inter_front_fb, {{0, 0}, instance.swapchain->extent()},
				1.0f,

				[&](auto &cmd){
					cmd.bind(m_depth_inter_front_shader);
					cmd.bind(m_depth_inter_front_shader, m_track->render.camera, 0);
					cmd.bind(m_depth_inter_front_shader, img.depth_inter_front_set, 1);
					cmd.bind(m_depth_inter_front_shader, m_track->entity.m_depth_object, 2);
					cmd.draw(m_track->entity.m_model);
				}
			);

			cmd.memoryBarrier(sb::PipelineStage::LateFragmentTests, sb::PipelineStage::FragmentShader, {},
				sb::Access::DepthStencilAttachmentWrite, sb::Access::ShaderRead);

			cmd.render(img.depth_inter_back_fb, {{0, 0}, instance.swapchain->extent()},
				1.0f,

				[&](auto &cmd){
					cmd.bind(m_depth_inter_back_shader);
					cmd.bind(m_depth_inter_back_shader, m_track->render.camera, 0);
					cmd.bind(m_depth_inter_back_shader, img.depth_inter_back_set, 1);
					cmd.bind(m_depth_inter_back_shader, m_track->entity.m_depth_object, 2);
					cmd.draw(m_track->entity.m_model);
				}
			);

			cmd.memoryBarrier(sb::PipelineStage::LateFragmentTests, sb::PipelineStage::FragmentShader, {},
				sb::Access::DepthStencilAttachmentWrite, sb::Access::ShaderRead);

			cmd.render(img.depth_to_fl_fb, {{0, 0}, img.fb_depth_buffer_raw_fl.extent()},
				[&](auto &cmd){
					cmd.bind(m_depth_to_fl_shader);
					cmd.bind(m_depth_to_fl_shader, img.depth_to_fl_set, 0);
					cmd.draw(instance.screen_quad);
				}
			);

			cmd.imageMemoryBarrier(sb::PipelineStage::ColorAttachmentOutput, sb::PipelineStage::Transfer, {},
				sb::Access::ColorAttachmentWrite, sb::Access::TransferRead,
				sb::Image::Layout::ShaderReadOnlyOptimal, sb::Image::Layout::TransferSrcOptimal, img.fb_depth_buffer_raw_fl_mips.at(0));

			{
				auto end = img.fb_depth_buffer_raw_fl_mips.end();
				for (auto it = img.fb_depth_buffer_raw_fl_mips.begin() + 1; it != end; it++) {
					auto &cur = *it;
					cmd.imageMemoryBarrier(sb::PipelineStage::Transfer, sb::PipelineStage::Transfer, {},
						sb::Access::TransferWrite, sb::Access::TransferRead,
						sb::Image::Layout::Undefined, sb::Image::Layout::TransferDstOptimal, cur);

					cmd.blit(*(it - 1), sb::Image::Layout::TransferSrcOptimal, (it - 1)->blitRegion({0, 0}, (it - 1)->extent()),
					cur, sb::Image::Layout::TransferDstOptimal, cur.blitRegion({0, 0}, cur.extent()), sb::Filter::Linear);

					cmd.imageMemoryBarrier(sb::PipelineStage::Transfer, sb::PipelineStage::Transfer, {},
						sb::Access::TransferWrite, sb::Access::TransferRead,
						sb::Image::Layout::TransferDstOptimal, sb::Image::Layout::TransferSrcOptimal, cur);

					cmd.imageMemoryBarrier(sb::PipelineStage::Transfer, sb::PipelineStage::Transfer, {},
						sb::Access::TransferWrite, sb::Access::TransferRead,
						sb::Image::Layout::TransferSrcOptimal, sb::Image::Layout::ShaderReadOnlyOptimal, *(it - 1));
				}
			}

			cmd.imageMemoryBarrier(sb::PipelineStage::Transfer, sb::PipelineStage::FragmentShader, {},
				sb::Access::TransferWrite, sb::Access::ShaderRead,
				sb::Image::Layout::TransferSrcOptimal, sb::Image::Layout::ShaderReadOnlyOptimal, *img.fb_depth_buffer_raw_fl_mips.rbegin());

			{
				size_t ndx = 0;
				for (auto &mip : img.fb_depth_buffer_fl_mips) {
					auto ex = mip.img.extent();
					cmd.setViewport({{0.0f, 0.0f}, {ex.x, ex.y}}, 0.0f, 1.0f);
					cmd.setScissor({{0, 0}, ex});
					cmd.render(mip.fb, {{0, 0}, ex},
						[&](auto &cmd){
							if (ndx == 0) {
								cmd.bind(m_first_depth_range);
								cmd.bind(m_first_depth_range, img.first_depth_range_in_fb, 0);
								cmd.draw(instance.screen_quad);
							} else {
								cmd.bind(m_compute_depth_range);
								cmd.bind(m_compute_depth_range, img.compute_depth_range_in_fb.at(ndx - 1), 0);
								cmd.draw(instance.screen_quad);
							}
						}
					);

					cmd.memoryBarrier(sb::PipelineStage::ColorAttachmentOutput, sb::PipelineStage::FragmentShader, {},
						sb::Access::ColorAttachmentWrite, sb::Access::ShaderRead);

					ndx++;
				}
			}

			/*{
				auto ex = img.cube_depth.extent();
				cmd.render(img.cube_depth_fb, {{0, 0}, ex},
					[&](auto &cmd){
						cmd.setViewport({{0.0f, 0.0f}, {ex.x, ex.y}}, 0.0f, 1.0f);
						cmd.setScissor({{0, 0}, ex});
						cmd.bind(m_cube_depth_shader);
						cmd.bind(m_cube_depth_shader, img.cube_depth_set, 0);
						cmd.draw(instance.screen_quad);
					}
				);
			}*/

			cmd.setViewport(viewport, 0.0f, 1.0f);
			cmd.setScissor({{0, 0}, instance.swapchain->extent()});

			cmd.memoryBarrier(sb::PipelineStage::ColorAttachmentOutput, sb::PipelineStage::FragmentShader, {},
				sb::Access::ColorAttachmentWrite, sb::Access::ShaderRead);

			for (auto &d : img.scheduling_set.random_sun_dir)
				d = sb::genDiffuseVector(*m_track, glm::normalize(glm::vec3(1.3, 3.0, 1.0)), 2000.0);

			for (auto &d : img.scheduling_set.random_diffuse)
				d = sb::genDiffuseVector(*m_track, glm::vec3(0.0f, 0.0f, 1.0f), 1.0);

			cmd.render(img.scheduling_fb, {{0, 0}, instance.swapchain->extent()},
				[&](auto &cmd){
					cmd.bind(m_scheduling_shader);
					cmd.bind(m_scheduling_shader, img.rt_set, 0);
					cmd.bind(m_scheduling_shader, img.env_set, 1);
					cmd.bind(m_scheduling_shader, img.scheduling_set, 2);
					cmd.draw(instance.screen_quad);
				}
			);

			cmd.memoryBarrier(sb::PipelineStage::ColorAttachmentOutput, sb::PipelineStage::FragmentShader, {},
				sb::Access::ColorAttachmentWrite, sb::Access::ShaderRead);

			cmd.render(img.buffer_to_wsi_screen_fbs.at(swapchain_img), {{0, 0}, instance.swapchain->extent()},
				[&](auto &cmd){
					cmd.bind(m_diffuse_to_wsi_screen);
					cmd.bind(m_diffuse_to_wsi_screen, img.diffuse_to_wsi_screen_set, 0);
					cmd.draw(instance.screen_quad);
				}
			);

			cmd.memoryBarrier(sb::PipelineStage::ColorAttachmentOutput, sb::PipelineStage::BottomOfPipe, {},
				sb::Access::ColorAttachmentWrite, sb::Access::MemoryRead);
		});

		instance.cur_img_res->transfer_unsafe.end();

		auto t_before_submit = std::chrono::high_resolution_clock::now();

		if (last_frame) {
			auto img_wait = std::pair {&img.swapchain_img_avail, sb::PipelineStage::ColorAttachmentOutput};
			auto last_render_wait = std::pair {&last_frame->render_done, sb::PipelineStage::TopOfPipe};
			instance.graphics.submit(img.render_done_fence,
				std::array{&img_wait, &last_render_wait},
				std::array{&instance.cur_img_res->transfer_cmd_buf, &img.cmd_prim},
				img.render_done);
		} else
			instance.graphics.submit(img.render_done_fence,
				std::pair {&img.swapchain_img_avail, sb::PipelineStage::ColorAttachmentOutput},
				std::array{&instance.cur_img_res->transfer_cmd_buf, &img.cmd_prim},
				img.render_done);

		auto t_before_present = std::chrono::high_resolution_clock::now();
		instance.graphics.present(img.render_done, instance.swapchain->images().at(swapchain_img));
		img.ever_rendered = true;

		auto t_next_frame = std::chrono::high_resolution_clock::now();
		last_frame = &img;
		instance.nextFrame();
		auto &next_img = images.at(instance.cur_img);
		if (next_img.ever_rendered) {
			next_img.render_done_fence.wait();
			next_img.render_done_fence.reset();
		}
		instance.cur_img_res->resetStagingOff();
		instance.cur_img_res->transfer_unsafe.begin(sb::CommandBuffer::Usage::OneTimeSubmit);

		step = (step + 1) % 3;

		auto t_end = std::chrono::high_resolution_clock::now();

		static_cast<void>(t_start);
		static_cast<void>(t_before_ac);
		static_cast<void>(t_begin_record);
		static_cast<void>(t_before_submit);
		static_cast<void>(t_before_present);
		static_cast<void>(t_next_frame);
		static_cast<void>(t_end);
		/*std::cout << "before_ac: " << std::chrono::duration_cast<std::chrono::duration<double>>(t_before_ac - t_start).count() <<
		", rec: " << std::chrono::duration_cast<std::chrono::duration<double>>(t_before_submit - t_begin_record).count() <<
		", submit: " << std::chrono::duration_cast<std::chrono::duration<double>>(t_before_present - t_before_submit).count() <<
		", present: " << std::chrono::duration_cast<std::chrono::duration<double>>(t_next_frame - t_before_present).count() <<
		", next_frame: " << std::chrono::duration_cast<std::chrono::duration<double>>(t_end - t_next_frame).count() <<
		", FRAME_LEN: " << std::chrono::duration_cast<std::chrono::duration<double>>(t_end - t_start).count() << std::endl;*/
	}
}

}