#include "DatabaseManager.h"
#include "../Model/Project.h"
#include "../Model/Task.h"
#include "../Model/Resource.h"

using namespace Upp;

// Database schema SQL
static const char* DATABASE_SCHEMA = 
    "-- Projects table\n"
    "CREATE TABLE IF NOT EXISTS projects (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    name TEXT NOT NULL UNIQUE,\n"
    "    description TEXT,\n"
    "    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,\n"
    "    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP\n"
    ");\n\n"
    
    "-- Resources table\n"
    "CREATE TABLE IF NOT EXISTS resources (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    project_id INTEGER NOT NULL,\n"
    "    name TEXT NOT NULL,\n"
    "    max_utilization_rate REAL NOT NULL DEFAULT 100.0,\n"
    "    description TEXT,\n"
    "    FOREIGN KEY (project_id) REFERENCES projects(id) ON DELETE CASCADE,\n"
    "    UNIQUE(project_id, name)\n"
    ");\n\n"
    
    "-- Tasks table\n"
    "CREATE TABLE IF NOT EXISTS tasks (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    project_id INTEGER NOT NULL,\n"
    "    name TEXT NOT NULL,\n"
    "    workload REAL NOT NULL DEFAULT 1.0,\n"
    "    duration INTEGER NOT NULL DEFAULT 1,\n"
    "    start_date TEXT,\n"
    "    end_date TEXT,\n"
    "    progress REAL NOT NULL DEFAULT 0.0,\n"
    "    priority INTEGER NOT NULL DEFAULT 3,\n"
    "    color TEXT NOT NULL DEFAULT '#FFFFFFFF',\n"
    "    x_position REAL NOT NULL DEFAULT 0.0,\n"
    "    y_position REAL NOT NULL DEFAULT 0.0,\n"
    "    FOREIGN KEY (project_id) REFERENCES projects(id) ON DELETE CASCADE,\n"
    "    UNIQUE(project_id, name)\n"
    ");\n\n"
    
    "-- Task dependencies table\n"
    "CREATE TABLE IF NOT EXISTS task_dependencies (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    task_id INTEGER NOT NULL,\n"
    "    depends_on_task_id INTEGER NOT NULL,\n"
    "    FOREIGN KEY (task_id) REFERENCES tasks(id) ON DELETE CASCADE,\n"
    "    FOREIGN KEY (depends_on_task_id) REFERENCES tasks(id) ON DELETE CASCADE,\n"
    "    UNIQUE(task_id, depends_on_task_id),\n"
    "    CHECK(task_id != depends_on_task_id)\n"
    ");\n\n"
    
    "-- Resource assignments to tasks\n"
    "CREATE TABLE IF NOT EXISTS task_resources (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    task_id INTEGER NOT NULL,\n"
    "    resource_id INTEGER NOT NULL,\n"
    "    allocation_rate REAL NOT NULL DEFAULT 100.0,\n"
    "    FOREIGN KEY (task_id) REFERENCES tasks(id) ON DELETE CASCADE,\n"
    "    FOREIGN KEY (resource_id) REFERENCES resources(id) ON DELETE CASCADE,\n"
    "    UNIQUE(task_id, resource_id)\n"
    ");\n\n"
    
    "-- Resource utilization profiles\n"
    "CREATE TABLE IF NOT EXISTS resource_utilization (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    resource_id INTEGER NOT NULL,\n"
    "    date TEXT NOT NULL,\n"
    "    utilization_rate REAL NOT NULL DEFAULT 0.0,\n"
    "    FOREIGN KEY (resource_id) REFERENCES resources(id) ON DELETE CASCADE,\n"
    "    UNIQUE(resource_id, date)\n"
    ");\n\n"
    
    "-- Create indexes\n"
    "CREATE INDEX IF NOT EXISTS idx_projects_name ON projects(name);\n"
    "CREATE INDEX IF NOT EXISTS idx_tasks_project ON tasks(project_id);\n"
    "CREATE INDEX IF NOT EXISTS idx_tasks_priority ON tasks(priority);\n"
    "CREATE INDEX IF NOT EXISTS idx_task_dependencies_task ON task_dependencies(task_id);\n"
    "CREATE INDEX IF NOT EXISTS idx_task_dependencies_depends ON task_dependencies(depends_on_task_id);\n"
    "CREATE INDEX IF NOT EXISTS idx_resources_project ON resources(project_id);\n"
    "CREATE INDEX IF NOT EXISTS idx_task_resources_task ON task_resources(task_id);\n"
    "CREATE INDEX IF NOT EXISTS idx_task_resources_resource ON task_resources(resource_id);\n";

