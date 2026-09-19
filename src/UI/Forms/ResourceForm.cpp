#include "ResourceForm.h"
#include "../../Model/Project.h"
#include "../../Model/Resource.h"

using namespace Upp;

ResourceForm::ResourceForm() {
    CtrlLayout(*this, "Resource Properties");
    
    // Connect buttons
    okButton <<= THIS(OnOK);
    cancelButton <<= THIS(OnCancel);
    viewProfileButton <<= THIS(OnViewProfile);
    
    // Set default values
    nameEdit.SetFocus();
}

ResourceForm::~ResourceForm() {}

void ResourceForm::SetResource(const Resource& resource, const Project& project) {
    currentResource = resource;
    currentProject = project;
    
    // Set form values
    nameEdit <<= currentResource.name;
    maxUtilizationEdit <<= currentResource.maxUtilizationRate;
    descriptionEdit <<= currentResource.description;
    
    // Update current tasks list
    UpdateCurrentTasksList();
}

void ResourceForm::GetResource(Resource& resource) const {
    resource = currentResource;
    
    // Update from form values
    resource.name = ~nameEdit;
    resource.maxUtilizationRate = ~maxUtilizationEdit;
    resource.description = ~descriptionEdit;
}

void ResourceForm::SetProject(const Project& project) {
    currentProject = project;
    UpdateCurrentTasksList();
}

void ResourceForm::OnOK() {
    // Validate input
    if (~nameEdit.IsEmpty()) {
        GetResource(currentResource);
        
        // Additional validation
        if (currentResource.maxUtilizationRate < 0 || currentResource.maxUtilizationRate > 100) {
            Exclamation("Max utilization must be between 0 and 100%!");
            maxUtilizationEdit.SetFocus();
            return;
        }
        
        WhenSave();
        Close();
    } else {
        Exclamation("Resource name cannot be empty!");
        nameEdit.SetFocus();
    }
}

void ResourceForm::OnCancel() {
    WhenCancel();
    Close();
}

void ResourceForm::OnViewProfile() {
    WhenViewProfile();
}

void ResourceForm::UpdateCurrentTasksList() {
    currentTasksArray.Clear();
    
    // Find tasks that use this resource
    for (const Task& task : currentProject.tasks) {
        for (const auto& assignment : task.resourceAssignments) {
            if (assignment.first == currentResource.id) {
                currentTasksArray.Add(task.name);
                break;
            }
        }
    }
    
    currentTasksArray.Refresh();
}
