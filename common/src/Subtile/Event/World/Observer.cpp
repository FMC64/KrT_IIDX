#include <fstream>
#include "Observer.hpp"
#include "Subtile/class_impl.hpp"

namespace Subtile {
namespace Event {
namespace World {

BOOST_PP_EVAL2p8(
scc(Observer,
	scp::Observer(Subtile::Event::System::Observer &system) :
		system(system)
	{
		add(static_cast<Observer::Cluster&>(update));
	}

	scp::~Observer(void)
	{
	}

	void scp::updateEvents(void)
	{
		Event::Observer::Cluster::update();
	}
))

BOOST_PP_EVAL2p8(
scc(Observer::Update,
	scp::Update(void) :
		Observer::Group<Update, std::tuple<>, std::tuple<double>>([this](){
			auto now = std::chrono::high_resolution_clock::now();
			double res = std::chrono::duration<double>(now - m_time_before).count();
			m_time_before = now;
			return res;
		}),
		m_time_before(std::chrono::high_resolution_clock::now())
	{
	}

	scp::~Update(void)
	{
	}
))

}
}
}