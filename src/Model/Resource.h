#ifndef RESOURCE_H
#define RESOURCE_H

#include <Core/Core.h>
#include <string>

using namespace Upp;

class Resource {
public:
    int id;
    int projectId;
    String name;
    double maxUtilizationRate; // Percentage (0-100)
    String description;

    Resource();
    Resource(int id, int projectId, const String& name, double maxUtilizationRate, const String& description = "");

    // Serialization methods
    void ToJson(Json& json) const;
    void FromJson(const Json& json);

    // Comparison operators
    bool operator==(const Resource& other) const;
    bool operator!=(const Resource& other) const;

    // Validation
    bool IsValid() const;
};

#endif // RESOURCE_H
