#include "MainWindow.h"
#include "../../Model/Project.h"
#include "../../Model/Task.h"
#include "../../Model/Resource.h"
#include "../../Database/DatabaseManager.h"
#include "../../Algorithms/Scheduler.h"
#include "../../Algorithms/CircularDependencyChecker.h"
#include "../../Export/Exporter.h"
#include "../Forms/ProjectForm.h"
#include "../Forms/TaskForm.h"
#include "../Forms/ResourceForm.h"
#include "../Diagrams/PertDiagram.h"
#include "../Diagrams/GanttDiagram.h"

using namespace Upp;

// MainWindow implementation
MainWindow::MainWindow() 
    : projectModified(false), dbManager(nullptr), scheduler(nullptr),
      dependencyChecker(nullptr), exportManager(nullptr), isUILocked(false),
      progressDialogVisible(false), schedulerRunning(false) {
    
    CtrlLayout(*this, "PertGanttManager - Project Management");
    
    // Initialize components
    dbManager = &DatabaseManagerSingleton::GetInstance();
    scheduler = &SchedulerSingleton::GetInstance();
    dependencyChecker = &CircularDependencyCheckerSingleton::GetInstance();
    exportManager = &ExportManagerSingleton::GetInstance();
    
    // Set up diagram callbacks
    SetupDiagramCallbacks();
    
    // Connect menu and toolbar actions
    NewProject <<= THIS(OnNewProject);
    OpenProject <<= THIS(OnOpenProject);
    SaveProject <<= THIS(OnSaveProject);
    SaveProjectAs <<= THIS(OnSaveProjectAs);
    Exit <<= THIS(OnExit);
    
    ImportFromJson <<= THIS(OnImportFromJson);
    ExportToJson <<= THIS(OnExportToJson);
    ExportToPng <<= THIS(OnExportToPng);
    ExportToQtf <<= THIS(OnExportToQtf);
    ExportToPowerPoint <<= THIS(OnExportToPowerPoint);
    
    Undo <<= THIS(OnUndo);
    Redo <<= THIS(OnRedo);
    Cut <<= THIS(OnCut);
    Copy <<= THIS(OnCopy);
    Paste <<= THIS(OnPaste);
    Delete <<= THIS(OnDelete);
    
    ShowPertDiagram <<= THIS(OnShowPertDiagram);
    ShowGanttDiagram <<= THIS(OnShowGanttDiagram);
    ShowResourceProfile <<= THIS(OnShowResourceProfile);
    ZoomIn <<= THIS(OnZoomIn);
    ZoomOut <<= THIS(OnZoomOut);
    FitToScreen <<= THIS(OnFitToScreen);
    
    RunScheduler <<= THIS(OnRunScheduler);
    CheckCircularDependencies <<= THIS(OnCheckCircularDependencies);
    About <<= THIS(OnAbout);
    
    // Connect array actions
    TaskArray.WhenAction <<= THIS(OnTaskArrayAction);
    ResourceArray.WhenAction <<= THIS(OnResourceArrayAction);
    ProjectList.WhenAction <<= THIS(OnProjectListAction);
    
    // Initialize UI
    UpdateUI();
    UpdateTitle();
    SetStatus("Ready");
    
    // Set default diagram
    diagramTabs.SetIndex(0); // Show PERT diagram by default
}

MainWindow::~MainWindow() {
    // Clean up
    if (schedulerThread && schedulerThread->joinable()) {
        scheduler->Cancel();
        schedulerThread->join();
    }
}

bool MainWindow::Initialize() {
    // Initialize database
    DatabaseManagerSingleton::SetDatabasePath(GetHomeDirFile("PertGanttManager.db"));
    
    if (!dbManager->IsOpen()) {
        if (!dbManager->CreateDatabase(DatabaseManagerSingleton::GetDatabasePath())) {
            ShowError("Failed to create database: " + dbManager->GetLastError());
            return false;
        }
    }
    
    // Load project list
    UpdateProjectList();
    
    return true;
}

void MainWindow::NewProject() {
    OnNewProject();
}

void MainWindow::OpenProject() {
    OnOpenProject();
}

void MainWindow::SaveProject() {
    OnSaveProject();
}

void MainWindow::SaveProjectAs() {
    OnSaveProjectAs();
}

