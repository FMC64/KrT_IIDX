#include "Track.hpp"
#include "Subtile/Binding.hpp"

#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>

namespace Krt {

Track::Track(void) :
	entity(add<EntityTest>())
{
	/*bind(entity.just_died(), [](const double &val){
		std::cout << "Just died event: " << val << std::endl;
	});

	bind(entity.got_score(), [this](const size_t &score){
		std::cout << "Got score event: " << score << std::endl;
		events.system.quit();
	});*/

	/*std::cout << sizeof(Subtile::Binding::Source::Strong<int>::Element) << std::endl;
	std::cout << sizeof(Subtile::Binding::Source::Strong<int>::Multiple::Element) << std::endl;

	Subtile::Binding::Dependency::Socket deps;
	Subtile::Binding::Source::Strong<int> socket;

	socket.bind(deps, 10);*/

}

Track::~Track(void)
{
}

}