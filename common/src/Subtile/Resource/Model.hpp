#pragma once

#include "File.hpp"

namespace Subtile {
namespace Resource {

class Model : public File
{
public:
	Model(void);
	~Model(void) override;
};

}
}