DatabaseManager::DatabaseManager() : isOpen(false), lastErrorCode(0) {}

DatabaseManager::~DatabaseManager() {
    CloseDatabase();
}

bool DatabaseManager::OpenDatabase(const String& dbPath) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    if (isOpen) {
        lastError = "Database is already open";
        lastErrorCode = -1;
        return false;
    }
    
    try {
        session.Open(dbPath);
        isOpen = true;
        lastError.Clear();
        lastErrorCode = 0;
        return true;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        isOpen = false;
        return false;
    }
}

bool DatabaseManager::CreateDatabase(const String& dbPath) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    // First, try to open the database
    if (!OpenDatabase(dbPath)) {
        return false;
    }
    
    // Execute the schema
    try {
        session.Execute(DATABASE_SCHEMA);
        return true;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        CloseDatabase();
        return false;
    }
}

bool DatabaseManager::IsOpen() const {
    std::lock_guard<std::mutex> lock(dbMutex);
    return isOpen;
}

void DatabaseManager::CloseDatabase() {
    std::lock_guard<std::mutex> lock(dbMutex);
    if (isOpen) {
        session.Close();
        isOpen = false;
    }
}

bool DatabaseManager::InitializeDatabase() {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    if (!isOpen) {
        lastError = "Database is not open";
        lastErrorCode = -1;
        return false;
    }
    
    try {
        // Check if tables exist
        if (!TableExists("projects")) {
            session.Execute(DATABASE_SCHEMA);
        }
        return true;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return false;
    }
}

bool DatabaseManager::ExecuteNonQuery(const String& sql) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    if (!isOpen) {
        lastError = "Database is not open";
        lastErrorCode = -1;
        return false;
    }
    
    try {
        session.Execute(sql);
        lastError.Clear();
        lastErrorCode = 0;
        return true;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return false;
    }
}

bool DatabaseManager::ExecuteQuery(const String& sql, SQLite3Statement& statement) const {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    if (!isOpen) {
        lastError = "Database is not open";
        lastErrorCode = -1;
        return false;
    }
    
    try {
        statement.Execute(sql);
        lastError.Clear();
        lastErrorCode = 0;
        return true;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return false;
    }
}

int DatabaseManager::GetLastInsertId() {
    std::lock_guard<std::mutex> lock(dbMutex);
    return session.GetLastInsertId();
}

bool DatabaseManager::TableExists(const String& tableName) const {
    SQLite3Statement stmt;
    String sql = "SELECT name FROM sqlite_master WHERE type='table' AND name=?";
    
    if (!ExecuteQuery(sql, stmt)) {
        return false;
    }
    
    stmt.Bind(1, tableName);
    return stmt.Fetch();
}

int DatabaseManager::CreateProject(const String& name, const String& description) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    String sql = "INSERT INTO projects (name, description) VALUES (?, ?)";
    SQLite3Statement stmt;
    
    try {
        stmt.Prepare(sql);
        stmt.Bind(1, name);
        stmt.Bind(2, description);
        stmt.Execute();
        
        return GetLastInsertId();
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return 0;
    }
}

bool DatabaseManager::UpdateProject(int projectId, const String& name, const String& description) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    String sql = "UPDATE projects SET name = ?, description = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?";
    SQLite3Statement stmt;
    
    try {
        stmt.Prepare(sql);
        stmt.Bind(1, name);
        stmt.Bind(2, description);
        stmt.Bind(3, projectId);
        stmt.Execute();
        
        return stmt.GetAffectedRows() > 0;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return false;
    }
}

bool DatabaseManager::DeleteProject(int projectId) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    String sql = "DELETE FROM projects WHERE id = ?";
    SQLite3Statement stmt;
    
    try {
        stmt.Prepare(sql);
        stmt.Bind(1, projectId);
        stmt.Execute();
        
        return stmt.GetAffectedRows() > 0;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return false;
    }
}

