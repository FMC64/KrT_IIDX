#pragma once

#include <vector>
#include <functional>
#include "util.hpp"

namespace Subtile {

class SessionBase;

namespace Screen {

class Section
{
	class TopBase
	{
	public:
		TopBase(void);
		~TopBase(void);
	};

	template <typename SectionDerived>
	class Top : public SectionDerived, public TopBase
	{
	public:
		template <typename ...Args>
		Top(Args &&...args) :
			SectionDerived(std::forward<Args>(args)...)
		{
		}
		~Top(void) override
		{
		}

	private:
		void _section_security(void) override
		{
		}
	};

public:
	template <typename ToUserDerive>
	class UserDerive
	{
	public:
		template <class FinalType>
		class Gen : public ToUserDerive
		{
		public:
			template <typename ...Args>
			Gen(Args &&...args) :
				ToUserDerive(std::forward<Args>(args)...)
			{
			}
			~Gen(void) override
			{
			}

			using Class = Top<FinalType>;
		};
	};

	Section(void);
	virtual ~Section(void) = 0;

protected:
	virtual void onRender(void);

private:
	friend SessionBase;

	virtual void _section_security(void) = 0;

	static thread_local util::stack<std::reference_wrapper<Section>> m_stack;
	std::vector<std::reference_wrapper<Section>> m_children;

	void render(void);
};

}
}