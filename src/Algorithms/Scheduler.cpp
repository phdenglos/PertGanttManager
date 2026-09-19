#include "Scheduler.h"
#include "../Model/Project.h"
#include "../Model/Task.h"
#include "../Model/Resource.h"

using namespace Upp;

// SchedulingResult implementation
SchedulingResult::SchedulingResult() : success(false), totalDuration(0.0) {
    projectStartDate = Null;
    projectEndDate = Null;
}

void SchedulingResult::Clear() {
    success = false;
    message.Clear();
    scheduledTasks.Clear();
    resourceUtilization.Clear();
    totalDuration = 0.0;
    projectStartDate = Null;
    projectEndDate = Null;
}

// Scheduler implementation
Scheduler::Scheduler() 
    : algorithm(PRIORITY_BASED), useResourceLeveling(true), maxResourceUtilization(100.0),
      useThreads(true), cancelled(false) {
}

Scheduler::~Scheduler() {
    Cancel();
}

SchedulingResult Scheduler::ScheduleProject(Project& project, bool useThreads) {
    SchedulingResult result;
    
    // Reset cancellation flag
    cancelled = false;
    
    // Check for circular dependencies first
    if (HasCircularDependencies(project)) {
        result.success = false;
        result.message = "Cannot schedule project: circular dependencies detected!";
        return result;
    }
    
    // If we have no tasks, return empty result
    if (project.tasks.IsEmpty()) {
        result.success = true;
        result.message = "No tasks to schedule";
        result.projectStartDate = Time::GetCurrent();
        result.projectEndDate = Time::GetCurrent();
        return result;
    }
    
    // Set thread mode
    this->useThreads = useThreads;
    
    if (useThreads) {
        // Run in worker thread
        std::promise<SchedulingResult> resultPromise;
        std::future<SchedulingResult> resultFuture = resultPromise.get_future();
        
        // Create and start thread
        workerThread = std::make_unique<std::thread>([this, &project, &resultPromise]() {
            try {
                SchedulingResult threadResult = ScheduleWithResourceLeveling(project);
                resultPromise.set_value(threadResult);
            } catch (const std::exception& e) {
                SchedulingResult errorResult;
                errorResult.success = false;
                errorResult.message = e.what();
                resultPromise.set_value(errorResult);
            }
        });
        
        // Wait for result
        result = resultFuture.get();
        
        // Clean up thread
        workerThread->join();
        workerThread.reset();
    } else {
        // Run synchronously
        result = ScheduleWithResourceLeveling(project);
    }
    
    return result;
}

void Scheduler::SetAlgorithm(SchedulingAlgorithm algorithm) {
    this->algorithm = algorithm;
}

Scheduler::SchedulingAlgorithm Scheduler::GetAlgorithm() const {
    return algorithm;
}

void Scheduler::SetUseResourceLeveling(bool use) {
    useResourceLeveling = use;
}

bool Scheduler::GetUseResourceLeveling() const {
    return useResourceLeveling;
}

void Scheduler::SetMaxResourceUtilization(double maxUtilization) {
    this->maxResourceUtilization = maxUtilization;
    if (this->maxResourceUtilization < 0) this->maxResourceUtilization = 0;
    if (this->maxResourceUtilization > 100) this->maxResourceUtilization = 100;
}

double Scheduler::GetMaxResourceUtilization() const {
    return maxResourceUtilization;
}

void Scheduler::SetProgressCallback(ProgressCallback callback) {
    std::lock_guard<std::mutex> lock(callbackMutex);
    progressCallback = callback;
}

void Scheduler::Cancel() {
    cancelled = true;
    
    // Wait for thread to finish if it's running
    if (workerThread && workerThread->joinable()) {
        workerThread->join();
        workerThread.reset();
    }
}

bool Scheduler::IsCancelled() const {
    return cancelled;
}

void Scheduler::SetUseThreads(bool use) {
    useThreads = use;
}

bool Scheduler::GetUseThreads() const {
    return useThreads;
}