bool DatabaseManager::GetProject(int projectId, Project& project) const {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    String sql = "SELECT id, name, description, created_at, updated_at FROM projects WHERE id = ?";
    SQLite3Statement stmt;
    
    try {
        if (!ExecuteQuery(sql, stmt)) {
            return false;
        }
        
        stmt.Bind(1, projectId);
        if (!stmt.Fetch()) {
            return false;
        }
        
        project.id = stmt[0];
        project.name = stmt[1];
        project.description = stmt[2];
        project.createdAt = Time::FromString(stmt[3]);
        project.updatedAt = Time::FromString(stmt[4]);
        
        return true;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return false;
    }
}

Vector<int> DatabaseManager::GetAllProjectIds() const {
    std::lock_guard<std::mutex> lock(dbMutex);
    Vector<int> projectIds;
    
    String sql = "SELECT id FROM projects ORDER BY name";
    SQLite3Statement stmt;
    
    try {
        if (!ExecuteQuery(sql, stmt)) {
            return projectIds;
        }
        
        while (stmt.Fetch()) {
            projectIds.Add(stmt[0]);
        }
        
        return projectIds;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return projectIds;
    }
}

Vector<String> DatabaseManager::GetAllProjectNames() const {
    std::lock_guard<std::mutex> lock(dbMutex);
    Vector<String> projectNames;
    
    String sql = "SELECT name FROM projects ORDER BY name";
    SQLite3Statement stmt;
    
    try {
        if (!ExecuteQuery(sql, stmt)) {
            return projectNames;
        }
        
        while (stmt.Fetch()) {
            projectNames.Add(stmt[0]);
        }
        
        return projectNames;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return projectNames;
    }
}

int DatabaseManager::CreateTask(int projectId, const String& name, double workload, int duration,
                               const Time& startDate, const Time& endDate, double progress, int priority,
                               const Color& color, double xPosition, double yPosition) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    String sql = "INSERT INTO tasks (project_id, name, workload, duration, start_date, end_date, "
                 "progress, priority, color, x_position, y_position) "
                 "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    SQLite3Statement stmt;
    
    try {
        stmt.Prepare(sql);
        stmt.Bind(1, projectId);
        stmt.Bind(2, name);
        stmt.Bind(3, workload);
        stmt.Bind(4, duration);
        stmt.Bind(5, startDate.ToString());
        stmt.Bind(6, endDate.ToString());
        stmt.Bind(7, progress);
        stmt.Bind(8, priority);
        stmt.Bind(9, color.ToString());
        stmt.Bind(10, xPosition);
        stmt.Bind(11, yPosition);
        stmt.Execute();
        
        return GetLastInsertId();
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return 0;
    }
}

bool DatabaseManager::UpdateTask(const Task& task) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    String sql = "UPDATE tasks SET project_id = ?, name = ?, workload = ?, duration = ?, "
                 "start_date = ?, end_date = ?, progress = ?, priority = ?, color = ?, "
                 "x_position = ?, y_position = ? WHERE id = ?";
    SQLite3Statement stmt;
    
    try {
        stmt.Prepare(sql);
        stmt.Bind(1, task.projectId);
        stmt.Bind(2, task.name);
        stmt.Bind(3, task.workload);
        stmt.Bind(4, task.duration);
        stmt.Bind(5, task.startDate.ToString());
        stmt.Bind(6, task.endDate.ToString());
        stmt.Bind(7, task.progress);
        stmt.Bind(8, task.priority);
        stmt.Bind(9, task.color.ToString());
        stmt.Bind(10, task.xPosition);
        stmt.Bind(11, task.yPosition);
        stmt.Bind(12, task.id);
        stmt.Execute();
        
        // Update dependencies
        ClearTaskDependencies(task.id);
        for (int depId : task.dependencies) {
            AddTaskDependency(task.id, depId);
        }
        
        // Update resource assignments
        ClearTaskResourceAssignments(task.id);
        for (const auto& assignment : task.resourceAssignments) {
            AddResourceAssignment(task.id, assignment.first, assignment.second);
        }
        
        return stmt.GetAffectedRows() > 0;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return false;
    }
}

bool DatabaseManager::DeleteTask(int taskId) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    String sql = "DELETE FROM tasks WHERE id = ?";
    SQLite3Statement stmt;
    
    try {
        stmt.Prepare(sql);
        stmt.Bind(1, taskId);
        stmt.Execute();
        
        return stmt.GetAffectedRows() > 0;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return false;
    }
}

