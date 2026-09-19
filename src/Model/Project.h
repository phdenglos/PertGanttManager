#ifndef PROJECT_H
#define PROJECT_H

#include <Core/Core.h>
#include <string>
#include <vector>
#include "Task.h"
#include "Resource.h"

using namespace Upp;

class Project {
public:
    int id;
    String name;
    String description;
    Time createdAt;
    Time updatedAt;

    // Collections
    Vector<Task> tasks;
    Vector<Resource> resources;

    Project();
    Project(int id, const String& name, const String& description = "");

    // Serialization methods
    void ToJson(Json& json) const;
    void FromJson(const Json& json);

    // Task management
    Task* GetTaskById(int taskId);
    const Task* GetTaskById(int taskId) const;
    Task* GetTaskByName(const String& taskName);
    const Task* GetTaskByName(const String& taskName) const;
    int AddTask(const Task& task);
    bool UpdateTask(const Task& task);
    bool RemoveTask(int taskId);
    bool RemoveTaskByName(const String& taskName);

    // Resource management
    Resource* GetResourceById(int resourceId);
    const Resource* GetResourceById(int resourceId) const;
    Resource* GetResourceByName(const String& resourceName);
    const Resource* GetResourceByName(const String& resourceName) const;
    int AddResource(const Resource& resource);
    bool UpdateResource(const Resource& resource);
    bool RemoveResource(int resourceId);
    bool RemoveResourceByName(const String& resourceName);

    // Get all tasks that depend on a specific task
    Vector<int> GetDependentTasks(int taskId) const;

    // Get all tasks that a specific task depends on
    Vector<int> GetTaskDependencies(int taskId) const;

    // Calculate project dates
    Time GetEarliestStartDate() const;
    Time GetLatestEndDate() const;

    // Calculate total workload
    double GetTotalWorkload() const;
    double GetCompletedWorkload() const;
    double GetRemainingWorkload() const;

    // Calculate overall progress
    double GetOverallProgress() const;

    // Validation
    bool IsValid() const;

    // Comparison operators
    bool operator==(const Project& other) const;
    bool operator!=(const Project& other) const;
};

#endif // PROJECT_H
