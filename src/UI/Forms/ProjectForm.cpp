#include "ProjectForm.h"
#include "../../Model/Project.h"

using namespace Upp;

ProjectForm::ProjectForm() {
    CtrlLayout(*this, "Project Properties");
    
    // Connect buttons
    okButton <<= THIS(OnOK);
    cancelButton <<= THIS(OnCancel);
    
    // Set default values
    nameEdit.SetFocus();
}

ProjectForm::~ProjectForm() {}

void ProjectForm::SetProject(const Project& project) {
    currentProject = project;
    
    // Set form values
    nameEdit <<= currentProject.name;
    descriptionEdit <<= currentProject.description;
    createdDateEdit <<= currentProject.createdAt;
    updatedDateEdit <<= currentProject.updatedAt;
    
    // Update statistics
    UpdateStatistics();
}

void ProjectForm::GetProject(Project& project) const {
    project = currentProject;
    
    // Update from form values
    project.name = ~nameEdit;
    project.description = ~descriptionEdit;
}

void ProjectForm::OnOK() {
    // Validate input
    if (~nameEdit.IsEmpty()) {
        currentProject.name = ~nameEdit;
        currentProject.description = ~descriptionEdit;
        
        WhenSave();
        Close();
    } else {
        Exclamation("Project name cannot be empty!");
        nameEdit.SetFocus();
    }
}

void ProjectForm::OnCancel() {
    WhenCancel();
    Close();
}

void ProjectForm::UpdateStatistics() {
    totalTasksEdit <<= currentProject.tasks.GetCount();
    totalResourcesEdit <<= currentProject.resources.GetCount();
    totalWorkloadEdit <<= currentProject.GetTotalWorkload();
    completedWorkloadEdit <<= currentProject.GetCompletedWorkload();
    overallProgressEdit <<= currentProject.GetOverallProgress();
}
