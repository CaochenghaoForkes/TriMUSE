#include "common/required_state.hpp"

namespace trimuse
{

RequiredState::RequiredState(const char* name, bool reference)
    :
    RequiredState(std::string(name), reference)
{
}

RequiredState::RequiredState(const std::string& name, bool reference)
    :
    name(name),
    reference(reference)
{
}

bool RequiredState::operator==(const std::string& other) const
{
    return name == other;
}

}