bool DatabaseManager::GetTask(int taskId, Task& task) const {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    String sql = "SELECT id, project_id, name, workload, duration, start_date, end_date, "
                 "progress, priority, color, x_position, y_position "
                 "FROM tasks WHERE id = ?";
    SQLite3Statement stmt;
    
    try {
        if (!ExecuteQuery(sql, stmt)) {
            return false;
        }
        
        stmt.Bind(1, taskId);
        if (!stmt.Fetch()) {
            return false;
        }
        
        task.id = stmt[0];
        task.projectId = stmt[1];
        task.name = stmt[2];
        task.workload = stmt[3];
        task.duration = stmt[4];
        task.startDate = Time::FromString(stmt[5]);
        task.endDate = Time::FromString(stmt[6]);
        task.progress = stmt[7];
        task.priority = stmt[8];
        task.color = Color::FromString(stmt[9]);
        task.xPosition = stmt[10];
        task.yPosition = stmt[11];
        
        // Load dependencies
        task.dependencies = GetTaskDependencies(taskId);
        
        // Load resource assignments
        task.resourceAssignments = GetTaskResourceAssignments(taskId);
        
        return true;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return false;
    }
}

Vector<int> DatabaseManager::GetAllTaskIds(int projectId) const {
    std::lock_guard<std::mutex> lock(dbMutex);
    Vector<int> taskIds;
    
    String sql = "SELECT id FROM tasks WHERE project_id = ? ORDER BY name";
    SQLite3Statement stmt;
    
    try {
        if (!ExecuteQuery(sql, stmt)) {
            return taskIds;
        }
        
        stmt.Bind(1, projectId);
        while (stmt.Fetch()) {
            taskIds.Add(stmt[0]);
        }
        
        return taskIds;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return taskIds;
    }
}

Vector<Task> DatabaseManager::GetAllTasks(int projectId) const {
    std::lock_guard<std::mutex> lock(dbMutex);
    Vector<Task> tasks;
    
    Vector<int> taskIds = GetAllTaskIds(projectId);
    for (int taskId : taskIds) {
        Task task;
        if (GetTask(taskId, task)) {
            tasks.Add(task);
        }
    }
    
    return tasks;
}

int DatabaseManager::CreateResource(int projectId, const String& name, double maxUtilizationRate, const String& description) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    String sql = "INSERT INTO resources (project_id, name, max_utilization_rate, description) "
                 "VALUES (?, ?, ?, ?)";
    SQLite3Statement stmt;
    
    try {
        stmt.Prepare(sql);
        stmt.Bind(1, projectId);
        stmt.Bind(2, name);
        stmt.Bind(3, maxUtilizationRate);
        stmt.Bind(4, description);
        stmt.Execute();
        
        return GetLastInsertId();
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return 0;
    }
}

bool DatabaseManager::UpdateResource(const Resource& resource) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    String sql = "UPDATE resources SET project_id = ?, name = ?, max_utilization_rate = ?, "
                 "description = ? WHERE id = ?";
    SQLite3Statement stmt;
    
    try {
        stmt.Prepare(sql);
        stmt.Bind(1, resource.projectId);
        stmt.Bind(2, resource.name);
        stmt.Bind(3, resource.maxUtilizationRate);
        stmt.Bind(4, resource.description);
        stmt.Bind(5, resource.id);
        stmt.Execute();
        
        return stmt.GetAffectedRows() > 0;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return false;
    }
}

bool DatabaseManager::DeleteResource(int resourceId) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    String sql = "DELETE FROM resources WHERE id = ?";
    SQLite3Statement stmt;
    
    try {
        stmt.Prepare(sql);
        stmt.Bind(1, resourceId);
        stmt.Execute();
        
        return stmt.GetAffectedRows() > 0;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return false;
    }
}

bool DatabaseManager::GetResource(int resourceId, Resource& resource) const {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    String sql = "SELECT id, project_id, name, max_utilization_rate, description "
                 "FROM resources WHERE id = ?";
    SQLite3Statement stmt;
    
    try {
        if (!ExecuteQuery(sql, stmt)) {
            return false;
        }
        
        stmt.Bind(1, resourceId);
        if (!stmt.Fetch()) {
            return false;
        }
        
        resource.id = stmt[0];
        resource.projectId = stmt[1];
        resource.name = stmt[2];
        resource.maxUtilizationRate = stmt[3];
        resource.description = stmt[4];
        
        return true;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return false;
    }
}

