#include "TaskForm.h"
#include "../../Model/Project.h"
#include "../../Model/Task.h"

using namespace Upp;

TaskForm::TaskForm() : updatingControls(false) {
    CtrlLayout(*this, "Task Properties");
    
    // Connect controls
    okButton <<= THIS(OnOK);
    cancelButton <<= THIS(OnCancel);
    
    priorityEdit <<= THIS(OnPriorityChange);
    progressEdit <<= THIS(OnProgressChange);
    progressSlider <<= THIS(OnProgressSliderChange);
    colorEdit <<= THIS(OnColorChange);
    
    addDependencyButton <<= THIS(OnAddDependency);
    removeDependencyButton <<= THIS(OnRemoveDependency);
    addResourceButton <<= THIS(OnAddResource);
    removeResourceButton <<= THIS(OnRemoveResource);
    
    // Initialize priority list
    priorityEdit.Add("1 (Highest)");
    priorityEdit.Add("2");
    priorityEdit.Add("3");
    priorityEdit.Add("4");
    priorityEdit.Add("5 (Lowest)");
    
    // Set default values
    nameEdit.SetFocus();
    
    // Sync progress edit and slider
    progressEdit <<= 0.0;
    progressSlider <<= 0.0;
}

TaskForm::~TaskForm() {}

void TaskForm::SetTask(const Task& task, const Project& project) {
    currentTask = task;
    currentProject = project;
    
    updatingControls = true;
    
    // Set form values
    nameEdit <<= currentTask.name;
    priorityEdit <<= currentTask.priority - 1; // 0-based index
    workloadEdit <<= currentTask.workload;
    durationEdit <<= currentTask.duration;
    startDateEdit <<= currentTask.startDate;
    endDateEdit <<= currentTask.endDate;
    progressEdit <<= currentTask.progress;
    progressSlider <<= currentTask.progress;
    colorEdit <<= currentTask.color;
    xPositionEdit <<= currentTask.xPosition;
    yPositionEdit <<= currentTask.yPosition;
    
    // Update dependency and resource lists
    UpdateDependenciesList();
    UpdateResourcesList();
    
    updatingControls = false;
}

void TaskForm::GetTask(Task& task) const {
    task = currentTask;
    
    // Update from form values
    task.name = ~nameEdit;
    task.priority = ~priorityEdit + 1; // Convert to 1-based
    task.workload = ~workloadEdit;
    task.duration = ~durationEdit;
    task.startDate = ~startDateEdit;
    task.endDate = ~endDateEdit;
    task.progress = ~progressEdit;
    task.color = ~colorEdit;
    task.xPosition = ~xPositionEdit;
    task.yPosition = ~yPositionEdit;
    
    // Update dependencies from the array
    task.dependencies.Clear();
    for (int i = 0; i < dependenciesArray.GetCount(); i++) {
        if (dependenciesArray.IsCursor(i)) {
            String taskName = dependenciesArray.Get(i);
            const Task* depTask = currentProject.GetTaskByName(taskName);
            if (depTask) {
                task.dependencies.Add(depTask->id);
            }
        }
    }
    
    // Update resource assignments from the arrays
    task.resourceAssignments.Clear();
    for (int i = 0; i < resourceArray.GetCount(); i++) {
        if (resourceArray.IsCursor(i)) {
            String resourceName = resourceArray.Get(i);
            const Resource* resource = currentProject.GetResourceByName(resourceName);
            if (resource) {
                double allocationRate = allocationArray.Get(i);
                task.resourceAssignments.Add(std::make_pair(resource->id, allocationRate));
            }
        }
    }
}

void TaskForm::SetProject(const Project& project) {
    currentProject = project;
    
    // Update dependency and resource lists
    UpdateDependenciesList();
    UpdateResourcesList();
}

void TaskForm::OnOK() {
    // Validate input
    if (~nameEdit.IsEmpty()) {
        GetTask(currentTask);
        
        // Additional validation
        if (currentTask.workload < 0) {
            Exclamation("Workload cannot be negative!");
            workloadEdit.SetFocus();
            return;
        }
        
        if (currentTask.duration < 1) {
            Exclamation("Duration must be at least 1 day!");
            durationEdit.SetFocus();
            return;
        }
        
        if (currentTask.progress < 0 || currentTask.progress > 100) {
            Exclamation("Progress must be between 0 and 100%!");
            progressEdit.SetFocus();
            return;
        }
        
        WhenSave();
        Close();
    } else {
        Exclamation("Task name cannot be empty!");
        nameEdit.SetFocus();
    }
}

void TaskForm::OnCancel() {
    WhenCancel();
    Close();
}

void TaskForm::OnPriorityChange() {
    if (updatingControls) return;
    
    currentTask.priority = ~priorityEdit + 1; // Convert to 1-based
}

void TaskForm::OnProgressChange() {
    if (updatingControls) return;
    
    double progress = ~progressEdit;
    if (progress < 0) progress = 0;
    if (progress > 100) progress = 100;
    
    progressEdit <<= progress;
    progressSlider <<= progress;
    currentTask.progress = progress;
}

void TaskForm::OnProgressSliderChange() {
    if (updatingControls) return;
    
    double progress = ~progressSlider;
    progressEdit <<= progress;
    currentTask.progress = progress;
}

void TaskForm::OnColorChange() {
    if (updatingControls) return;
    
    currentTask.color = ~colorEdit;
}

