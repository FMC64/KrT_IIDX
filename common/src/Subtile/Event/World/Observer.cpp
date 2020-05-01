#include <fstream>
#include "Observer.hpp"

namespace Subtile {
namespace Event {
namespace World {

Observer::Observer(ISystem &system) :
	util::dep<ISystem&>(system),
	input(system)
{
	//add(lifetime);
	add(input);
	add(static_cast<Cluster&>(update));
}

Observer::~Observer(void)
{
}

/*Observer::Lifetime::Lifetime(void)
{
	Cluster::add(add);
}

Observer::Lifetime::~Lifetime(void)
{
}

Observer::Lifetime::Add::Add(void)
{
}

Observer::Lifetime::Add::~Add(void)
{
}

std::unique_ptr<Listener> Observer::Lifetime::Add::operator()(Entity &parent, std::unique_ptr<Entity> &to_add)
{
	return m_to_add.bind(parent, std::move(to_add));
}

void Observer::Lifetime::Add::update(void)
{
	while (m_to_add.getMap().size() > 0) {
		auto p = std::move(m_to_add.getMap().)
	}
}*/

Observer::Input::Input(ISystem &system) :
	util::dep<ISystem&>(system),
	analog(*this),
	button(*this)
{
	add(static_cast<Cluster&>(analog));
	add(static_cast<Cluster&>(button));
	add(m_input_update);
}

Observer::Input::~Input(void)
{
}

std::map<std::string, std::string> Observer::Input::loadBindings(void) const
{
	std::map<std::string, std::string> res;

	res.emplace("quit", "KEY_ESCAPE");

	return res;
}

Observer::Input::Analog::Analog(Input &input) :
	Group<Analog, std::tuple<std::string>, std::tuple<util::ref_wrapper<Subtile::Input::Analog>>, std::tuple<double>>(
	[this](const std::string &inputName){
		return util::ref_wrapper(dynamic_cast<Subtile::Input::Analog&>(*m_input.m_inputs.at(inputName)));
	}, [](const util::ref_wrapper<Subtile::Input::Analog> &input) {
		return input.get().getState();
	}, [this](void) -> auto& {
		return m_input.m_input_update;
	}),
	m_input(input)
{
}

Observer::Input::Analog::~Analog(void)
{
}

Observer::Input::Button::Button(Input &input) :
	Group<Button, std::tuple<std::string>, std::tuple<util::ref_wrapper<Subtile::Input::Button>>, std::tuple<bool>>(
	[this](const std::string &inputName){
		return util::ref_wrapper(dynamic_cast<Subtile::Input::Button&>(*m_input.m_inputs.at(inputName)));
	}, [](const util::ref_wrapper<Subtile::Input::Button> &input) {
		return input.get().getState();
	}, [this](void) -> auto& {
		return m_input.m_input_update;
	}),
	pressed(input),
	released(input),
	m_input(input)
{
	add(static_cast<Observer::Cluster&>(pressed));
	add(static_cast<Observer::Cluster&>(released));
}

Observer::Input::Button::~Button(void)
{
}

Observer::Input::Button::Pressed::Pressed(Input &input) :
	Group<Pressed, std::tuple<std::string>, std::tuple<util::ref_wrapper<Subtile::Input::Button>>, std::tuple<>>(
	[this](const std::string &inputName){
		return util::ref_wrapper(dynamic_cast<Subtile::Input::Button&>(*m_input.m_inputs.at(inputName)));
	}, [](const util::ref_wrapper<Subtile::Input::Button> &input) {
		if (input.get().isPressed())
			return std::optional(std::make_tuple());
		else
			return std::optional<std::tuple<>>();
	}, [this](void) -> auto& {
		return m_input.m_input_update;
	}),
	m_input(input)
{
}

Observer::Input::Button::Pressed::~Pressed(void)
{
}

Observer::Input::Button::Released::Released(Input &input) :
	Group<Released, std::tuple<std::string>, std::tuple<util::ref_wrapper<Subtile::Input::Button>>, std::tuple<>>(
	[this](const std::string &inputName){
		return util::ref_wrapper(dynamic_cast<Subtile::Input::Button&>(*m_input.m_inputs.at(inputName)));
	}, [](const util::ref_wrapper<Subtile::Input::Button> &input) {
		if (input.get().isReleased())
			return std::optional(std::make_tuple());
		else
			return std::optional<std::tuple<>>();
	}, [this](void) -> auto& {
		return m_input.m_input_update;
	}),
	m_input(input)
{
}

Observer::Input::Button::Released::~Released(void)
{
}

Observer::Update::Update(void) :
	Group<Update, std::tuple<>, std::tuple<double>>([this](){
		auto now = std::chrono::high_resolution_clock::now();
		double res = std::chrono::duration<double>(now - m_time_before).count();
		m_time_before = now;
		return res;
	}),
	m_time_before(std::chrono::high_resolution_clock::now())
{
}

Observer::Update::~Update(void)
{
}

Observer::System::System(void)
{
}

Observer::System::~System(void)
{
}

Observer::System::Quit::Quit(void) :
	m_quit(false)
{
}

Observer::System::Quit::~Quit(void)
{
}

void Observer::System::Quit::operator()(void)
{
	m_quit = true;
}

Observer::System::Quit::operator bool(void) const
{
	return m_quit;
}

}
}
}