SchedulingResult Scheduler::ScheduleASAP(Project& project) {
    SchedulingResult result;
    
    if (IsCancelled()) {
        result.success = false;
        result.message = "Scheduling cancelled";
        return result;
    }
    
    // Get topological order
    Vector<int> order = TopologicalSort(project);
    
    // Calculate earliest start and end dates
    Vector<Time> earliestStart(project.tasks.GetCount() + 1, Null);
    Vector<Time> earliestEnd(project.tasks.GetCount() + 1, Null);
    
    // Find project start date (minimum of all task start dates or current date)
    Time projectStart = Time::GetCurrent();
    bool hasStartDate = false;
    
    for (const Task& task : project.tasks) {
        if (!task.startDate.IsNull()) {
            if (!hasStartDate || task.startDate < projectStart) {
                projectStart = task.startDate;
            }
            hasStartDate = true;
        }
    }
    
    // Process tasks in topological order
    for (int taskId : order) {
        const Task* task = project.GetTaskById(taskId);
        if (!task) continue;
        
        // Calculate earliest start date
        Time startDate = CalculateEarliestStartDate(project, taskId, earliestEnd);
        
        if (startDate.IsNull()) {
            // No dependencies, can start at project start
            startDate = projectStart;
        }
        
        earliestStart[taskId] = startDate;
        earliestEnd[taskId] = CalculateEarliestEndDate(project, taskId, startDate);
        
        if (IsCancelled()) {
            result.success = false;
            result.message = "Scheduling cancelled";
            return result;
        }
    }
    
    // Update tasks with calculated dates
    for (Task& task : project.tasks) {
        if (earliestStart[task.id].IsNull()) {
            task.startDate = projectStart;
        } else {
            task.startDate = earliestStart[task.id];
        }
        task.endDate = earliestEnd[task.id];
        
        // Update positions for diagram
        task.xPosition = (task.startDate - projectStart).GetDays() * 20;
        task.yPosition = 0; // Will be set by diagram layout
    }
    
    // Calculate resource utilization
    CalculateResourceUtilization(project, earliestStart, earliestEnd);
    
    result.success = true;
    result.message = "Scheduling completed successfully";
    result.scheduledTasks = project.tasks;
    result.projectStartDate = projectStart;
    result.projectEndDate = earliestEnd[order.Last()];
    result.totalDuration = (result.projectEndDate - result.projectStartDate).GetDays();
    
    return result;
}

SchedulingResult Scheduler::ScheduleALAP(Project& project) {
    SchedulingResult result;
    
    if (IsCancelled()) {
        result.success = false;
        result.message = "Scheduling cancelled";
        return result;
    }
    
    // First, calculate ASAP to get project end date
    SchedulingResult asapResult = ScheduleASAP(project);
    if (!asapResult.success) {
        return asapResult;
    }
    
    Time projectEnd = asapResult.projectEndDate;
    
    // Get reverse topological order (from end to start)
    Vector<int> order = TopologicalSort(project);
    std::reverse(order.begin(), order.end());
    
    // Calculate latest start and end dates
    Vector<Time> latestStart(project.tasks.GetCount() + 1, projectEnd);
    Vector<Time> latestEnd(project.tasks.GetCount() + 1, projectEnd);
    
    // Process tasks in reverse topological order
    for (int taskId : order) {
        const Task* task = project.GetTaskById(taskId);
        if (!task) continue;
        
        // Calculate latest end date
        Time endDate = CalculateLatestEndDate(project, taskId, latestStart);
        
        if (endDate.IsNull()) {
            // No dependent tasks, can end at project end
            endDate = projectEnd;
        }
        
        latestEnd[taskId] = endDate;
        latestStart[taskId] = CalculateLatestStartDate(project, taskId, endDate);
        
        if (IsCancelled()) {
            result.success = false;
            result.message = "Scheduling cancelled";
            return result;
        }
    }
    
    // Update tasks with calculated dates
    for (Task& task : project.tasks) {
        task.startDate = latestStart[task.id];
        task.endDate = latestEnd[task.id];
        
        // Update positions for diagram
        task.xPosition = (task.startDate - asapResult.projectStartDate).GetDays() * 20;
        task.yPosition = 0;
    }
    
    result.success = true;
    result.message = "ALAP scheduling completed successfully";
    result.scheduledTasks = project.tasks;
    result.projectStartDate = latestStart[order.Last()];
    result.projectEndDate = projectEnd;
    result.totalDuration = (result.projectEndDate - result.projectStartDate).GetDays();
    
    return result;
}

