#include "CircularDependencyChecker.h"
#include "../Model/Project.h"
#include "../Model/Task.h"

using namespace Upp;

// CircularDependencyCheckResult implementation
CircularDependencyCheckResult::CircularDependencyCheckResult() 
    : hasCircularDependencies(false) {}

void CircularDependencyCheckResult::Clear() {
    hasCircularDependencies = false;
    cycles.Clear();
    messages.Clear();
}

bool CircularDependencyCheckResult::HasCycles() const {
    return hasCircularDependencies && !cycles.IsEmpty();
}

// CircularDependencyChecker implementation
CircularDependencyChecker::CircularDependencyChecker() {}

CircularDependencyChecker::~CircularDependencyChecker() {}

CircularDependencyCheckResult CircularDependencyChecker::CheckProject(const Project& project) {
    CircularDependencyCheckResult result;
    
    // Find all cycles
    result.cycles = FindAllCycles(project);
    result.hasCircularDependencies = !result.cycles.IsEmpty();
    
    // Generate messages
    if (result.hasCircularDependencies) {
        result.messages.Add(Format("Found %d circular dependency cycles", result.cycles.GetCount()));
        
        for (int i = 0; i < result.cycles.GetCount(); i++) {
            String cycleStr = "Cycle " + AsString(i + 1) + ": ";
            for (int taskId : result.cycles[i]) {
                const Task* task = project.GetTaskById(taskId);
                if (task) {
                    cycleStr += task->name + " -> ";
                }
            }
            // Remove last arrow
            if (cycleStr.EndsWith(" -> ")) {
                cycleStr = cycleStr.Left(cycleStr.GetLength() - 4);
            }
            result.messages.Add(cycleStr);
        }
    } else {
        result.messages.Add("No circular dependencies found");
    }
    
    return result;
}

bool CircularDependencyChecker::CheckTask(const Project& project, int taskId, Vector<int>& cycle) {
    Vector<bool> visited(project.tasks.GetCount() + 1, false);
    Vector<bool> recursionStack(project.tasks.GetCount() + 1, false);
    Vector<int> path;
    
    return HasCycleRecursive(project, taskId, visited, recursionStack, path, cycle);
}

Vector<Vector<int>> CircularDependencyChecker::FindAllCycles(const Project& project) {
    Vector<Vector<int>> allCycles;
    Vector<bool> visited(project.tasks.GetCount() + 1, false);
    
    // Check each task
    for (const Task& task : project.tasks) {
        if (!visited[task.id]) {
            Vector<int> path;
            Vector<int> cycle;
            
            if (HasCycleRecursive(project, task.id, visited, Vector<bool>(project.tasks.GetCount() + 1, false), 
                               path, cycle)) {
                allCycles.Add(cycle);
            }
        }
    }
    
    return allCycles;
}

Vector<int> CircularDependencyChecker::FindFirstCycle(const Project& project) {
    Vector<Vector<int>> allCycles = FindAllCycles(project);
    
    if (allCycles.IsEmpty()) {
        return Vector<int>();
    }
    
    return allCycles[0];
}

bool CircularDependencyChecker::WouldCreateCycle(const Project& project, int fromTaskId, int toTaskId) {
    // Create a temporary project with the new dependency
    Project tempProject = project;
    
    Task* fromTask = tempProject.GetTaskById(fromTaskId);
    Task* toTask = tempProject.GetTaskById(toTaskId);
    
    if (!fromTask || !toTask) {
        return false;
    }
    
    // Add the dependency temporarily
    fromTask->AddDependency(toTaskId);
    
    // Check for cycles
    bool hasCycle = HasCycle(tempProject, fromTaskId);
    
    // Remove the dependency (restore state)
    fromTask->RemoveDependency(toTaskId);
    
    return hasCycle;
}

bool CircularDependencyChecker::HasCycle(const Project& project) {
    for (const Task& task : project.tasks) {
        if (HasCycle(project, task.id)) {
            return true;
        }
    }
    return false;
}

bool CircularDependencyChecker::HasCycle(const Project& project, int startTaskId) {
    Vector<bool> visited(project.tasks.GetCount() + 1, false);
    Vector<bool> recursionStack(project.tasks.GetCount() + 1, false);
    Vector<int> path;
    Vector<int> cycle;
    
    return HasCycleRecursive(project, startTaskId, visited, recursionStack, path, cycle);
}

bool CircularDependencyChecker::HasCycleRecursive(const Project& project, int taskId, 
                                               Vector<bool>& visited, Vector<bool>& recursionStack,
                                               Vector<int>& path, Vector<int>& cycle) {
    // Mark current task as visited and add to recursion stack
    visited[taskId] = true;
    recursionStack[taskId] = true;
    path.Add(taskId);
    
    const Task* task = project.GetTaskById(taskId);
    if (!task) {
        return false;
    }
    
    // Check all dependencies
    for (int depId : task->dependencies) {
        if (!visited[depId]) {
            if (HasCycleRecursive(project, depId, visited, recursionStack, path, cycle)) {
                return true;
            }
        } else if (recursionStack[depId]) {
            // Found a cycle
            // Extract the cycle from the path
            int cycleStartIndex = FindPathIndex(depId, path);
            if (cycleStartIndex >= 0) {
                for (int i = cycleStartIndex; i < path.GetCount(); i++) {
                    cycle.Add(path[i]);
                }
                cycle.Add(depId); // Complete the cycle
            }
            return true;
        }
    }
    
    // Remove current task from recursion stack and path
    recursionStack[taskId] = false;
    path.Remove(path.GetCount() - 1);
    
    return false;
}

Vector<Vector<int>> CircularDependencyChecker::FindAllCyclesJohnson(const Project& project) {
    // Johnson's algorithm for finding all elementary circuits in a directed graph
    // This is a more efficient algorithm for finding all cycles
    
    Vector<Vector<int>> allCycles;
    
    // Implementation of Johnson's algorithm would go here
    // For simplicity, we'll use the recursive approach for now
    
    return FindAllCycles(project);
}

bool CircularDependencyChecker::IsInPath(int taskId, const Vector<int>& path) const {
    for (int id : path) {
        if (id == taskId) {
            return true;
        }
    }
    return false;
}

int CircularDependencyChecker::FindPathIndex(int taskId, const Vector<int>& path) const {
    for (int i = 0; i < path.GetCount(); i++) {
        if (path[i] == taskId) {
            return i;
        }
    }
    return -1;
}

// Singleton implementation
CircularDependencyChecker CircularDependencyCheckerSingleton::instance;

CircularDependencyChecker& CircularDependencyCheckerSingleton::GetInstance() {
    return instance;
}