Vector<int> DatabaseManager::GetAllResourceIds(int projectId) const {
    std::lock_guard<std::mutex> lock(dbMutex);
    Vector<int> resourceIds;
    
    String sql = "SELECT id FROM resources WHERE project_id = ? ORDER BY name";
    SQLite3Statement stmt;
    
    try {
        if (!ExecuteQuery(sql, stmt)) {
            return resourceIds;
        }
        
        stmt.Bind(1, projectId);
        while (stmt.Fetch()) {
            resourceIds.Add(stmt[0]);
        }
        
        return resourceIds;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return resourceIds;
    }
}

Vector<Resource> DatabaseManager::GetAllResources(int projectId) const {
    std::lock_guard<std::mutex> lock(dbMutex);
    Vector<Resource> resources;
    
    Vector<int> resourceIds = GetAllResourceIds(projectId);
    for (int resourceId : resourceIds) {
        Resource resource;
        if (GetResource(resourceId, resource)) {
            resources.Add(resource);
        }
    }
    
    return resources;
}

bool DatabaseManager::AddTaskDependency(int taskId, int dependsOnTaskId) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    if (taskId == dependsOnTaskId) {
        lastError = "Cannot add dependency: task cannot depend on itself";
        lastErrorCode = -1;
        return false;
    }
    
    String sql = "INSERT OR IGNORE INTO task_dependencies (task_id, depends_on_task_id) VALUES (?, ?)";
    SQLite3Statement stmt;
    
    try {
        stmt.Prepare(sql);
        stmt.Bind(1, taskId);
        stmt.Bind(2, dependsOnTaskId);
        stmt.Execute();
        
        return stmt.GetAffectedRows() > 0;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return false;
    }
}

bool DatabaseManager::RemoveTaskDependency(int taskId, int dependsOnTaskId) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    String sql = "DELETE FROM task_dependencies WHERE task_id = ? AND depends_on_task_id = ?";
    SQLite3Statement stmt;
    
    try {
        stmt.Prepare(sql);
        stmt.Bind(1, taskId);
        stmt.Bind(2, dependsOnTaskId);
        stmt.Execute();
        
        return stmt.GetAffectedRows() > 0;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return false;
    }
}

Vector<int> DatabaseManager::GetTaskDependencies(int taskId) const {
    std::lock_guard<std::mutex> lock(dbMutex);
    Vector<int> dependencies;
    
    String sql = "SELECT depends_on_task_id FROM task_dependencies WHERE task_id = ?";
    SQLite3Statement stmt;
    
    try {
        if (!ExecuteQuery(sql, stmt)) {
            return dependencies;
        }
        
        stmt.Bind(1, taskId);
        while (stmt.Fetch()) {
            dependencies.Add(stmt[0]);
        }
        
        return dependencies;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return dependencies;
    }
}

Vector<int> DatabaseManager::GetDependentTasks(int taskId) const {
    std::lock_guard<std::mutex> lock(dbMutex);
    Vector<int> dependentTasks;
    
    String sql = "SELECT task_id FROM task_dependencies WHERE depends_on_task_id = ?";
    SQLite3Statement stmt;
    
    try {
        if (!ExecuteQuery(sql, stmt)) {
            return dependentTasks;
        }
        
        stmt.Bind(1, taskId);
        while (stmt.Fetch()) {
            dependentTasks.Add(stmt[0]);
        }
        
        return dependentTasks;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return dependentTasks;
    }
}

bool DatabaseManager::ClearTaskDependencies(int taskId) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    String sql = "DELETE FROM task_dependencies WHERE task_id = ?";
    SQLite3Statement stmt;
    
    try {
        stmt.Prepare(sql);
        stmt.Bind(1, taskId);
        stmt.Execute();
        
        return true;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return false;
    }
}

bool DatabaseManager::AddResourceAssignment(int taskId, int resourceId, double allocationRate) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    String sql = "INSERT OR REPLACE INTO task_resources (task_id, resource_id, allocation_rate) "
                 "VALUES (?, ?, ?)";
    SQLite3Statement stmt;
    
    try {
        stmt.Prepare(sql);
        stmt.Bind(1, taskId);
        stmt.Bind(2, resourceId);
        stmt.Bind(3, allocationRate);
        stmt.Execute();
        
        return true;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return false;
    }
}

