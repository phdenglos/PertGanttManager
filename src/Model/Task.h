#ifndef TASK_H
#define TASK_H

#include <Core/Core.h>
#include <string>
#include <vector>

using namespace Upp;

class Task {
public:
    int id;
    int projectId;
    String name;
    double workload; // in person-days
    int duration; // in days
    Time startDate;
    Time endDate;
    double progress; // Percentage (0-100)
    int priority; // 1-5, 1 is highest priority
    Color color;
    double xPosition; // X position for diagram
    double yPosition; // Y position for diagram

    // Resource assignments (resourceId, allocationRate)
    Vector<std::pair<int, double>> resourceAssignments;

    // Dependencies (taskIds this task depends on)
    Vector<int> dependencies;

    Task();
    Task(int id, int projectId, const String& name, double workload, int duration,
         const Time& startDate, const Time& endDate, double progress, int priority,
         Color color, double xPosition = 0.0, double yPosition = 0.0);

    // Serialization methods
    void ToJson(Json& json) const;
    void FromJson(const Json& json);

    // Add/Remove dependencies
    void AddDependency(int taskId);
    void RemoveDependency(int taskId);
    bool HasDependency(int taskId) const;

    // Add/Remove resource assignments
    void AddResourceAssignment(int resourceId, double allocationRate);
    void RemoveResourceAssignment(int resourceId);
    void UpdateResourceAssignment(int resourceId, double allocationRate);
    double GetResourceAllocationRate(int resourceId) const;

    // Calculate remaining workload based on progress
    double GetRemainingWorkload() const;

    // Calculate actual duration based on progress
    int GetActualDuration() const;

    // Calculate end date based on start date and duration
    static Time CalculateEndDate(const Time& startDate, int duration);

    // Comparison operators
    bool operator==(const Task& other) const;
    bool operator!=(const Task& other) const;

    // Validation
    bool IsValid() const;

    // Priority comparison (lower number = higher priority)
    bool HasHigherPriorityThan(const Task& other) const;
};

#endif // TASK_H
