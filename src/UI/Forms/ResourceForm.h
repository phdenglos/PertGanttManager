#ifndef RESOURCE_FORM_H
#define RESOURCE_FORM_H

#include <CtrlLib/CtrlLib.h>

using namespace Upp;

class Project;
class Resource;

// Resource form dialog
class ResourceForm : public WithResourceFormLayout<TopWindow> {
public:
    typedef ResourceForm CLASSNAME;
    
    ResourceForm();
    ~ResourceForm();
    
    // Initialize with resource data and project context
    void SetResource(const Resource& resource, const Project& project);
    
    // Get updated resource data
    void GetResource(Resource& resource) const;
    
    // Set project context (for task list)
    void SetProject(const Project& project);
    
    // Event callbacks
    Callback WhenSave;
    Callback WhenCancel;
    Callback WhenViewProfile;
    
private:
    // Resource data
    Resource currentResource;
    Project currentProject;
    
    // Event handlers
    void OnOK();
    void OnCancel();
    void OnViewProfile();
    
    // Update form controls
    void UpdateCurrentTasksList();
};

#endif // RESOURCE_FORM_H
