#pragma once

#include "Track.hpp"
#include "res.resdecl.hpp"

namespace Krt {

class EntityTest2;

class EntityTest : public Track::Entity
{
public:
	EntityTest(void);
	~EntityTest(void) override;

	Event<double> just_died;
	Event<size_t> got_score;

private:
	EntityTest2 &entity1;
	EntityTest2 &entity2;
	decltype(res.shaders().diffuse().loaded()) m_shader;
	decltype(m_shader.material()) m_material;
	decltype(m_shader.object()) m_object;
	decltype(m_shader.model()) m_model;

	decltype(m_model) createModel(void);
};

}

#include "EntityTest2.hpp"
