#pragma once

#include <array>
#include <map>
#include "common/model_registry.hpp"
#include "physics/radioactivity/boundary/radioactivity_boundary_model.hpp"
#include "properties/properties_system.hpp"

namespace trimuse
{

class RadioactivityBoundaryManager
{
public:
    explicit RadioactivityBoundaryManager(const std::vector<RadioactivityBoundaryConfig>& configs);

    void update(const PropertiesFieldData& properties);

    void mark_coupled(PropertiesSystem& properties, const std::string& field) const;

    bool coupled(const PropertiesSystem& properties, const std::string& field) const;

    bool required_no_change(int zone, RadioactivityBoundaryPart part) const;

    void assembled(int zone, RadioactivityBoundaryPart part);

    std::vector<int> zones() const;

    const RadioactivityBoundaryValues& values(int zone) const;

    bool essential(int zone) const;

private:
    struct Assignment
    {
        std::unique_ptr<RadioactivityBoundaryModel> model;

        std::vector<std::string> required;

        RadioactivityBoundaryValues values;

        std::array<bool, 3> dirty{true, true, true};
    };

    ModelRegistry<RadioactivityBoundaryModel> registry_;

    std::map<int, Assignment> models_;
};

}