void MainWindow::CloseProject() {
    OnCloseProject();
}

void MainWindow::ImportFromJson() {
    OnImportFromJson();
}

void MainWindow::ExportToJson() {
    OnExportToJson();
}

void MainWindow::ExportToPng() {
    OnExportToPng();
}

void MainWindow::ExportToQtf() {
    OnExportToQtf();
}

void MainWindow::ExportToPowerPoint() {
    OnExportToPowerPoint();
}

void MainWindow::Undo() {
    OnUndo();
}

void MainWindow::Redo() {
    OnRedo();
}

void MainWindow::Cut() {
    OnCut();
}

void MainWindow::Copy() {
    OnCopy();
}

void MainWindow::Paste() {
    OnPaste();
}

void MainWindow::Delete() {
    OnDelete();
}

void MainWindow::ShowPertDiagram() {
    OnShowPertDiagram();
}

void MainWindow::ShowGanttDiagram() {
    OnShowGanttDiagram();
}

void MainWindow::ShowResourceProfile() {
    OnShowResourceProfile();
}

void MainWindow::ZoomIn() {
    OnZoomIn();
}

void MainWindow::ZoomOut() {
    OnZoomOut();
}

void MainWindow::FitToScreen() {
    OnFitToScreen();
}

void MainWindow::RunScheduler() {
    OnRunScheduler();
}

void MainWindow::CheckCircularDependencies() {
    OnCheckCircularDependencies();
}

void MainWindow::About() {
    OnAbout();
}

void MainWindow::AddTask() {
    ShowTaskForm(nullptr);
}

void MainWindow::EditTask() {
    int index = TaskArray.GetCursor();
    if (index >= 0) {
        Task* task = currentProject.GetTaskByName(TaskArray.Get(index));
        if (task) {
            ShowTaskForm(task);
        }
    }
}

void MainWindow::RemoveTask() {
    int index = TaskArray.GetCursor();
    if (index >= 0) {
        String taskName = TaskArray.Get(index);
        if (PromptOKCancel("Delete Task", "Are you sure you want to delete this task?")) {
            currentProject.RemoveTaskByName(taskName);
            UpdateTaskList();
            RefreshDiagrams();
            SetStatus("Task deleted");
            projectModified = true;
        }
    }
}

void MainWindow::UpdateTaskList() {
    UpdateTaskArrays();
}

void MainWindow::AddResource() {
    ShowResourceForm(nullptr);
}

void MainWindow::EditResource() {
    int index = ResourceArray.GetCursor();
    if (index >= 0) {
        Resource* resource = currentProject.GetResourceByName(ResourceArray.Get(index));
        if (resource) {
            ShowResourceForm(resource);
        }
    }
}

void MainWindow::RemoveResource() {
    int index = ResourceArray.GetCursor();
    if (index >= 0) {
        String resourceName = ResourceArray.Get(index);
        if (PromptOKCancel("Delete Resource", "Are you sure you want to delete this resource?")) {
            currentProject.RemoveResourceByName(resourceName);
            UpdateResourceList();
            RefreshDiagrams();
            SetStatus("Resource deleted");
            projectModified = true;
        }
    }
}

void MainWindow::UpdateResourceList() {
    UpdateResourceArrays();
}

void MainWindow::EditProject() {
    ShowProjectForm();
}

void MainWindow::UpdateProjectInfo() {
    UpdateProjectInfo();
}

void MainWindow::OnTaskSelected(int taskId) {
    // Select task in task array
    const Task* task = currentProject.GetTaskById(taskId);
    if (task) {
        int index = TaskArray.Find(task->name);
        if (index >= 0) {
            TaskArray.SetCursor(index);
            TaskArray.EnsureVisible(index);
        }
    }
    
    SetStatus(Format("Task selected: %d", taskId));
}

void MainWindow::OnDependencyAdded(int fromTaskId, int toTaskId) {
    SetStatus(Format("Dependency added: %d -> %d", fromTaskId, toTaskId));
    projectModified = true;
}

void MainWindow::OnDependencyRemoved(int fromTaskId, int toTaskId) {
    SetStatus(Format("Dependency removed: %d -> %d", fromTaskId, toTaskId));
    projectModified = true;
}

void MainWindow::OnTaskMoved(int taskId) {
    SetStatus(Format("Task moved: %d", taskId));
    projectModified = true;
}