SchedulingResult Scheduler::SchedulePriorityBased(Project& project) {
    SchedulingResult result;
    
    if (IsCancelled()) {
        result.success = false;
        result.message = "Scheduling cancelled";
        return result;
    }
    
    // Sort tasks by priority (1 is highest)
    Vector<Task*> sortedTasks;
    for (Task& task : project.tasks) {
        sortedTasks.Add(&task);
    }
    
    std::sort(sortedTasks.begin(), sortedTasks.end(), 
        [](Task* a, Task* b) {
            return a->priority < b->priority; // Lower number = higher priority
        });
    
    // Calculate dates
    Vector<Time> startDates(project.tasks.GetCount() + 1, Null);
    Vector<Time> endDates(project.tasks.GetCount() + 1, Null);
    
    Time projectStart = Time::GetCurrent();
    
    // Process tasks by priority
    for (Task* task : sortedTasks) {
        // Calculate earliest start date considering dependencies
        Time startDate = CalculateEarliestStartDate(project, task->id, endDates);
        
        if (startDate.IsNull()) {
            startDate = projectStart;
        }
        
        startDates[task->id] = startDate;
        endDates[task->id] = CalculateEarliestEndDate(project, task->id, startDate);
        
        if (IsCancelled()) {
            result.success = false;
            result.message = "Scheduling cancelled";
            return result;
        }
    }
    
    // Update tasks with calculated dates
    for (Task& task : project.tasks) {
        task.startDate = startDates[task.id];
        task.endDate = endDates[task.id];
        
        // Update positions for diagram
        task.xPosition = (task.startDate - projectStart).GetDays() * 20;
        task.yPosition = 0;
    }
    
    // Calculate resource utilization
    CalculateResourceUtilization(project, startDates, endDates);
    
    result.success = true;
    result.message = "Priority-based scheduling completed successfully";
    result.scheduledTasks = project.tasks;
    result.projectStartDate = projectStart;
    result.projectEndDate = endDates[sortedTasks.Last()->id];
    result.totalDuration = (result.projectEndDate - result.projectStartDate).GetDays();
    
    return result;
}

SchedulingResult Scheduler::ScheduleWithResourceLeveling(Project& project) {
    SchedulingResult result;
    
    if (IsCancelled()) {
        result.success = false;
        result.message = "Scheduling cancelled";
        return result;
    }
    
    // First, run the selected algorithm
    SchedulingResult initialResult;
    
    switch (algorithm) {
        case ASAP:
            initialResult = ScheduleASAP(project);
            break;
        case ALAP:
            initialResult = ScheduleALAP(project);
            break;
        case PRIORITY_BASED:
            initialResult = SchedulePriorityBased(project);
            break;
        default:
            initialResult = SchedulePriorityBased(project);
            break;
    }
    
    if (!initialResult.success) {
        return initialResult;
    }
    
    if (IsCancelled()) {
        result.success = false;
        result.message = "Scheduling cancelled";
        return result;
    }
    
    // If resource leveling is enabled, adjust dates to avoid over-allocation
    if (useResourceLeveling) {
        Vector<Time> startDates(project.tasks.GetCount() + 1);
        Vector<Time> endDates(project.tasks.GetCount() + 1);
        
        for (const Task& task : project.tasks) {
            startDates[task.id] = task.startDate;
            endDates[task.id] = task.endDate;
        }
        
        LevelResources(project, startDates, endDates);
        
        // Update tasks with leveled dates
        for (Task& task : project.tasks) {
            task.startDate = startDates[task.id];
            task.endDate = endDates[task.id];
        }
    }
    
    // Calculate resource utilization
    Vector<Time> finalStartDates(project.tasks.GetCount() + 1);
    Vector<Time> finalEndDates(project.tasks.GetCount() + 1);
    
    for (const Task& task : project.tasks) {
        finalStartDates[task.id] = task.startDate;
        finalEndDates[task.id] = task.endDate;
    }
    
    CalculateResourceUtilization(project, finalStartDates, finalEndDates);
    
    result = initialResult;
    result.scheduledTasks = project.tasks;
    result.message = "Scheduling with resource leveling completed successfully";
    
    return result;
}

