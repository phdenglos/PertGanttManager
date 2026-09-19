-- SQLite Database Schema for PertGanttManager

-- Projects table
CREATE TABLE IF NOT EXISTS projects (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE,
    description TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Resources table
CREATE TABLE IF NOT EXISTS resources (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    project_id INTEGER NOT NULL,
    name TEXT NOT NULL,
    max_utilization_rate REAL NOT NULL DEFAULT 100.0, -- Percentage
    description TEXT,
    FOREIGN KEY (project_id) REFERENCES projects(id) ON DELETE CASCADE,
    UNIQUE(project_id, name)
);

-- Tasks table
CREATE TABLE IF NOT EXISTS tasks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    project_id INTEGER NOT NULL,
    name TEXT NOT NULL,
    duration INTEGER NOT NULL DEFAULT 1, -- in days
    workload REAL NOT NULL DEFAULT 1.0, -- in person-days
    start_date TEXT, -- ISO format YYYY-MM-DD
    end_date TEXT, -- ISO format YYYY-MM-DD
    progress REAL NOT NULL DEFAULT 0.0, -- Percentage (0-100)
    priority INTEGER NOT NULL DEFAULT 3, -- 1-5, 1 is highest
    color TEXT NOT NULL DEFAULT '#FFFFFFFF', -- RGBA color
    x_position REAL NOT NULL DEFAULT 0.0, -- X position for diagram
    y_position REAL NOT NULL DEFAULT 0.0, -- Y position for diagram
    FOREIGN KEY (project_id) REFERENCES projects(id) ON DELETE CASCADE,
    UNIQUE(project_id, name)
);

-- Task dependencies table (PERT diagram dependencies)
CREATE TABLE IF NOT EXISTS task_dependencies (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    task_id INTEGER NOT NULL,
    depends_on_task_id INTEGER NOT NULL,
    FOREIGN KEY (task_id) REFERENCES tasks(id) ON DELETE CASCADE,
    FOREIGN KEY (depends_on_task_id) REFERENCES tasks(id) ON DELETE CASCADE,
    UNIQUE(task_id, depends_on_task_id),
    CHECK(task_id != depends_on_task_id)
);

-- Resource assignments to tasks
CREATE TABLE IF NOT EXISTS task_resources (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    task_id INTEGER NOT NULL,
    resource_id INTEGER NOT NULL,
    allocation_rate REAL NOT NULL DEFAULT 100.0, -- Percentage
    FOREIGN KEY (task_id) REFERENCES tasks(id) ON DELETE CASCADE,
    FOREIGN KEY (resource_id) REFERENCES resources(id) ON DELETE CASCADE,
    UNIQUE(task_id, resource_id)
);

-- Resource utilization profiles (for tracking usage over time)
CREATE TABLE IF NOT EXISTS resource_utilization (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    resource_id INTEGER NOT NULL,
    date TEXT NOT NULL, -- ISO format YYYY-MM-DD
    utilization_rate REAL NOT NULL DEFAULT 0.0, -- Percentage
    FOREIGN KEY (resource_id) REFERENCES resources(id) ON DELETE CASCADE,
    UNIQUE(resource_id, date)
);

-- Create indexes for performance
CREATE INDEX IF NOT EXISTS idx_projects_name ON projects(name);
CREATE INDEX IF NOT EXISTS idx_tasks_project ON tasks(project_id);
CREATE INDEX IF NOT EXISTS idx_tasks_priority ON tasks(priority);
CREATE INDEX IF NOT EXISTS idx_task_dependencies_task ON task_dependencies(task_id);
CREATE INDEX IF NOT EXISTS idx_task_dependencies_depends ON task_dependencies(depends_on_task_id);
CREATE INDEX IF NOT EXISTS idx_resources_project ON resources(project_id);
CREATE INDEX IF NOT EXISTS idx_task_resources_task ON task_resources(task_id);
CREATE INDEX IF NOT EXISTS idx_task_resources_resource ON task_resources(resource_id);

-- Create triggers for updated_at timestamp
CREATE TRIGGER IF NOT EXISTS update_project_timestamp 
AFTER UPDATE ON projects 
FOR EACH ROW 
BEGIN 
    UPDATE projects SET updated_at = CURRENT_TIMESTAMP WHERE id = OLD.id;
END;
