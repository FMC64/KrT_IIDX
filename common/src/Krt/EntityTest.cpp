#include <iostream>
#include "EntityTest.hpp"
#include <glm/gtx/transform.hpp>

namespace Krt {

EntityTest::EntityTest(void) :
	entity1(add<EntityTest2>()),
	entity2(add<EntityTest2>()),
	m_shader(load(res.shaders().diffuse())),
	m_material(m_shader.material()),
	m_object(m_shader.object()),
	m_model(createModel())
{
	bind(world.events.system.input.button.pressed("quit"), [this](){
		//std::cout << "quit pressed" << std::endl;
		trigger(just_died, 14.2);
	});

	bind(world.events.system.input.button.released("quit"), [this](){
		//std::cout << "quit released" << std::endl;
		trigger(got_score, 7.92);
	});

	bind(world.render, m_shader.render(m_model, world.render.camera, m_material, m_object));

	bind(world.events.update, [this](auto &time){
		m_material.counter++;
		if (m_material.counter > 256) {
			m_material.counter = 0;
		}
		m_material.upload();

		m_angle += time;
		auto mat = glm::rotate((float)m_angle, glm::normalize(glm::vec3(1.0, 1.0, 1.0)));
		m_object.model_world = mat;
		m_object.upload();
	});
}

EntityTest::~EntityTest(void)
{
}

decltype(EntityTest::m_model) EntityTest::createModel(void)
{
	std::vector<decltype(m_model)::Triangle> triangles;
	auto gen_vtx = [this](){
		decltype(m_shader)::Model::Vertex res;
		for (size_t i = 0; i < 3; i++)
			res.in_vtx_pos[i] = world.srandf() * 5.0;
		for (size_t i = 0; i < 3; i++)
			res.in_vtx_normal[i] = world.srandf();
		res.in_vtx_normal = sb::math::normalize(res.in_vtx_normal);
		for (size_t i = 0; i < 2; i++)
			res.in_vtx_uv[i] = world.urandf();
		return res;
	};

	for (size_t i = 0; i < 1000; i++) {
		decltype(m_model)::Triangle tri {gen_vtx(), gen_vtx(), gen_vtx()};
		triangles.emplace_back(tri);
	}
	return m_shader.model(std::move(triangles));
}

}