void MainWindow::OnDiagramChanged() {
    SetStatus("Diagram changed");
    projectModified = true;
}

void MainWindow::SetStatus(const String& message) {
    statusLabel <<= message;
    statusLabel.Refresh();
}

void MainWindow::ShowProgressDialog(const String& title, int maxValue) {
    progressDialog.SetTitle(title);
    progressBar.SetMax(maxValue);
    progressBar.SetValue(0);
    progressLabel <<= "Processing... 0%"
    progressDialog.Show();
    progressDialog.SetFocus();
    progressDialogVisible = true;
}

void MainWindow::UpdateProgress(int value, const String& message) {
    progressBar <<= value;
    progressLabel <<= message;
    progressDialog.Refresh();
}

void MainWindow::HideProgressDialog() {
    progressDialog.Hide();
    progressDialogVisible = false;
}

bool MainWindow::IsProgressDialogVisible() const {
    return progressDialogVisible;
}

void MainWindow::LockUI() {
    isUILocked = true;
    SetStatus("Processing... Please wait");
}

void MainWindow::UnlockUI() {
    isUILocked = false;
    SetStatus("Ready");
}

// Private methods
void MainWindow::SetupDiagramCallbacks() {
    // PERT diagram callbacks
    PertDiagram.WhenTaskSelected <<= THIS(OnTaskSelected);
    PertDiagram.WhenDependencyAdded <<= THIS(OnDependencyAdded);
    PertDiagram.WhenDependencyRemoved <<= THIS(OnDependencyRemoved);
    PertDiagram.WhenTaskMoved <<= THIS(OnTaskMoved);
    PertDiagram.WhenDiagramChanged <<= THIS(OnDiagramChanged);
    
    // Gantt diagram callbacks
    GanttDiagram.WhenTaskSelected <<= THIS(OnTaskSelected);
    GanttDiagram.WhenDependencyAdded <<= THIS(OnDependencyAdded);
    GanttDiagram.WhenDependencyRemoved <<= THIS(OnDependencyRemoved);
    GanttDiagram.WhenTaskMoved <<= THIS(OnTaskMoved);
    GanttDiagram.WhenDiagramChanged <<= THIS(OnDiagramChanged);
    
    // Set scheduler progress callback
    scheduler->SetProgressCallback([this](int value, const String& message) {
        UpdateSchedulerProgress(value, message);
    });
}

void MainWindow::OnNewProject() {
    if (projectModified && !PromptOKCancel("New Project", "You have unsaved changes. Create new project anyway?")) {
        return;
    }
    
    // Create new project
    currentProject = Project(0, "New Project", "");
    currentProjectPath.Clear();
    projectModified = false;
    
    UpdateUI();
    UpdateTitle();
    SetStatus("New project created");
}

void MainWindow::OnOpenProject() {
    if (projectModified && !PromptOKCancel("Open Project", "You have unsaved changes. Open another project anyway?")) {
        return;
    }
    
    // Show project list dialog
    int projectIndex = ProjectList.GetCursor();
    if (projectIndex >= 0) {
        String projectName = ProjectList.Get(projectIndex);
        
        // Find project ID by name
        Vector<int> projectIds = dbManager->GetAllProjectIds();
        for (int projectId : projectIds) {
            Project tempProject;
            if (dbManager->GetProject(projectId, tempProject) && tempProject.name == projectName) {
                if (LoadProjectFromDatabase(projectId)) {
                    currentProjectPath = GetHomeDirFile(projectName + ".pert");
                    projectModified = false;
                    UpdateUI();
                    UpdateTitle();
                    SetStatus("Project loaded: " + projectName);
                    return;
                }
                break;
            }
        }
    }
    
    // If no project selected or not found, show file dialog
    String filePath = GetOpenFilePath("Open Project", {".pert", ".json"});
    if (!filePath.IsEmpty()) {
        if (LoadProjectFromFile(filePath)) {
            currentProjectPath = filePath;
            projectModified = false;
            UpdateUI();
            UpdateTitle();
            SetStatus("Project loaded: " + filePath);
        }
    }
}

void MainWindow::OnSaveProject() {
    if (currentProjectPath.IsEmpty()) {
        OnSaveProjectAs();
        return;
    }
    
    if (SaveProjectToFile(currentProjectPath)) {
        projectModified = false;
        UpdateTitle();
        SetStatus("Project saved: " + currentProjectPath);
    }
}