void TaskForm::OnAddDependency() {
    // Show dialog to select dependency
    Vector<String> availableTasks = GetAvailableTasks();
    
    if (availableTasks.IsEmpty()) {
        Exclamation("No other tasks available for dependency!");
        return;
    }
    
    String selectedTask = SelectList(availableTasks, "Select Dependency");
    if (!selectedTask.IsEmpty()) {
        // Check if already in dependencies
        for (int i = 0; i < dependenciesArray.GetCount(); i++) {
            if (dependenciesArray.Get(i) == selectedTask) {
                Exclamation("This dependency is already added!");
                return;
            }
        }
        
        // Add to dependencies
        dependenciesArray.Add(selectedTask);
        UpdateDependenciesList();
        
        // Add to current task dependencies
        const Task* task = currentProject.GetTaskByName(selectedTask);
        if (task) {
            currentTask.AddDependency(task->id);
        }
    }
}

void TaskForm::OnRemoveDependency() {
    int index = dependenciesArray.GetCursor();
    if (index >= 0) {
        String taskName = dependenciesArray.Get(index);
        
        // Remove from dependencies array
        dependenciesArray.Remove(index);
        
        // Remove from current task dependencies
        const Task* task = currentProject.GetTaskByName(taskName);
        if (task) {
            currentTask.RemoveDependency(task->id);
        }
    }
}

void TaskForm::OnAddResource() {
    Vector<String> availableResources = GetAvailableResources();
    
    if (availableResources.IsEmpty()) {
        Exclamation("No resources available!");
        return;
    }
    
    String selectedResource = SelectList(availableResources, "Select Resource");
    if (!selectedResource.IsEmpty()) {
        // Check if already in resources
        for (int i = 0; i < resourceArray.GetCount(); i++) {
            if (resourceArray.Get(i) == selectedResource) {
                Exclamation("This resource is already assigned!");
                return;
            }
        }
        
        // Add to resources
        resourceArray.Add(selectedResource);
        allocationArray.Add(100.0); // Default allocation rate
        
        // Add to current task resource assignments
        const Resource* resource = currentProject.GetResourceByName(selectedResource);
        if (resource) {
            currentTask.AddResourceAssignment(resource->id, 100.0);
        }
    }
}

void TaskForm::OnRemoveResource() {
    int index = resourceArray.GetCursor();
    if (index >= 0) {
        String resourceName = resourceArray.Get(index);
        
        // Remove from arrays
        resourceArray.Remove(index);
        allocationArray.Remove(index);
        
        // Remove from current task resource assignments
        const Resource* resource = currentProject.GetResourceByName(resourceName);
        if (resource) {
            currentTask.RemoveResourceAssignment(resource->id);
        }
    }
}

void TaskForm::UpdateDependenciesList() {
    dependenciesArray.Clear();
    
    for (int depId : currentTask.dependencies) {
        const Task* task = currentProject.GetTaskById(depId);
        if (task) {
            dependenciesArray.Add(task->name);
        }
    }
    
    dependenciesArray.Refresh();
}

void TaskForm::UpdateResourcesList() {
    resourceArray.Clear();
    allocationArray.Clear();
    
    for (const auto& assignment : currentTask.resourceAssignments) {
        const Resource* resource = currentProject.GetResourceById(assignment.first);
        if (resource) {
            resourceArray.Add(resource->name);
            allocationArray.Add(assignment.second);
        }
    }
    
    resourceArray.Refresh();
    allocationArray.Refresh();
}

void TaskForm::UpdateEndDate() {
    Time startDate = ~startDateEdit;
    int duration = ~durationEdit;
    
    if (!startDate.IsNull() && duration > 0) {
        Time endDate = Task::CalculateEndDate(startDate, duration);
        endDateEdit <<= endDate;
        currentTask.endDate = endDate;
    }
}

Vector<String> TaskForm::GetAvailableTasks() const {
    Vector<String> availableTasks;
    
    for (const Task& task : currentProject.tasks) {
        // Don't include current task or tasks that already depend on current task
        if (task.id != currentTask.id && !currentTask.HasDependency(task.id)) {
            availableTasks.Add(task.name);
        }
    }
    
    return availableTasks;
}

Vector<String> TaskForm::GetAvailableResources() const {
    Vector<String> availableResources;
    
    for (const Resource& resource : currentProject.resources) {
        // Check if already assigned
        bool alreadyAssigned = false;
        for (const auto& assignment : currentTask.resourceAssignments) {
            if (assignment.first == resource.id) {
                alreadyAssigned = true;
                break;
            }
        }
        
        if (!alreadyAssigned) {
            availableResources.Add(resource.name);
        }
    }
    
    return availableResources;
}

int TaskForm::FindTaskIndex(int taskId) const {
    for (int i = 0; i < dependenciesArray.GetCount(); i++) {
        String taskName = dependenciesArray.Get(i);
        const Task* task = currentProject.GetTaskByName(taskName);
        if (task && task->id == taskId) {
            return i;
        }
    }
    return -1;
}

int TaskForm::FindResourceIndex(int resourceId) const {
    for (int i = 0; i < resourceArray.GetCount(); i++) {
        String resourceName = resourceArray.Get(i);
        const Resource* resource = currentProject.GetResourceByName(resourceName);
        if (resource && resource->id == resourceId) {
            return i;
        }
    }
    return -1;
}
