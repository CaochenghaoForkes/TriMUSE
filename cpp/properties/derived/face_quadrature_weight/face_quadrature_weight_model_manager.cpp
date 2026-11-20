#include "properties/derived/face_quadrature_weight/face_quadrature_weight_model_manager.hpp"
#include "properties/derived/face_quadrature_weight/undeformed.hpp"

namespace trimuse
{

FaceQuadratureWeightModel::~FaceQuadratureWeightModel() = default;

void FaceQuadratureWeightModelManager::register_models()
{
    registry_.register_model<UndeformedFaceQuadratureWeight>("common.undeformed", {"precomputed.initial_face_quadrature_weight"});
}

void FaceQuadratureWeightModelManager::assign(int face_attribute, const std::string& model_name, const ModelParameterMap& parameters)
{
    face_model_map_[face_attribute] = registry_.registration(model_name).factory(parameters);
}

StateValue FaceQuadratureWeightModelManager::evaluate(const PropertiesStatePoint& point) const
{
    return face_model_map_.at(point.face_attribute)->evaluate(point);
}

const std::vector<std::string>& FaceQuadratureWeightModelManager::required_states(const std::string& model_name) const
{
    return registry_.registration(model_name).required_states;
}

}