void MainWindow::OnSaveProjectAs() {
    String filePath = GetSaveFilePath("Save Project As", ".pert");
    if (!filePath.IsEmpty()) {
        if (SaveProjectToFile(filePath)) {
            currentProjectPath = filePath;
            projectModified = false;
            UpdateTitle();
            SetStatus("Project saved: " + filePath);
        }
    }
}

void MainWindow::OnCloseProject() {
    if (projectModified && !PromptOKCancel("Close Project", "You have unsaved changes. Close project anyway?")) {
        return;
    }
    
    currentProject = Project();
    currentProjectPath.Clear();
    projectModified = false;
    
    UpdateUI();
    UpdateTitle();
    SetStatus("Project closed");
}

void MainWindow::OnExit() {
    if (projectModified && !PromptOKCancel("Exit", "You have unsaved changes. Exit anyway?")) {
        return;
    }
    
    Close();
}

void MainWindow::OnImportFromJson() {
    String filePath = GetOpenFilePath("Import from JSON", {".json"});
    if (!filePath.IsEmpty()) {
        if (LoadProjectFromFile(filePath)) {
            currentProjectPath.Clear();
            projectModified = false;
            UpdateUI();
            UpdateTitle();
            SetStatus("Project imported from JSON: " + filePath);
        }
    }
}

void MainWindow::OnExportToJson() {
    if (currentProject.IsValid()) {
        String filePath = GetSaveFilePath("Export to JSON", ".json");
        if (!filePath.IsEmpty()) {
            ExportResult result = exportManager->ExportToFile(currentProject, filePath, ExportFormat::JSON);
            if (result.success) {
                SetStatus(result.message);
            } else {
                ShowError(result.message);
            }
        }
    } else {
        ShowError("No project to export");
    }
}

void MainWindow::OnExportToPng() {
    if (diagramTabs.GetIndex() == 0) {
        // PERT diagram
        String filePath = GetSaveFilePath("Export PERT to PNG", ".png");
        if (!filePath.IsEmpty()) {
            ExportResult result = exportManager->ExportToFile(PertDiagram, filePath, ExportFormat::PNG);
            if (result.success) {
                SetStatus(result.message);
            } else {
                ShowError(result.message);
            }
        }
    } else {
        // Gantt diagram
        String filePath = GetSaveFilePath("Export Gantt to PNG", ".png");
        if (!filePath.IsEmpty()) {
            ExportResult result = exportManager->ExportToFile(GanttDiagram, filePath, ExportFormat::PNG);
            if (result.success) {
                SetStatus(result.message);
            } else {
                ShowError(result.message);
            }
        }
    }
}

void MainWindow::OnExportToQtf() {
    if (diagramTabs.GetIndex() == 0) {
        // PERT diagram
        String filePath = GetSaveFilePath("Export PERT to QTF", ".qtf");
        if (!filePath.IsEmpty()) {
            ExportResult result = exportManager->ExportToFile(PertDiagram, filePath, ExportFormat::QTF);
            if (result.success) {
                SetStatus(result.message);
            } else {
                ShowError(result.message);
            }
        }
    } else {
        // Gantt diagram
        String filePath = GetSaveFilePath("Export Gantt to QTF", ".qtf");
        if (!filePath.IsEmpty()) {
            ExportResult result = exportManager->ExportToFile(GanttDiagram, filePath, ExportFormat::QTF);
            if (result.success) {
                SetStatus(result.message);
            } else {
                ShowError(result.message);
            }
        }
    }
}

void MainWindow::OnExportToPowerPoint() {
    if (diagramTabs.GetIndex() == 0) {
        // PERT diagram
        String filePath = GetSaveFilePath("Export PERT to PowerPoint", ".pptx");
        if (!filePath.IsEmpty()) {
            ExportResult result = exportManager->ExportToFile(PertDiagram, filePath, ExportFormat::POWERPOINT);
            if (result.success) {
                SetStatus(result.message);
            } else {
                ShowError(result.message);
            }
        }
    } else {
        // Gantt diagram
        String filePath = GetSaveFilePath("Export Gantt to PowerPoint", ".pptx");
        if (!filePath.IsEmpty()) {
            ExportResult result = exportManager->ExportToFile(GanttDiagram, filePath, ExportFormat::POWERPOINT);
            if (result.success) {
                SetStatus(result.message);
            } else {
                ShowError(result.message);
            }
        }
    }
}

