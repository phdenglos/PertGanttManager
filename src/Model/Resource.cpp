#include "Resource.h"

using namespace Upp;

Resource::Resource() : id(0), projectId(0), name(""), maxUtilizationRate(100.0), description("") {}

Resource::Resource(int id, int projectId, const String& name, double maxUtilizationRate, const String& description)
    : id(id), projectId(projectId), name(name), maxUtilizationRate(maxUtilizationRate), description(description) {
    if (maxUtilizationRate < 0) maxUtilizationRate = 0;
    if (maxUtilizationRate > 100) maxUtilizationRate = 100;
}

void Resource::ToJson(Json& json) const {
    json("id", id);
    json("projectId", projectId);
    json("name", name);
    json("maxUtilizationRate", maxUtilizationRate);
    json("description", description);
}

void Resource::FromJson(const Json& json) {
    id = json("id", 0);
    projectId = json("projectId", 0);
    name = json("name", "");
    maxUtilizationRate = json("maxUtilizationRate", 100.0);
    description = json("description", "");
    
    // Ensure valid range
    if (maxUtilizationRate < 0) maxUtilizationRate = 0;
    if (maxUtilizationRate > 100) maxUtilizationRate = 100;
}

bool Resource::operator==(const Resource& other) const {
    return id == other.id && 
           projectId == other.projectId && 
           name == other.name &&
           maxUtilizationRate == other.maxUtilizationRate &&
           description == other.description;
}

bool Resource::operator!=(const Resource& other) const {
    return !(*this == other);
}

bool Resource::IsValid() const {
    return !name.IsEmpty() && 
           maxUtilizationRate >= 0 && 
           maxUtilizationRate <= 100;
}
