#include "Track.hpp"
#include "Subtile/Binding.hpp"

#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <array>

#include <glm/gtx/projection.hpp>

namespace Krt {

Track::Track(void) :
	render(instance),
	entity(add<EntityTest>())
{
	bind(entity.just_died, [](const double &){
		//std::cout << "Just died event: " << val << std::endl;
	});

	bind(entity.got_score, [this](const size_t &){
		//std::cout << "Got score event: " << score << std::endl;
		trigger(done);
	});
}

Track::~Track(void)
{
	destroy_children();
}

void Track::Render::render(sb::CommandBuffer::Record::RenderPass &cmd)
{
	const double near = 0.1, far = 1000.0,
	ratio = static_cast<double>(m_instance.swapchain->extent().x) / static_cast<double>(m_instance.swapchain->extent().y),
	fov = 120.0 * (sb::pi / 180.0);
	auto proj = glm::perspectiveLH_ZO<float>(fov, ratio, near, far);
	proj[1][1] *= -1.0;

	auto view = glm::lookAtLH(glm::vec3(0.0, 0.0, -7.0), glm::vec3(0.0), glm::vec3(0.0, 1.0, 0.0));
	camera.vp = proj * view;
	camera.view = view;
	camera.view_normal = view;
	for (size_t i = 0; i < 3; i++)
		camera.view_normal[3][i] = 0.0f;
	camera.proj = proj;
	camera.inv_proj = glm::inverse(proj);
	camera.near = near;
	camera.far = far;
	camera.a = far / (far - near);
	camera.b = -(far * near) / (far - near);
	camera.ratio = glm::vec2(ratio, -1.0) * glm::vec2(std::tan(fov / 2.0));
	m_instance.cur_img_res->uploadDescSet(camera);
	static_cast<sb::RenderList&>(*this).draw(cmd);
}

}