void MainWindow::OnUndo() {
    SetStatus("Undo not implemented yet");
}

void MainWindow::OnRedo() {
    SetStatus("Redo not implemented yet");
}

void MainWindow::OnCut() {
    SetStatus("Cut not implemented yet");
}

void MainWindow::OnCopy() {
    SetStatus("Copy not implemented yet");
}

void MainWindow::OnPaste() {
    SetStatus("Paste not implemented yet");
}

void MainWindow::OnDelete() {
    if (diagramTabs.GetIndex() == 0) {
        // Delete selected tasks in PERT diagram
        const Vector<int>& selected = PertDiagram.GetSelectedNodes();
        for (int taskId : selected) {
            currentProject.RemoveTask(taskId);
        }
    } else {
        // Delete selected tasks in Gantt diagram
        const Vector<int>& selected = GanttDiagram.GetSelectedBars();
        for (int taskId : selected) {
            currentProject.RemoveTask(taskId);
        }
    }
    
    UpdateTaskList();
    RefreshDiagrams();
    SetStatus("Tasks deleted");
    projectModified = true;
}

void MainWindow::OnShowPertDiagram() {
    diagramTabs.SetIndex(0);
}

void MainWindow::OnShowGanttDiagram() {
    diagramTabs.SetIndex(1);
}

void MainWindow::OnShowResourceProfile() {
    if (diagramTabs.GetIndex() == 0) {
        PertDiagram.ShowResourceProfile(!PertDiagram.IsResourceProfileShown());
    } else {
        GanttDiagram.ShowResourceProfile(!GanttDiagram.IsResourceProfileShown());
    }
}

void MainWindow::OnZoomIn() {
    if (diagramTabs.GetIndex() == 0) {
        PertDiagram.SetZoom(PertDiagram.GetZoom() * 1.1);
    } else {
        GanttDiagram.SetZoom(GanttDiagram.GetZoom() * 1.1);
    }
}

void MainWindow::OnZoomOut() {
    if (diagramTabs.GetIndex() == 0) {
        PertDiagram.SetZoom(PertDiagram.GetZoom() / 1.1);
    } else {
        GanttDiagram.SetZoom(GanttDiagram.GetZoom() / 1.1);
    }
}

void MainWindow::OnFitToScreen() {
    if (diagramTabs.GetIndex() == 0) {
        PertDiagram.FitToScreen();
    } else {
        GanttDiagram.FitToScreen();
    }
}

void MainWindow::OnRunScheduler() {
    if (!currentProject.IsValid()) {
        ShowError("No project to schedule");
        return;
    }
    
    if (currentProject.tasks.IsEmpty()) {
        ShowError("No tasks to schedule");
        return;
    }
    
    // Check for circular dependencies first
    if (dependencyChecker->HasCycle(currentProject)) {
        CircularDependencyCheckResult result = dependencyChecker->CheckProject(currentProject);
        ShowError(result.messages[0]);
        return;
    }
    
    // Show progress dialog
    ShowProgressDialog("Running Scheduler", 100);
    
    // Run scheduler in thread
    schedulerRunning = true;
    schedulerThread = std::make_unique<std::thread>([this]() {
        try {
            SchedulingResult result = scheduler->ScheduleProject(currentProject, true);
            
            // Update UI on main thread
            GuiLock __;
            HideProgressDialog();
            
            if (result.success) {
                currentProject = result.scheduledTasks;
                RefreshDiagrams();
                SetStatus(result.message);
                projectModified = true;
            } else {
                ShowError(result.message);
            }
        } catch (const std::exception& e) {
            GuiLock __;
            HideProgressDialog();
            ShowError(e.what());
        }
        
        schedulerRunning = false;
    });
}

void MainWindow::OnCheckCircularDependencies() {
    if (!currentProject.IsValid()) {
        ShowError("No project to check");
        return;
    }
    
    CircularDependencyCheckResult result = dependencyChecker->CheckProject(currentProject);
    
    if (result.hasCircularDependencies) {
        String message = "Circular dependencies found:\n\n";
        for (const String& msg : result.messages) {
            message += msg + "\n";
        }
        Exclamation(message);
    } else {
        ShowInfo(result.messages[0]);
    }
}

