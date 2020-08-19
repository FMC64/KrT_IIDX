#pragma once

#include <chrono>
#include "../Observer.hpp"
#include "../../Input.hpp"
#include "util.hpp"

namespace Subtile {

class System;
class InstanceBase;
class SessionBase;

namespace Event {
namespace System {

class Socket;

class Observer : public Event::Observer::Cluster, private util::dep<sb::System&>
{
public:
	Observer(sb::System &system);
	~Observer(void) override;

public:
	class Input : public Cluster, private util::dep<sb::System&>
	{
	public:
		Input(sb::System &system);
		~Input(void) override;

		class Analog : public Cluster, public DescGen<Analog>, public Observer::GroupCb<Analog, std::tuple<std::string>, std::tuple<util::ref_wrapper<Subtile::Input::Analog>>, std::tuple<double>>
		{
		public:
			Analog(Input &input);
			~Analog(void) override;

		private:
			Input &m_input;
		} analog;

		class Button : public Cluster, public DescGen<Button>, public Observer::GroupCb<Button, std::tuple<std::string>, std::tuple<util::ref_wrapper<Subtile::Input::Button>>, std::tuple<bool>>
		{
		public:
			Button(Input &input);
			~Button(void) override;

			class Pressed : public Observer::Cluster, public DescGen<Pressed>, public Observer::GroupCb<Pressed, std::tuple<std::string>, std::tuple<util::ref_wrapper<Subtile::Input::Button>>, std::tuple<>>
			{
			public:
				Pressed(Input &input);
				~Pressed(void) override;

			private:
				Input &m_input;
			} pressed;

			class Released : public Observer::Cluster, public DescGen<Released>, public Observer::GroupCb<Released, std::tuple<std::string>, std::tuple<util::ref_wrapper<Subtile::Input::Button>>, std::tuple<>>
			{
			public:
				Released(Input &input);
				~Released(void) override;

			private:
				Input &m_input;
			} released;

		private:
			Input &m_input;
		} button;

		class Setter
		{
		public:
			Setter(Input &input) :
				m_input(input)
			{
			}
			~Setter(void)
			{
			}

			template <typename ...ArgsTypes>
			void addAnalog(const std::string &name, ArgsTypes &&...args) const
			{
				const auto [it, s] = m_input.m_inputs.emplace(name, new Subtile::Input::Analog(std::forward<ArgsTypes>(args)...));
				if (!s)
					throw std::runtime_error(std::string("Can't create analog input: '") + name + std::string("'"));
			}

			template <typename ...ArgsTypes>
			void addButton(const std::string &name, ArgsTypes &&...args) const
			{
				const auto [it, s] = m_input.m_inputs.emplace(name, new Subtile::Input::Button(std::forward<ArgsTypes>(args)...));
				if (!s)
					throw std::runtime_error(std::string("Can't create button input: '") + name + std::string("'"));
			}

		private:
			Input &m_input;
		};

	private:
		friend Analog;
		friend Button;
		friend InstanceBase;

		std::map<std::string, std::unique_ptr<sb::Input>> m_inputs;
		std::map<std::string, std::string> m_bindings;
		Observer::Cluster::Optimized m_input_update;

		void set(const std::function<void (const Setter &setter)> &binder);
		std::map<std::string, std::string> loadBindings(void) const;
		void saveBindings(void) const;
	} input;

	class Update : public Cluster, public DescGen<Update>, public Group<Update, std::tuple<>, std::tuple<double>>
	{
	public:
		Update(void);
		~Update(void) override;

	private:
		std::chrono::high_resolution_clock::time_point m_time_before;
	} update;

	class System
	{
	public:
		System(void);
		~System(void);

		class Quit
		{
		public:
			Quit(void);
			~Quit(void);

			void operator()(void);
			operator bool(void) const;

		private:
			bool m_quit;
		} quit;
	} system;

private:
	friend SessionBase;

	void updateEvents(void);
};

}
}
}