bool DatabaseManager::UpdateResourceAssignment(int taskId, int resourceId, double allocationRate) {
    return AddResourceAssignment(taskId, resourceId, allocationRate);
}

bool DatabaseManager::RemoveResourceAssignment(int taskId, int resourceId) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    String sql = "DELETE FROM task_resources WHERE task_id = ? AND resource_id = ?";
    SQLite3Statement stmt;
    
    try {
        stmt.Prepare(sql);
        stmt.Bind(1, taskId);
        stmt.Bind(2, resourceId);
        stmt.Execute();
        
        return stmt.GetAffectedRows() > 0;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return false;
    }
}

Vector<std::pair<int, double>> DatabaseManager::GetTaskResourceAssignments(int taskId) const {
    std::lock_guard<std::mutex> lock(dbMutex);
    Vector<std::pair<int, double>> assignments;
    
    String sql = "SELECT resource_id, allocation_rate FROM task_resources WHERE task_id = ?";
    SQLite3Statement stmt;
    
    try {
        if (!ExecuteQuery(sql, stmt)) {
            return assignments;
        }
        
        stmt.Bind(1, taskId);
        while (stmt.Fetch()) {
            assignments.Add(std::make_pair(stmt[0], stmt[1]));
        }
        
        return assignments;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return assignments;
    }
}

bool DatabaseManager::ClearTaskResourceAssignments(int taskId) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    String sql = "DELETE FROM task_resources WHERE task_id = ?";
    SQLite3Statement stmt;
    
    try {
        stmt.Prepare(sql);
        stmt.Bind(1, taskId);
        stmt.Execute();
        
        return true;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return false;
    }
}

bool DatabaseManager::UpdateResourceUtilization(int resourceId, const Time& date, double utilizationRate) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    String dateStr = date.ToString();
    String sql = "INSERT OR REPLACE INTO resource_utilization (resource_id, date, utilization_rate) "
                 "VALUES (?, ?, ?)";
    SQLite3Statement stmt;
    
    try {
        stmt.Prepare(sql);
        stmt.Bind(1, resourceId);
        stmt.Bind(2, dateStr);
        stmt.Bind(3, utilizationRate);
        stmt.Execute();
        
        return true;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return false;
    }
}

double DatabaseManager::GetResourceUtilization(int resourceId, const Time& date) const {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    String dateStr = date.ToString();
    String sql = "SELECT utilization_rate FROM resource_utilization WHERE resource_id = ? AND date = ?";
    SQLite3Statement stmt;
    
    try {
        if (!ExecuteQuery(sql, stmt)) {
            return 0.0;
        }
        
        stmt.Bind(1, resourceId);
        stmt.Bind(2, dateStr);
        if (stmt.Fetch()) {
            return stmt[0];
        }
        
        return 0.0;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return 0.0;
    }
}

Vector<std::pair<Time, double>> DatabaseManager::GetResourceUtilizationProfile(int resourceId, 
                                                                                const Time& startDate, 
                                                                                const Time& endDate) const {
    std::lock_guard<std::mutex> lock(dbMutex);
    Vector<std::pair<Time, double>> profile;
    
    String sql = "SELECT date, utilization_rate FROM resource_utilization "
                 "WHERE resource_id = ? AND date >= ? AND date <= ? ORDER BY date";
    SQLite3Statement stmt;
    
    try {
        if (!ExecuteQuery(sql, stmt)) {
            return profile;
        }
        
        stmt.Bind(1, resourceId);
        stmt.Bind(2, startDate.ToString());
        stmt.Bind(3, endDate.ToString());
        
        while (stmt.Fetch()) {
            Time date = Time::FromString(stmt[0]);
            double rate = stmt[1];
            profile.Add(std::make_pair(date, rate));
        }
        
        return profile;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        return profile;
    }
}