void MainWindow::OnAbout() {
    PromptOK("About PertGanttManager", 
             "PertGanttManager v1.0\n\n"
             "A project management tool for creating PERT and Gantt diagrams.\n\n"
             "Built with U++ framework");
}

void MainWindow::OnTaskArrayAction() {
    int index = TaskArray.GetCursor();
    if (index >= 0) {
        String taskName = TaskArray.Get(index);
        const Task* task = currentProject.GetTaskByName(taskName);
        if (task) {
            // Select task in diagrams
            PertDiagram.SelectNode(task->id);
            GanttDiagram.SelectBar(task->id);
        }
    }
}

void MainWindow::OnResourceArrayAction() {
    int index = ResourceArray.GetCursor();
    if (index >= 0) {
        String resourceName = ResourceArray.Get(index);
        const Resource* resource = currentProject.GetResourceByName(resourceName);
        if (resource) {
            // Could highlight tasks that use this resource
            SetStatus("Resource selected: " + resourceName);
        }
    }
}

void MainWindow::OnProjectListAction() {
    // Project selection changed
    int index = ProjectList.GetCursor();
    if (index >= 0) {
        SetStatus("Project selected: " + ProjectList.Get(index));
    }
}

void MainWindow::UpdateSchedulerProgress(int value, const String& message) {
    GuiLock __;
    if (progressDialogVisible) {
        UpdateProgress(value, message);
    }
}

bool MainWindow::SaveProjectToFile(const String& filePath) {
    try {
        // Save as JSON
        Json json;
        currentProject.ToJson(json);
        
        String content = json.ToString();
        if (SaveFile(filePath, content)) {
            return true;
        }
    } catch (const std::exception& e) {
        ShowError("Failed to save project: " + String(e.what()));
    }
    
    return false;
}

bool MainWindow::LoadProjectFromFile(const String& filePath) {
    try {
        // Load JSON
        String content = LoadFile(filePath);
        if (content.IsEmpty()) {
            return false;
        }
        
        Json json;
        if (ParseJSON(content, json)) {
            currentProject.FromJson(json);
            return true;
        }
    } catch (const std::exception& e) {
        ShowError("Failed to load project: " + String(e.what()));
    }
    
    return false;
}

bool MainWindow::SaveProjectToDatabase() {
    if (currentProject.id <= 0) {
        // New project, need to create it first
        currentProject.id = dbManager->CreateProject(currentProject.name, currentProject.description);
        if (currentProject.id <= 0) {
            return false;
        }
    }
    
    return dbManager->SaveProject(currentProject);
}

bool MainWindow::LoadProjectFromDatabase(int projectId) {
    return dbManager->LoadProject(projectId, currentProject);
}

void MainWindow::UpdateUI() {
    // Update project info
    UpdateTaskArrays();
    UpdateResourceArrays();
    UpdateProjectList();
    
    // Update diagrams
    RefreshDiagrams();
    
    // Update menu states
    SaveProject.Enable(currentProject.IsValid());
    SaveProjectAs.Enable(currentProject.IsValid());
    ExportToJson.Enable(currentProject.IsValid());
    ExportToPng.Enable(currentProject.IsValid());
    ExportToQtf.Enable(currentProject.IsValid());
    ExportToPowerPoint.Enable(currentProject.IsValid());
    
    RunScheduler.Enable(currentProject.IsValid() && !currentProject.tasks.IsEmpty());
    CheckCircularDependencies.Enable(currentProject.IsValid() && !currentProject.tasks.IsEmpty());
    
    EditProject.Enable(currentProject.IsValid());
    AddTask.Enable(currentProject.IsValid());
    EditTask.Enable(currentProject.IsValid() && TaskArray.GetCount() > 0);
    RemoveTask.Enable(currentProject.IsValid() && TaskArray.GetCount() > 0);
    
    AddResource.Enable(currentProject.IsValid());
    EditResource.Enable(currentProject.IsValid() && ResourceArray.GetCount() > 0);
    RemoveResource.Enable(currentProject.IsValid() && ResourceArray.GetCount() > 0);
}

