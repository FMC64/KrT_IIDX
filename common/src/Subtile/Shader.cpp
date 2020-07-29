#include "Shader.hpp"

namespace Subtile {

const std::set<Shader::Sbi>& Shader::getSbi(void)
{
	static const std::set<Shader::Sbi> res {
		Shader::Sbi::Vulkan
	};

	return res;
}

template <>
void Shader::Type::Vec<Shader::Type::Bool, 2>::createVertexInput(Shader::VertexInput::Creator &creator) const
{
	creator.addAttr(this, VertexInput::Format::Bvec2);
}
template <>
void Shader::Type::Vec<Shader::Type::Int, 2>::createVertexInput(Shader::VertexInput::Creator &creator) const
{
	creator.addAttr(this, VertexInput::Format::Ivec2);
}
template <>
void Shader::Type::Vec<Shader::Type::Uint, 2>::createVertexInput(Shader::VertexInput::Creator &creator) const
{
	creator.addAttr(this, VertexInput::Format::Uvec2);
}
template <>
void Shader::Type::Vec<Shader::Type::Float, 2>::createVertexInput(Shader::VertexInput::Creator &creator) const
{
	creator.addAttr(this, VertexInput::Format::Vec2);
}
template <>
void Shader::Type::Vec<Shader::Type::Double, 2>::createVertexInput(Shader::VertexInput::Creator &creator) const
{
	creator.addAttr(this, VertexInput::Format::Dvec2);
}

template <>
void Shader::Type::Vec<Shader::Type::Bool, 3>::createVertexInput(Shader::VertexInput::Creator &creator) const
{
	creator.addAttr(this, VertexInput::Format::Bvec3);
}
template <>
void Shader::Type::Vec<Shader::Type::Int, 3>::createVertexInput(Shader::VertexInput::Creator &creator) const
{
	creator.addAttr(this, VertexInput::Format::Ivec3);
}
template <>
void Shader::Type::Vec<Shader::Type::Uint, 3>::createVertexInput(Shader::VertexInput::Creator &creator) const
{
	creator.addAttr(this, VertexInput::Format::Uvec3);
}
template <>
void Shader::Type::Vec<Shader::Type::Float, 3>::createVertexInput(Shader::VertexInput::Creator &creator) const
{
	creator.addAttr(this, VertexInput::Format::Vec3);
}
template <>
void Shader::Type::Vec<Shader::Type::Double, 3>::createVertexInput(Shader::VertexInput::Creator &creator) const
{
	creator.addAttr(this, VertexInput::Format::Dvec3);
}

template <>
void Shader::Type::Vec<Shader::Type::Bool, 4>::createVertexInput(Shader::VertexInput::Creator &creator) const
{
	creator.addAttr(this, VertexInput::Format::Bvec4);
}
template <>
void Shader::Type::Vec<Shader::Type::Int, 4>::createVertexInput(Shader::VertexInput::Creator &creator) const
{
	creator.addAttr(this, VertexInput::Format::Ivec4);
}
template <>
void Shader::Type::Vec<Shader::Type::Uint, 4>::createVertexInput(Shader::VertexInput::Creator &creator) const
{
	creator.addAttr(this, VertexInput::Format::Uvec4);
}
template <>
void Shader::Type::Vec<Shader::Type::Float, 4>::createVertexInput(Shader::VertexInput::Creator &creator) const
{
	creator.addAttr(this, VertexInput::Format::Vec4);
}
template <>
void Shader::Type::Vec<Shader::Type::Double, 4>::createVertexInput(Shader::VertexInput::Creator &creator) const
{
	creator.addAttr(this, VertexInput::Format::Dvec4);
}

}