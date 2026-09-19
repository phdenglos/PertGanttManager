#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <CtrlLib/CtrlLib.h>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>

using namespace Upp;

class Project;
class Task;
class Resource;
class PertDiagram;
class GanttDiagram;
class DatabaseManager;
class Scheduler;
class CircularDependencyChecker;
class ExportManager;

// Forward declarations
class MainWindow;

// Main application window
class MainWindow : public WithMainWindowLayout<TopWindow> {
public:
    typedef MainWindow CLASSNAME;
    
    MainWindow();
    ~MainWindow();
    
    // Initialization
    bool Initialize();
    
    // Project management
    void NewProject();
    void OpenProject();
    void SaveProject();
    void SaveProjectAs();
    void CloseProject();
    
    // File operations
    void ImportFromJson();
    void ExportToJson();
    void ExportToPng();
    void ExportToQtf();
    void ExportToPowerPoint();
    
    // Edit operations
    void Undo();
    void Redo();
    void Cut();
    void Copy();
    void Paste();
    void Delete();
    
    // View operations
    void ShowPertDiagram();
    void ShowGanttDiagram();
    void ShowResourceProfile();
    void ZoomIn();
    void ZoomOut();
    void FitToScreen();
    
    // Tools operations
    void RunScheduler();
    void CheckCircularDependencies();
    
    // Help operations
    void About();
    
    // Task operations
    void AddTask();
    void EditTask();
    void RemoveTask();
    void UpdateTaskList();
    
    // Resource operations
    void AddResource();
    void EditResource();
    void RemoveResource();
    void UpdateResourceList();
    
    // Project operations
    void EditProject();
    void UpdateProjectInfo();
    
    // Diagram interaction
    void OnTaskSelected(int taskId);
    void OnDependencyAdded(int fromTaskId, int toTaskId);
    void OnDependencyRemoved(int fromTaskId, int toTaskId);
    void OnTaskMoved(int taskId);
    void OnDiagramChanged();
    
    // Status updates
    void SetStatus(const String& message);
    
    // Progress dialog
    void ShowProgressDialog(const String& title, int maxValue);
    void UpdateProgress(int value, const String& message);
    void HideProgressDialog();
    bool IsProgressDialogVisible() const;
    
    // Thread safety
    void LockUI();
    void UnlockUI();
    
private:
    // Current project
    Project currentProject;
    String currentProjectPath;
    bool projectModified;
    
    // Database
    DatabaseManager* dbManager;
    
    // Scheduler
    Scheduler* scheduler;
    std::unique_ptr<std::thread> schedulerThread;
    std::atomic<bool> schedulerRunning;
    
    // Circular dependency checker
    CircularDependencyChecker* dependencyChecker;
    
    // Export manager
    ExportManager* exportManager;
    
    // UI state
    bool isUILocked;
    
    // Progress dialog state
    bool progressDialogVisible;
    
    // Event handlers
    void OnNewProject();
    void OnOpenProject();
    void OnSaveProject();
    void OnSaveProjectAs();
    void OnCloseProject();
    void OnExit();
    
    void OnImportFromJson();
    void OnExportToJson();
    void OnExportToPng();
    void OnExportToQtf();
    void OnExportToPowerPoint();
    
    void OnUndo();
    void OnRedo();
    void OnCut();
    void OnCopy();
    void OnPaste();
    void OnDelete();
    
    void OnShowPertDiagram();
    void OnShowGanttDiagram();
    void OnShowResourceProfile();
    void OnZoomIn();
    void OnZoomOut();
    void OnFitToScreen();
    
    void OnRunScheduler();
    void OnCheckCircularDependencies();
    void OnAbout();
    
    // Task list callbacks
    void OnTaskArrayAction();
    void OnResourceArrayAction();
    void OnProjectListAction();
    
    // Diagram callbacks
    void SetupDiagramCallbacks();
    
    // Scheduler thread
    void SchedulerThreadFunction();
    void UpdateSchedulerProgress(int value, const String& message);
    
    // Helper methods
    bool SaveProjectToFile(const String& filePath);
    bool LoadProjectFromFile(const String& filePath);
    bool SaveProjectToDatabase();
    bool LoadProjectFromDatabase(int projectId);
    
    void UpdateUI();
    void UpdateTitle();
    void UpdateTaskArrays();
    void UpdateResourceArrays();
    void UpdateProjectList();
    
    void ShowTaskForm(Task* task = nullptr);
    void ShowResourceForm(Resource* resource = nullptr);
    void ShowProjectForm();
    
    void RefreshDiagrams();
    
    // File dialog helpers
    String GetOpenFilePath(const String& title, const Vector<String>& extensions);
    String GetSaveFilePath(const String& title, const String& defaultExtension);
    
    // Message helpers
    void ShowError(const String& message);
    void ShowInfo(const String& message);
    
    // Disable copy and assignment
    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
};

#endif // MAIN_WINDOW_H
