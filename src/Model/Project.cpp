#include "Project.h"

using namespace Upp;

Project::Project() : id(0), name(""), description("") {
    createdAt = Null;
    updatedAt = Null;
}

Project::Project(int id, const String& name, const String& description)
    : id(id), name(name), description(description) {
    createdAt = Time::GetCurrent();
    updatedAt = createdAt;
}

void Project::ToJson(Json& json) const {
    json("id", id);
    json("name", name);
    json("description", description);
    json("createdAt", createdAt.ToString());
    json("updatedAt", updatedAt.ToString());
    
    // Tasks
    JsonArray taskArray;
    for (const Task& task : tasks) {
        Json taskJson;
        task.ToJson(taskJson);
        taskArray.Add(taskJson);
    }
    json("tasks", taskArray);
    
    // Resources
    JsonArray resourceArray;
    for (const Resource& resource : resources) {
        Json resourceJson;
        resource.ToJson(resourceJson);
        resourceArray.Add(resourceJson);
    }
    json("resources", resourceArray);
}

void Project::FromJson(const Json& json) {
    id = json("id", 0);
    name = json("name", "");
    description = json("description", "");
    
    String createdAtStr = json("createdAt", "");
    String updatedAtStr = json("updatedAt", "");
    
    if (!createdAtStr.IsEmpty()) createdAt = Time::FromString(createdAtStr);
    else createdAt = Null;
    
    if (!updatedAtStr.IsEmpty()) updatedAt = Time::FromString(updatedAtStr);
    else updatedAt = Null;
    
    // Tasks
    tasks.Clear();
    JsonArray taskArray = json("tasks");
    for (const Json& taskJson : taskArray) {
        Task task;
        task.FromJson(taskJson);
        tasks.Add(task);
    }
    
    // Resources
    resources.Clear();
    JsonArray resourceArray = json("resources");
    for (const Json& resourceJson : resourceArray) {
        Resource resource;
        resource.FromJson(resourceJson);
        resources.Add(resource);
    }
}

Task* Project::GetTaskById(int taskId) {
    for (Task& task : tasks) {
        if (task.id == taskId) {
            return &task;
        }
    }
    return nullptr;
}

const Task* Project::GetTaskById(int taskId) const {
    for (const Task& task : tasks) {
        if (task.id == taskId) {
            return &task;
        }
    }
    return nullptr;
}

Task* Project::GetTaskByName(const String& taskName) {
    for (Task& task : tasks) {
        if (task.name == taskName) {
            return &task;
        }
    }
    return nullptr;
}

const Task* Project::GetTaskByName(const String& taskName) const {
    for (const Task& task : tasks) {
        if (task.name == taskName) {
            return &task;
        }
    }
    return nullptr;
}

int Project::AddTask(const Task& task) {
    // Generate new ID if not set
    int newId = task.id;
    if (newId <= 0) {
        newId = 1;
        for (const Task& existingTask : tasks) {
            if (existingTask.id >= newId) {
                newId = existingTask.id + 1;
            }
        }
    }
    
    Task newTask = task;
    newTask.id = newId;
    newTask.projectId = id;
    tasks.Add(newTask);
    updatedAt = Time::GetCurrent();
    return newId;
}

bool Project::UpdateTask(const Task& task) {
    for (Task& existingTask : tasks) {
        if (existingTask.id == task.id) {
            existingTask = task;
            existingTask.projectId = id;
            updatedAt = Time::GetCurrent();
            return true;
        }
    }
    return false;
}

bool Project::RemoveTask(int taskId) {
    for (int i = 0; i < tasks.GetCount(); i++) {
        if (tasks[i].id == taskId) {
            tasks.Remove(i);
            updatedAt = Time::GetCurrent();
            return true;
        }
    }
    return false;
}

bool Project::RemoveTaskByName(const String& taskName) {
    for (int i = 0; i < tasks.GetCount(); i++) {
        if (tasks[i].name == taskName) {
            tasks.Remove(i);
            updatedAt = Time::GetCurrent();
            return true;
        }
    }
    return false;
}

Resource* Project::GetResourceById(int resourceId) {
    for (Resource& resource : resources) {
        if (resource.id == resourceId) {
            return &resource;
        }
    }
    return nullptr;
}

