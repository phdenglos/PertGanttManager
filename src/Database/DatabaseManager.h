#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <Core/Core.h>
#include <Sqlite3/Sqlite3.h>
#include <vector>
#include <memory>
#include <mutex>

using namespace Upp;

class Project;
class Task;
class Resource;

class DatabaseManager {
public:
    DatabaseManager();
    ~DatabaseManager();

    // Connection management
    bool OpenDatabase(const String& dbPath);
    bool CreateDatabase(const String& dbPath);
    bool IsOpen() const;
    void CloseDatabase();

    // Database initialization
    bool InitializeDatabase();

    // Project operations
    int CreateProject(const String& name, const String& description = "");
    bool UpdateProject(int projectId, const String& name, const String& description = "");
    bool DeleteProject(int projectId);
    bool GetProject(int projectId, Project& project) const;
    Vector<int> GetAllProjectIds() const;
    Vector<String> GetAllProjectNames() const;

    // Task operations
    int CreateTask(int projectId, const String& name, double workload, int duration,
                   const Time& startDate, const Time& endDate, double progress, int priority,
                   const Color& color, double xPosition = 0.0, double yPosition = 0.0);
    bool UpdateTask(const Task& task);
    bool DeleteTask(int taskId);
    bool GetTask(int taskId, Task& task) const;
    Vector<int> GetAllTaskIds(int projectId) const;
    Vector<Task> GetAllTasks(int projectId) const;

    // Resource operations
    int CreateResource(int projectId, const String& name, double maxUtilizationRate, const String& description = "");
    bool UpdateResource(const Resource& resource);
    bool DeleteResource(int resourceId);
    bool GetResource(int resourceId, Resource& resource) const;
    Vector<int> GetAllResourceIds(int projectId) const;
    Vector<Resource> GetAllResources(int projectId) const;

    // Task dependency operations
    bool AddTaskDependency(int taskId, int dependsOnTaskId);
    bool RemoveTaskDependency(int taskId, int dependsOnTaskId);
    Vector<int> GetTaskDependencies(int taskId) const;
    Vector<int> GetDependentTasks(int taskId) const;
    bool ClearTaskDependencies(int taskId);

    // Resource assignment operations
    bool AddResourceAssignment(int taskId, int resourceId, double allocationRate);
    bool UpdateResourceAssignment(int taskId, int resourceId, double allocationRate);
    bool RemoveResourceAssignment(int taskId, int resourceId);
    Vector<std::pair<int, double>> GetTaskResourceAssignments(int taskId) const;
    bool ClearTaskResourceAssignments(int taskId);

    // Resource utilization operations
    bool UpdateResourceUtilization(int resourceId, const Time& date, double utilizationRate);
    double GetResourceUtilization(int resourceId, const Time& date) const;
    Vector<std::pair<Time, double>> GetResourceUtilizationProfile(int resourceId, 
                                                                  const Time& startDate, 
                                                                  const Time& endDate) const;

    // Project data operations
    bool SaveProject(const Project& project);
    bool LoadProject(int projectId, Project& project) const;
    bool DeleteProjectData(int projectId);

    // Transaction management
    bool BeginTransaction();
    bool CommitTransaction();
    bool RollbackTransaction();

    // Error handling
    String GetLastError() const;
    int GetLastErrorCode() const;

    // Thread safety
    std::mutex& GetMutex();

private:
    // Database connection
    SQLite3Session session;
    bool isOpen;
    String lastError;
    int lastErrorCode;
    mutable std::mutex dbMutex;

    // Helper methods
    bool ExecuteNonQuery(const String& sql);
    bool ExecuteQuery(const String& sql, SQLite3Statement& statement) const;
    int GetLastInsertId();
    bool TableExists(const String& tableName) const;

    // Disable copy and assignment
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;
};

// Singleton database manager
class DatabaseManagerSingleton {
public:
    static DatabaseManager& GetInstance();
    static void SetDatabasePath(const String& dbPath);
    static String GetDatabasePath();

private:
    static DatabaseManager instance;
    static String dbPath;
    static bool initialized;
};

#endif // DATABASE_MANAGER_H