void MainWindow::UpdateTitle() {
    String title = "PertGanttManager";
    if (currentProject.IsValid()) {
        title += " - " + currentProject.name;
        if (projectModified) {
            title += " *";
        }
    }
    SetTitle(title);
}

void MainWindow::UpdateTaskArrays() {
    TaskArray.Clear();
    for (const Task& task : currentProject.tasks) {
        TaskArray.Add(task.name);
    }
    TaskArray.Refresh();
}

void MainWindow::UpdateResourceArrays() {
    ResourceArray.Clear();
    for (const Resource& resource : currentProject.resources) {
        ResourceArray.Add(resource.name);
    }
    ResourceArray.Refresh();
}

void MainWindow::UpdateProjectList() {
    ProjectList.Clear();
    Vector<String> projectNames = dbManager->GetAllProjectNames();
    for (const String& name : projectNames) {
        ProjectList.Add(name);
    }
    ProjectList.Refresh();
}

void MainWindow::ShowTaskForm(Task* task) {
    TaskForm form;
    
    if (task) {
        form.SetTask(*task, currentProject);
    } else {
        // New task
        Task newTask;
        newTask.priority = 3;
        newTask.progress = 0.0;
        newTask.workload = 1.0;
        newTask.duration = 1;
        newTask.color = RandomColor();
        form.SetTask(newTask, currentProject);
    }
    
    form.WhenSave <<= [this, task]() {
        if (task) {
            // Update existing task
            Task updatedTask;
            form.GetTask(updatedTask);
            currentProject.UpdateTask(updatedTask);
        } else {
            // Add new task
            Task newTask;
            form.GetTask(newTask);
            currentProject.AddTask(newTask);
        }
        UpdateTaskList();
        RefreshDiagrams();
        projectModified = true;
    };
    
    form.WhenCancel <<= [this]() {
        // Do nothing
    };
    
    form.Execute();
}

void MainWindow::ShowResourceForm(Resource* resource) {
    ResourceForm form;
    
    if (resource) {
        form.SetResource(*resource, currentProject);
    } else {
        // New resource
        Resource newResource;
        newResource.maxUtilizationRate = 100.0;
        form.SetResource(newResource, currentProject);
    }
    
    form.WhenSave <<= [this, resource]() {
        if (resource) {
            // Update existing resource
            Resource updatedResource;
            form.GetResource(updatedResource);
            currentProject.UpdateResource(updatedResource);
        } else {
            // Add new resource
            Resource newResource;
            form.GetResource(newResource);
            currentProject.AddResource(newResource);
        }
        UpdateResourceList();
        RefreshDiagrams();
        projectModified = true;
    };
    
    form.WhenCancel <<= [this]() {
        // Do nothing
    };
    
    form.WhenViewProfile <<= [this, resource]() {
        if (resource) {
            // Show resource profile
            // Could open a dialog showing resource utilization over time
            ShowInfo("Resource profile for: " + resource->name);
        }
    };
    
    form.Execute();
}

void MainWindow::ShowProjectForm() {
    ProjectForm form;
    form.SetProject(currentProject);
    
    form.WhenSave <<= [this]() {
        Project updatedProject;
        form.GetProject(updatedProject);
        currentProject.name = updatedProject.name;
        currentProject.description = updatedProject.description;
        UpdateTitle();
        projectModified = true;
    };
    
    form.WhenCancel <<= [this]() {
        // Do nothing
    };
    
    form.Execute();
}

void MainWindow::RefreshDiagrams() {
    PertDiagram.SetProject(currentProject);
    GanttDiagram.SetProject(currentProject);
}

String MainWindow::GetOpenFilePath(const String& title, const Vector<String>& extensions) {
    FileSel fs;
    fs.Type(title, extensions);
    if (fs.ExecuteOpen()) {
        return fs.Get();
    }
    return String();
}

String MainWindow::GetSaveFilePath(const String& title, const String& defaultExtension) {
    FileSel fs;
    fs.Type(title, {defaultExtension});
    if (fs.ExecuteSaveAs()) {
        return fs.Get();
    }
    return String();
}

void MainWindow::ShowError(const String& message) {
    Exclamation(message);
    SetStatus("Error: " + message);
}

void MainWindow::ShowInfo(const String& message) {
    PromptOK("Information", message);
    SetStatus(message);
}