void Scheduler::CalculateTaskDates(Project& project, Vector<Time>& earliestStart, Vector<Time>& earliestEnd,
                               Vector<Time>& latestStart, Vector<Time>& latestEnd) {
    // Resize vectors
    earliestStart.SetCount(project.tasks.GetCount() + 1, Null);
    earliestEnd.SetCount(project.tasks.GetCount() + 1, Null);
    latestStart.SetCount(project.tasks.GetCount() + 1, Null);
    latestEnd.SetCount(project.tasks.GetCount() + 1, Null);
    
    // Calculate earliest dates (forward pass)
    Time projectStart = Time::GetCurrent();
    
    for (const Task& task : project.tasks) {
        earliestStart[task.id] = CalculateEarliestStartDate(project, task.id, earliestEnd);
        if (earliestStart[task.id].IsNull()) {
            earliestStart[task.id] = projectStart;
        }
        earliestEnd[task.id] = CalculateEarliestEndDate(project, task.id, earliestStart[task.id]);
    }
    
    // Calculate latest dates (backward pass)
    Time projectEnd = earliestEnd[project.tasks.Last().id];
    
    for (int i = project.tasks.GetCount() - 1; i >= 0; i--) {
        const Task& task = project.tasks[i];
        latestEnd[task.id] = CalculateLatestEndDate(project, task.id, latestStart);
        if (latestEnd[task.id].IsNull()) {
            latestEnd[task.id] = projectEnd;
        }
        latestStart[task.id] = CalculateLatestStartDate(project, task.id, latestEnd[task.id]);
    }
}

void Scheduler::CalculateResourceUtilization(const Project& project, const Vector<Time>& taskStartDates,
                                          const Vector<Time>& taskEndDates) {
    // This method calculates the maximum utilization rate for each resource
    // over the project timeline
    
    // For each resource, find the maximum utilization rate
    for (const Resource& resource : project.resources) {
        double maxUtilization = 0.0;
        
        // Check all tasks that use this resource
        for (const Task& task : project.tasks) {
            for (const auto& assignment : task.resourceAssignments) {
                if (assignment.first == resource.id) {
                    // Calculate utilization contribution
                    double utilization = assignment.second / resource.maxUtilizationRate * 100.0;
                    if (utilization > maxUtilization) {
                        maxUtilization = utilization;
                    }
                }
            }
        }
        
        // Store result
        // In a real implementation, you would track utilization over time
        // For now, we just store the maximum allocation rate
    }
}

