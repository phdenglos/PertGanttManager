#ifndef CIRCULAR_DEPENDENCY_CHECKER_H
#define CIRCULAR_DEPENDENCY_CHECKER_H

#include <Core/Core.h>
#include <vector>
#include <memory>

using namespace Upp;

class Project;
class Task;

// Circular dependency check result
struct CircularDependencyCheckResult {
    bool hasCircularDependencies;
    Vector<Vector<int>> cycles; // List of cycles found
    Vector<String> messages;
    
    CircularDependencyCheckResult();
    void Clear();
    bool HasCycles() const;
};

// Circular dependency checker
class CircularDependencyChecker {
public:
    CircularDependencyChecker();
    ~CircularDependencyChecker();
    
    // Check for circular dependencies in a project
    CircularDependencyCheckResult CheckProject(const Project& project);
    
    // Check for circular dependencies starting from a specific task
    bool CheckTask(const Project& project, int taskId, Vector<int>& cycle);
    
    // Find all cycles in the project
    Vector<Vector<int>> FindAllCycles(const Project& project);
    
    // Get the first cycle found (if any)
    Vector<int> FindFirstCycle(const Project& project);
    
    // Check if a specific dependency would create a cycle
    bool WouldCreateCycle(const Project& project, int fromTaskId, int toTaskId);
    
    // Utility methods
    static bool HasCycle(const Project& project);
    static bool HasCycle(const Project& project, int startTaskId);
    
private:
    // Recursive cycle detection
    bool HasCycleRecursive(const Project& project, int taskId, Vector<bool>& visited,
                          Vector<bool>& recursionStack, Vector<int>& path, Vector<int>& cycle);
    
    // Find all cycles using Johnson's algorithm
    Vector<Vector<int>> FindAllCyclesJohnson(const Project& project);
    
    // Helper methods
    bool IsInPath(int taskId, const Vector<int>& path) const;
    int FindPathIndex(int taskId, const Vector<int>& path) const;
};

// Singleton circular dependency checker
class CircularDependencyCheckerSingleton {
public:
    static CircularDependencyChecker& GetInstance();
    
private:
    static CircularDependencyChecker instance;
};

#endif // CIRCULAR_DEPENDENCY_CHECKER_H
