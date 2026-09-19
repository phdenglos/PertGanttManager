#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Core/Core.h>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>

using namespace Upp;

class Project;
class Task;
class Resource;

// Scheduling result structure
struct SchedulingResult {
    bool success;
    String message;
    Vector<Task> scheduledTasks;
    Vector<std::pair<int, double>> resourceUtilization; // resourceId, max utilization rate
    double totalDuration;
    Time projectStartDate;
    Time projectEndDate;
    
    SchedulingResult();
    void Clear();
};

// Scheduler class for project planning
class Scheduler {
public:
    Scheduler();
    ~Scheduler();
    
    // Main scheduling method
    SchedulingResult ScheduleProject(Project& project, bool useThreads = true);
    
    // Scheduling algorithms
    enum SchedulingAlgorithm {
        ASAP, // As Soon As Possible
        ALAP, // As Late As Possible
        PRIORITY_BASED,
        RESOURCE_LEVELING
    };
    
    // Set scheduling algorithm
    void SetAlgorithm(SchedulingAlgorithm algorithm);
    SchedulingAlgorithm GetAlgorithm() const;
    
    // Set scheduling parameters
    void SetUseResourceLeveling(bool use);
    bool GetUseResourceLeveling() const;
    
    void SetMaxResourceUtilization(double maxUtilization);
    double GetMaxResourceUtilization() const;
    
    // Progress reporting
    typedef std::function<void(int, const String&)> ProgressCallback;
    void SetProgressCallback(ProgressCallback callback);
    
    // Cancellation
    void Cancel();
    bool IsCancelled() const;
    
    // Thread safety
    void SetUseThreads(bool use);
    bool GetUseThreads() const;
    
private:
    // Scheduling algorithm
    SchedulingAlgorithm algorithm;
    
    // Parameters
    bool useResourceLeveling;
    double maxResourceUtilization;
    bool useThreads;
    
    // Cancellation
    std::atomic<bool> cancelled;
    
    // Progress callback
    ProgressCallback progressCallback;
    std::mutex callbackMutex;
    
    // Thread management
    std::unique_ptr<std::thread> workerThread;
    std::mutex threadMutex;
    
    // Helper methods
    SchedulingResult ScheduleASAP(Project& project);
    SchedulingResult ScheduleALAP(Project& project);
    SchedulingResult SchedulePriorityBased(Project& project);
    SchedulingResult ScheduleWithResourceLeveling(Project& project);
    
    // Core scheduling logic
    void CalculateTaskDates(Project& project, Vector<Time>& earliestStart, Vector<Time>& earliestEnd,
                          Vector<Time>& latestStart, Vector<Time>& latestEnd);
    void CalculateResourceUtilization(const Project& project, const Vector<Time>& taskStartDates,
                                     const Vector<Time>& taskEndDates);
    void LevelResources(Project& project, Vector<Time>& startDates, Vector<Time>& endDates);
    
    // Topological sorting
    Vector<int> TopologicalSort(const Project& project);
    bool HasCircularDependencies(const Project& project);
    
    // Critical path calculation
    Vector<int> FindCriticalPath(const Project& project);
    double CalculateCriticalPathDuration(const Project& project);
    
    // Utility methods
    Time CalculateEarliestStartDate(const Project& project, int taskId, 
                                    const Vector<Time>& earliestEndDates);
    Time CalculateEarliestEndDate(const Project& project, int taskId, const Time& startDate);
    
    Time CalculateLatestEndDate(const Project& project, int taskId, const Vector<Time>& latestStartDates);
    Time CalculateLatestStartDate(const Project& project, int taskId, const Time& endDate);
    
    // Resource availability checking
    bool IsResourceAvailable(const Project& project, int resourceId, const Time& startDate, 
                             const Time& endDate, double requiredRate);
    double GetResourceUtilizationAtTime(const Project& project, int resourceId, const Time& date);
    
    // Thread worker
    void WorkerThreadFunction(Project& project, SchedulingResult& result);
    
    // Disable copy and assignment
    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;
};

// Singleton scheduler
class SchedulerSingleton {
public:
    static Scheduler& GetInstance();
    
private:
    static Scheduler instance;
};

#endif // SCHEDULER_H
