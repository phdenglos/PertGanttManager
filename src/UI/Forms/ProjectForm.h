#ifndef PROJECT_FORM_H
#define PROJECT_FORM_H

#include <CtrlLib/CtrlLib.h>

using namespace Upp;

class Project;

// Project form dialog
class ProjectForm : public WithProjectFormLayout<TopWindow> {
public:
    typedef ProjectForm CLASSNAME;
    
    ProjectForm();
    ~ProjectForm();
    
    // Initialize with project data
    void SetProject(const Project& project);
    
    // Get updated project data
    void GetProject(Project& project) const;
    
    // Event callbacks
    Callback WhenSave;
    Callback WhenCancel;
    
private:
    // Project data
    Project currentProject;
    
    // Event handlers
    void OnOK();
    void OnCancel();
    
    // Update statistics
    void UpdateStatistics();
};

#endif // PROJECT_FORM_H
