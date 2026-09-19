#ifndef TASK_FORM_H
#define TASK_FORM_H

#include <CtrlLib/CtrlLib.h>
#include <vector>

using namespace Upp;

class Project;
class Task;

// Task form dialog
class TaskForm : public WithTaskFormLayout<TopWindow> {
public:
    typedef TaskForm CLASSNAME;
    
    TaskForm();
    ~TaskForm();
    
    // Initialize with task data and project context
    void SetTask(const Task& task, const Project& project);
    
    // Get updated task data
    void GetTask(Task& task) const;
    
    // Set project context (for dependency and resource lists)
    void SetProject(const Project& project);
    
    // Event callbacks
    Callback WhenSave;
    Callback WhenCancel;
    
private:
    // Task data
    Task currentTask;
    Project currentProject;
    
    // UI state
    bool updatingControls;
    
    // Event handlers
    void OnOK();
    void OnCancel();
    void OnPriorityChange();
    void OnProgressChange();
    void OnProgressSliderChange();
    void OnColorChange();
    void OnAddDependency();
    void OnRemoveDependency();
    void OnAddResource();
    void OnRemoveResource();
    
    // Update form controls
    void UpdateDependenciesList();
    void UpdateResourcesList();
    void UpdateEndDate();
    
    // Helper methods
    Vector<String> GetAvailableTasks() const;
    Vector<String> GetAvailableResources() const;
    int FindTaskIndex(int taskId) const;
    int FindResourceIndex(int resourceId) const;
};

#endif // TASK_FORM_H
