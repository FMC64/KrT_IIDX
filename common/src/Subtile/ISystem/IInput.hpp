#pragma once

#include <vector>
#include <string>
#include <map>

namespace Subtile {
namespace System {

class IInput
{
public:
	virtual ~IInput(void) = default;

	virtual const std::string& getName(void) const;

private:
	void scan(void);
};

}
}