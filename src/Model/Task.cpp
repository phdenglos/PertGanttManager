#include "Task.h"

using namespace Upp;

Task::Task() 
    : id(0), projectId(0), name(""), workload(1.0), duration(1), 
      progress(0.0), priority(3), color(White()), xPosition(0.0), yPosition(0.0) {
    startDate = Null;
    endDate = Null;
}

Task::Task(int id, int projectId, const String& name, double workload, int duration,
           const Time& startDate, const Time& endDate, double progress, int priority,
           Color color, double xPosition, double yPosition)
    : id(id), projectId(projectId), name(name), workload(workload), duration(duration),
      startDate(startDate), endDate(endDate), progress(progress), priority(priority),
      color(color), xPosition(xPosition), yPosition(yPosition) {
    
    if (progress < 0) progress = 0;
    if (progress > 100) progress = 100;
    if (priority < 1) priority = 1;
    if (priority > 5) priority = 5;
    if (workload < 0) workload = 0;
    if (duration < 0) duration = 0;
}

void Task::ToJson(Json& json) const {
    json("id", id);
    json("projectId", projectId);
    json("name", name);
    json("workload", workload);
    json("duration", duration);
    json("startDate", startDate.ToString());
    json("endDate", endDate.ToString());
    json("progress", progress);
    json("priority", priority);
    json("color", color.ToString());
    json("xPosition", xPosition);
    json("yPosition", yPosition);
    
    // Resource assignments
    JsonArray resourceArray;
    for (const auto& assignment : resourceAssignments) {
        Json resourceJson;
        resourceJson("resourceId", assignment.first);
        resourceJson("allocationRate", assignment.second);
        resourceArray.Add(resourceJson);
    }
    json("resourceAssignments", resourceArray);
    
    // Dependencies
    JsonArray dependencyArray;
    for (int depId : dependencies) {
        dependencyArray.Add(depId);
    }
    json("dependencies", dependencyArray);
}

void Task::FromJson(const Json& json) {
    id = json("id", 0);
    projectId = json("projectId", 0);
    name = json("name", "");
    workload = json("workload", 1.0);
    duration = json("duration", 1);
    
    String startDateStr = json("startDate", "");
    String endDateStr = json("endDate", "");
    
    if (!startDateStr.IsEmpty()) startDate = Time::FromString(startDateStr);
    else startDate = Null;
    
    if (!endDateStr.IsEmpty()) endDate = Time::FromString(endDateStr);
    else endDate = Null;
    
    progress = json("progress", 0.0);
    priority = json("priority", 3);
    
    String colorStr = json("color", "#FFFFFFFF");
    color = Color::FromString(colorStr);
    
    xPosition = json("xPosition", 0.0);
    yPosition = json("yPosition", 0.0);
    
    // Resource assignments
    JsonArray resourceArray = json("resourceAssignments");
    for (const Json& resourceJson : resourceArray) {
        int resourceId = resourceJson("resourceId", 0);
        double allocationRate = resourceJson("allocationRate", 100.0);
        resourceAssignments.Add(std::make_pair(resourceId, allocationRate));
    }
    
    // Dependencies
    JsonArray dependencyArray = json("dependencies");
    for (const Json& depJson : dependencyArray) {
        dependencies.Add(depJson.GetInt());
    }
    
    // Validate ranges
    if (progress < 0) progress = 0;
    if (progress > 100) progress = 100;
    if (priority < 1) priority = 1;
    if (priority > 5) priority = 5;
    if (workload < 0) workload = 0;
    if (duration < 0) duration = 0;
}

void Task::AddDependency(int taskId) {
    if (!HasDependency(taskId) && taskId != id) {
        dependencies.Add(taskId);
    }
}

void Task::RemoveDependency(int taskId) {
    int index = dependencies.Find(taskId);
    if (index >= 0) {
        dependencies.Remove(index);
    }
}

bool Task::HasDependency(int taskId) const {
    return dependencies.Find(taskId) >= 0;
}

void Task::AddResourceAssignment(int resourceId, double allocationRate) {
    if (allocationRate < 0) allocationRate = 0;
    if (allocationRate > 100) allocationRate = 100;
    
    // Check if already exists
    for (auto& assignment : resourceAssignments) {
        if (assignment.first == resourceId) {
            assignment.second = allocationRate;
            return;
        }
    }
    resourceAssignments.Add(std::make_pair(resourceId, allocationRate));
}

void Task::RemoveResourceAssignment(int resourceId) {
    for (int i = 0; i < resourceAssignments.GetCount(); i++) {
        if (resourceAssignments[i].first == resourceId) {
            resourceAssignments.Remove(i);
            return;
        }
    }
}

void Task::UpdateResourceAssignment(int resourceId, double allocationRate) {
    if (allocationRate < 0) allocationRate = 0;
    if (allocationRate > 100) allocationRate = 100;
    
    for (auto& assignment : resourceAssignments) {
        if (assignment.first == resourceId) {
            assignment.second = allocationRate;
            return;
        }
    }
    // If not found, add it
    resourceAssignments.Add(std::make_pair(resourceId, allocationRate));
}

double Task::GetResourceAllocationRate(int resourceId) const {
    for (const auto& assignment : resourceAssignments) {
        if (assignment.first == resourceId) {
            return assignment.second;
        }
    }
    return 0.0;
}

double Task::GetRemainingWorkload() const {
    return workload * (1.0 - progress / 100.0);
}

int Task::GetActualDuration() const {
    if (progress <= 0) return 0;
    return static_cast<int>(duration * (progress / 100.0));
}

Time Task::CalculateEndDate(const Time& startDate, int duration) {
    if (startDate.IsNull() || duration <= 0) return Null;
    return startDate + duration * 24 * 3600; // Add days in seconds
}

bool Task::operator==(const Task& other) const {
    return id == other.id && 
           projectId == other.projectId && 
           name == other.name &&
           workload == other.workload &&
           duration == other.duration &&
           startDate == other.startDate &&
           endDate == other.endDate &&
           progress == other.progress &&
           priority == other.priority &&
           color == other.color &&
           xPosition == other.xPosition &&
           yPosition == other.yPosition;
}

bool Task::operator!=(const Task& other) const {
    return !(*this == other);
}

bool Task::IsValid() const {
    return !name.IsEmpty() && 
           workload >= 0 && 
           duration >= 0 && 
           progress >= 0 && progress <= 100 &&
           priority >= 1 && priority <= 5;
}

bool Task::HasHigherPriorityThan(const Task& other) const {
    return priority < other.priority;
}