bool DatabaseManager::SaveProject(const Project& project) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    if (!BeginTransaction()) {
        return false;
    }
    
    try {
        // Update or insert project
        if (project.id <= 0) {
            project.id = CreateProject(project.name, project.description);
        } else {
            UpdateProject(project.id, project.name, project.description);
        }
        
        if (project.id <= 0) {
            RollbackTransaction();
            return false;
        }
        
        // Save resources
        for (const Resource& resource : project.resources) {
            Resource existingResource;
            if (GetResource(resource.id, existingResource)) {
                UpdateResource(resource);
            } else {
                CreateResource(project.id, resource.name, resource.maxUtilizationRate, resource.description);
            }
        }
        
        // Save tasks
        for (const Task& task : project.tasks) {
            Task existingTask;
            if (GetTask(task.id, existingTask)) {
                UpdateTask(task);
            } else {
                CreateTask(project.id, task.name, task.workload, task.duration,
                          task.startDate, task.endDate, task.progress, task.priority,
                          task.color, task.xPosition, task.yPosition);
            }
        }
        
        CommitTransaction();
        return true;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        RollbackTransaction();
        return false;
    }
}

bool DatabaseManager::LoadProject(int projectId, Project& project) const {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    if (!GetProject(projectId, project)) {
        return false;
    }
    
    // Load resources
    project.resources = GetAllResources(projectId);
    
    // Load tasks
    project.tasks = GetAllTasks(projectId);
    
    return true;
}

bool DatabaseManager::DeleteProjectData(int projectId) {
    std::lock_guard<std::mutex> lock(dbMutex);
    
    if (!BeginTransaction()) {
        return false;
    }
    
    try {
        // Delete all task dependencies for project tasks
        String sql = "DELETE FROM task_dependencies WHERE task_id IN "
                     "(SELECT id FROM tasks WHERE project_id = ?)";
        SQLite3Statement stmt;
        stmt.Prepare(sql);
        stmt.Bind(1, projectId);
        stmt.Execute();
        
        // Delete all resource assignments for project tasks
        sql = "DELETE FROM task_resources WHERE task_id IN "
              "(SELECT id FROM tasks WHERE project_id = ?)";
        stmt.Prepare(sql);
        stmt.Bind(1, projectId);
        stmt.Execute();
        
        // Delete all resource utilization for project resources
        sql = "DELETE FROM resource_utilization WHERE resource_id IN "
              "(SELECT id FROM resources WHERE project_id = ?)";
        stmt.Prepare(sql);
        stmt.Bind(1, projectId);
        stmt.Execute();
        
        // Delete all tasks
        sql = "DELETE FROM tasks WHERE project_id = ?";
        stmt.Prepare(sql);
        stmt.Bind(1, projectId);
        stmt.Execute();
        
        // Delete all resources
        sql = "DELETE FROM resources WHERE project_id = ?";
        stmt.Prepare(sql);
        stmt.Bind(1, projectId);
        stmt.Execute();
        
        // Finally, delete the project
        if (!DeleteProject(projectId)) {
            RollbackTransaction();
            return false;
        }
        
        CommitTransaction();
        return true;
    } catch (const SQLite3Exception& e) {
        lastError = e.GetMessage();
        lastErrorCode = e.GetCode();
        RollbackTransaction();
        return false;
    }
}

bool DatabaseManager::BeginTransaction() {
    std::lock_guard<std::mutex> lock(dbMutex);
    return ExecuteNonQuery("BEGIN TRANSACTION");
}

bool DatabaseManager::CommitTransaction() {
    std::lock_guard<std::mutex> lock(dbMutex);
    return ExecuteNonQuery("COMMIT");
}

bool DatabaseManager::RollbackTransaction() {
    std::lock_guard<std::mutex> lock(dbMutex);
    return ExecuteNonQuery("ROLLBACK");
}

String DatabaseManager::GetLastError() const {
    std::lock_guard<std::mutex> lock(dbMutex);
    return lastError;
}

int DatabaseManager::GetLastErrorCode() const {
    std::lock_guard<std::mutex> lock(dbMutex);
    return lastErrorCode;
}

std::mutex& DatabaseManager::GetMutex() {
    return dbMutex;
}

// Singleton implementation
DatabaseManager DatabaseManagerSingleton::instance;
String DatabaseManagerSingleton::dbPath;
bool DatabaseManagerSingleton::initialized = false;

DatabaseManager& DatabaseManagerSingleton::GetInstance() {
    if (!initialized) {
        if (!instance.OpenDatabase(dbPath)) {
            // Try to create database if it doesn't exist
            instance.CreateDatabase(dbPath);
        }
        instance.InitializeDatabase();
        initialized = true;
    }
    return instance;
}

void DatabaseManagerSingleton::SetDatabasePath(const String& path) {
    dbPath = path;
}

String DatabaseManagerSingleton::GetDatabasePath() {
    return dbPath;
}