void Scheduler::LevelResources(Project& project, Vector<Time>& startDates, Vector<Time>& endDates) {
    // Resource leveling algorithm
    // This is a simplified version that delays tasks to avoid resource conflicts
    
    bool changed = true;
    int iterations = 0;
    const int MAX_ITERATIONS = 100;
    
    while (changed && iterations < MAX_ITERATIONS) {
        changed = false;
        iterations++;
        
        // For each task, check if it conflicts with other tasks on the same resource
        for (Task& task : project.tasks) {
            for (const auto& assignment : task.resourceAssignments) {
                int resourceId = assignment.first;
                double allocationRate = assignment.second;
                
                // Find other tasks using the same resource
                for (Task& otherTask : project.tasks) {
                    if (&task == &otherTask) continue;
                    
                    for (const auto& otherAssignment : otherTask.resourceAssignments) {
                        if (otherAssignment.first == resourceId) {
                            // Check if tasks overlap
                            if (startDates[task.id] < endDates[otherTask.id] &&
                                startDates[otherTask.id] < endDates[task.id]) {
                                
                                // Calculate total allocation rate
                                double totalAllocation = allocationRate + otherAssignment.second;
                                
                                // If total exceeds max utilization, delay the task with lower priority
                                if (totalAllocation > maxResourceUtilization) {
                                    // Delay the lower priority task
                                    if (task.HasHigherPriorityThan(otherTask)) {
                                        // Delay otherTask
                                        Time delay = (endDates[task.id] - startDates[otherTask.id]).GetDays() + 1;
                                        startDates[otherTask.id] = endDates[task.id];
                                        endDates[otherTask.id] = startDates[otherTask.id] + 
                                                                 (endDates[otherTask.id] - otherTask.startDate).GetDays() * 24 * 3600;
                                        changed = true;
                                    } else {
                                        // Delay this task
                                        Time delay = (endDates[otherTask.id] - startDates[task.id]).GetDays() + 1;
                                        startDates[task.id] = endDates[otherTask.id];
                                        endDates[task.id] = startDates[task.id] + 
                                                           (endDates[task.id] - task.startDate).GetDays() * 24 * 3600;
                                        changed = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            if (IsCancelled()) {
                return;
            }
        }
    }
}

Vector<int> Scheduler::TopologicalSort(const Project& project) {
    Vector<int> sortedTasks;
    Vector<bool> visited(project.tasks.GetCount() + 1, false);
    Vector<bool> tempMarked(project.tasks.GetCount() + 1, false);
    
    // Use Kahn's algorithm for topological sorting
    Vector<int> inDegree(project.tasks.GetCount() + 1, 0);
    Vector<Vector<int>> adjacencyList(project.tasks.GetCount() + 1);
    
    // Build adjacency list and calculate in-degrees
    for (const Task& task : project.tasks) {
        for (int depId : task.dependencies) {
            adjacencyList[depId].Add(task.id);
            inDegree[task.id]++;
        }
    }
    
    // Find all tasks with in-degree 0
    std::queue<int> queue;
    for (const Task& task : project.tasks) {
        if (inDegree[task.id] == 0) {
            queue.push(task.id);
        }
    }
    
    // Process queue
    while (!queue.empty()) {
        int taskId = queue.front();
        queue.pop();
        sortedTasks.Add(taskId);
        
        // Decrease in-degree of adjacent tasks
        for (int adjTaskId : adjacencyList[taskId]) {
            inDegree[adjTaskId]--;
            if (inDegree[adjTaskId] == 0) {
                queue.push(adjTaskId);
            }
        }
    }
    
    // Check for circular dependencies
    if (sortedTasks.GetCount() != project.tasks.GetCount()) {
        // There's a cycle, return empty list
        sortedTasks.Clear();
    }
    
    return sortedTasks;
}

bool Scheduler::HasCircularDependencies(const Project& project) {
    Vector<int> sorted = TopologicalSort(project);
    return sorted.GetCount() != project.tasks.GetCount();
}

Vector<int> Scheduler::FindCriticalPath(const Project& project) {
    Vector<int> criticalPath;
    
    if (project.tasks.IsEmpty()) {
        return criticalPath;
    }
    
    // Calculate earliest and latest dates
    Vector<Time> earliestStart(project.tasks.GetCount() + 1, Null);
    Vector<Time> earliestEnd(project.tasks.GetCount() + 1, Null);
    Vector<Time> latestStart(project.tasks.GetCount() + 1, Null);
    Vector<Time> latestEnd(project.tasks.GetCount() + 1, Null);
    
    CalculateTaskDates(project, earliestStart, earliestEnd, latestStart, latestEnd);
    
    // Find tasks with zero float (critical path tasks)
    for (const Task& task : project.tasks) {
        double totalFloat = (latestStart[task.id] - earliestStart[task.id]).GetDays();
        if (totalFloat == 0) {
            criticalPath.Add(task.id);
        }
    }
    
    return criticalPath;
}

double Scheduler::CalculateCriticalPathDuration(const Project& project) {
    Vector<int> criticalPath = FindCriticalPath(project);
    
    if (criticalPath.IsEmpty()) {
        return 0.0;
    }
    
    // Calculate total duration of critical path
    double totalDuration = 0.0;
    
    for (int taskId : criticalPath) {
        const Task* task = project.GetTaskById(taskId);
        if (task) {
            totalDuration += task->duration;
        }
    }
    
    return totalDuration;
}

Time Scheduler::CalculateEarliestStartDate(const Project& project, int taskId, 
                                         const Vector<Time>& earliestEndDates) {
    const Task* task = project.GetTaskById(taskId);
    if (!task) return Null;
    
    Time earliestStart = Null;
    
    // If task has dependencies, earliest start is the maximum of all dependency end dates
    for (int depId : task->dependencies) {
        if (!earliestEndDates[depId].IsNull()) {
            if (earliestStart.IsNull() || earliestEndDates[depId] > earliestStart) {
                earliestStart = earliestEndDates[depId];
            }
        }
    }
    
    return earliestStart;
}

Time Scheduler::CalculateEarliestEndDate(const Project& project, int taskId, const Time& startDate) {
    const Task* task = project.GetTaskById(taskId);
    if (!task || startDate.IsNull()) return Null;
    
    return startDate + task->duration * 24 * 3600;
}

Time Scheduler::CalculateLatestEndDate(const Project& project, int taskId, 
                                     const Vector<Time>& latestStartDates) {
    const Task* task = project.GetTaskById(taskId);
    if (!task) return Null;
    
    Time latestEnd = Null;
    
    // Find all tasks that depend on this task
    Vector<int> dependentTasks = project.GetDependentTasks(taskId);
    
    // Latest end is the minimum of all dependent task start dates
    for (int depTaskId : dependentTasks) {
        if (!latestStartDates[depTaskId].IsNull()) {
            if (latestEnd.IsNull() || latestStartDates[depTaskId] < latestEnd) {
                latestEnd = latestStartDates[depTaskId];
            }
        }
    }
    
    return latestEnd;
}

Time Scheduler::CalculateLatestStartDate(const Project& project, int taskId, const Time& endDate) {
    const Task* task = project.GetTaskById(taskId);
    if (!task || endDate.IsNull()) return Null;
    
    return endDate - task->duration * 24 * 3600;
}

bool Scheduler::IsResourceAvailable(const Project& project, int resourceId, const Time& startDate, 
                                const Time& endDate, double requiredRate) {
    if (startDate.IsNull() || endDate.IsNull()) return false;
    
    const Resource* resource = project.GetResourceById(resourceId);
    if (!resource) return false;
    
    // Check if the resource is available at the required rate for the entire duration
    // This is a simplified check - in a real implementation, you would check
    // the utilization at multiple points in time
    
    double currentUtilization = GetResourceUtilizationAtTime(project, resourceId, startDate);
    
    return (currentUtilization + requiredRate) <= resource->maxUtilizationRate;
}

double Scheduler::GetResourceUtilizationAtTime(const Project& project, int resourceId, const Time& date) {
    double utilization = 0.0;
    
    for (const Task& task : project.tasks) {
        for (const auto& assignment : task.resourceAssignments) {
            if (assignment.first == resourceId) {
                // Check if task is active at the given date
                if (!task.startDate.IsNull() && !task.endDate.IsNull() &&
                    date >= task.startDate && date <= task.endDate) {
                    utilization += assignment.second;
                }
            }
        }
    }
    
    return utilization;
}

void Scheduler::WorkerThreadFunction(Project& project, SchedulingResult& result) {
    result = ScheduleWithResourceLeveling(project);
}

// Singleton implementation
Scheduler SchedulerSingleton::instance;

Scheduler& SchedulerSingleton::GetInstance() {
    return instance;
}