const Resource* Project::GetResourceById(int resourceId) const {
    for (const Resource& resource : resources) {
        if (resource.id == resourceId) {
            return &resource;
        }
    }
    return nullptr;
}

Resource* Project::GetResourceByName(const String& resourceName) {
    for (Resource& resource : resources) {
        if (resource.name == resourceName) {
            return &resource;
        }
    }
    return nullptr;
}

const Resource* Project::GetResourceByName(const String& resourceName) const {
    for (const Resource& resource : resources) {
        if (resource.name == resourceName) {
            return &resource;
        }
    }
    return nullptr;
}

int Project::AddResource(const Resource& resource) {
    // Generate new ID if not set
    int newId = resource.id;
    if (newId <= 0) {
        newId = 1;
        for (const Resource& existingResource : resources) {
            if (existingResource.id >= newId) {
                newId = existingResource.id + 1;
            }
        }
    }
    
    Resource newResource = resource;
    newResource.id = newId;
    newResource.projectId = id;
    resources.Add(newResource);
    updatedAt = Time::GetCurrent();
    return newId;
}

bool Project::UpdateResource(const Resource& resource) {
    for (Resource& existingResource : resources) {
        if (existingResource.id == resource.id) {
            existingResource = resource;
            existingResource.projectId = id;
            updatedAt = Time::GetCurrent();
            return true;
        }
    }
    return false;
}

bool Project::RemoveResource(int resourceId) {
    for (int i = 0; i < resources.GetCount(); i++) {
        if (resources[i].id == resourceId) {
            resources.Remove(i);
            updatedAt = Time::GetCurrent();
            return true;
        }
    }
    return false;
}

bool Project::RemoveResourceByName(const String& resourceName) {
    for (int i = 0; i < resources.GetCount(); i++) {
        if (resources[i].name == resourceName) {
            resources.Remove(i);
            updatedAt = Time::GetCurrent();
            return true;
        }
    }
    return false;
}

Vector<int> Project::GetDependentTasks(int taskId) const {
    Vector<int> dependentTasks;
    for (const Task& task : tasks) {
        if (task.HasDependency(taskId)) {
            dependentTasks.Add(task.id);
        }
    }
    return dependentTasks;
}

Vector<int> Project::GetTaskDependencies(int taskId) const {
    const Task* task = GetTaskById(taskId);
    if (task) {
        return task->dependencies;
    }
    return Vector<int>();
}

Time Project::GetEarliestStartDate() const {
    Time earliest = Null;
    for (const Task& task : tasks) {
        if (!task.startDate.IsNull()) {
            if (earliest.IsNull() || task.startDate < earliest) {
                earliest = task.startDate;
            }
        }
    }
    return earliest;
}

Time Project::GetLatestEndDate() const {
    Time latest = Null;
    for (const Task& task : tasks) {
        if (!task.endDate.IsNull()) {
            if (latest.IsNull() || task.endDate > latest) {
                latest = task.endDate;
            }
        }
    }
    return latest;
}

double Project::GetTotalWorkload() const {
    double total = 0.0;
    for (const Task& task : tasks) {
        total += task.workload;
    }
    return total;
}

double Project::GetCompletedWorkload() const {
    double completed = 0.0;
    for (const Task& task : tasks) {
        completed += task.workload * (task.progress / 100.0);
    }
    return completed;
}

double Project::GetRemainingWorkload() const {
    double remaining = 0.0;
    for (const Task& task : tasks) {
        remaining += task.GetRemainingWorkload();
    }
    return remaining;
}

double Project::GetOverallProgress() const {
    double totalWorkload = GetTotalWorkload();
    if (totalWorkload <= 0) return 0.0;
    
    double completedWorkload = GetCompletedWorkload();
    return (completedWorkload / totalWorkload) * 100.0;
}

bool Project::IsValid() const {
    return !name.IsEmpty();
}

bool Project::operator==(const Project& other) const {
    return id == other.id && 
           name == other.name && 
           description == other.description;
}

bool Project::operator!=(const Project& other) const {
    return !(*this == other);
}
