#ifndef GANTT_DIAGRAM_H
#define GANTT_DIAGRAM_H

#include <CtrlLib/CtrlLib.h>
#include <vector>
#include <memory>

using namespace Upp;

class Task;
class Resource;
class Project;

// Forward declarations
class GanttDiagram;

// Bar representation for Gantt diagram
class GanttBar {
public:
    int taskId;
    int row;
    String label;
    Time startDate;
    Time endDate;
    Color color;
    double progress;
    int priority;
    
    GanttBar();
    GanttBar(int taskId, int row, const String& label, const Time& startDate, const Time& endDate,
            const Color& color, double progress, int priority);
    
    void Draw(Draw& draw, int x, int y, int width, int height, const Time& timelineStart) const;
    bool Contains(const Point& point, int x, int y, int width, int height, const Time& timelineStart) const;
    
    // Calculate bar position and size
    int GetXPosition(int chartWidth, const Time& timelineStart, const Time& timelineEnd) const;
    int GetWidth(int chartWidth, const Time& timelineStart, const Time& timelineEnd) const;
};

// Main Gantt diagram widget
class GanttDiagram : public Ctrl {
public:
    typedef GanttDiagram CLASSNAME;
    
    GanttDiagram();
    ~GanttDiagram();
    
    // Data management
    void SetProject(const Project& project);
    const Project& GetProject() const;
    void UpdateTaskPositions();
    
    // Interaction
    virtual void Paint(Draw& draw);
    virtual void LeftDown(Point point, dword keyflags);
    virtual void LeftUp(Point point, dword keyflags);
    virtual void MouseMove(Point point, dword keyflags);
    virtual void RightDown(Point point, dword keyflags);
    virtual void RightUp(Point point, dword keyflags);
    virtual void MouseWheel(Point point, int zdelta, dword keyflags);
    virtual void KeyDown(dword key, int count);
    
    // Timeline settings
    void SetTimelineStart(const Time& start);
    void SetTimelineEnd(const Time& end);
    Time GetTimelineStart() const;
    Time GetTimelineEnd() const;
    void FitTimelineToProject();
    
    // Layout and zoom
    void SetZoom(double zoom);
    double GetZoom() const;
    void SetOffset(const Point& offset);
    Point GetOffset() const;
    void CenterView();
    void FitToScreen();
    
    // Selection
    void SelectBar(int taskId);
    void SelectBars(const Vector<int>& taskIds);
    void DeselectAll();
    const Vector<int>& GetSelectedBars() const;
    
    // Task operations
    void AddTask(const Task& task);
    void UpdateTask(const Task& task);
    void RemoveTask(int taskId);
    void AddDependency(int fromTaskId, int toTaskId);
    void RemoveDependency(int fromTaskId, int toTaskId);
    
    // Visual settings
    void SetBarHeight(int height);
    void SetRowHeight(int height);
    void SetHeaderHeight(int height);
    void SetSelectionColor(const Color& color);
    void SetBackgroundColor(const Color& color);
    void SetGridColor(const Color& color);
    void SetTextColor(const Color& color);
    
    // Event callbacks
    Callback1<int> WhenTaskSelected;
    Callback2<int, int> WhenDependencyAdded;
    Callback2<int, int> WhenDependencyRemoved;
    Callback1<int> WhenTaskMoved;
    Callback WhenDiagramChanged;
    
    // Utility methods
    Point GetTaskPosition(int taskId) const;
    void SetTaskPosition(int taskId, const Point& position);
    bool HasDependency(int fromTaskId, int toTaskId) const;
    
    // Export
    Image GetAsImage() const;
    
    // Resource profile
    void ShowResourceProfile(bool show);
    bool IsResourceProfileShown() const;
    
private:
    // Data
    Project project;
    Vector<GanttBar> bars;
    
    // Timeline
    Time timelineStart;
    Time timelineEnd;
    
    // Visual state
    double zoom;
    Point offset;
    int barHeight;
    int rowHeight;
    int headerHeight;
    Color selectionColor;
    Color backgroundColor;
    Color gridColor;
    Color textColor;
    
    // Interaction state
    Vector<int> selectedBars;
    Point dragStartPoint;
    int dragTaskId;
    bool isDragging;
    bool isConnecting;
    int connectFromTaskId;
    Point connectStartPoint;
    
    // Resource profile
    bool showResourceProfile;
    
    // Helper methods
    void BuildBars();
    void BuildDependencies();
    void LayoutBars();
    
    GanttBar* FindBar(int taskId);
    const GanttBar* FindBar(int taskId) const;
    GanttBar* FindBarAt(const Point& point);
    
    // Drawing helpers
    void DrawHeader(Draw& draw);
    void DrawBars(Draw& draw);
    void DrawDependencies(Draw& draw);
    void DrawSelection(Draw& draw);
    void DrawConnectionPreview(Draw& draw);
    void DrawResourceProfile(Draw& draw);
    
    // Timeline helpers
    void DrawTimeline(Draw& draw, int y, int width);
    String FormatDate(const Time& date) const;
    int DateToX(const Time& date) const;
    Time XToDate(int x) const;
    
    // Hit testing
    bool IsBarSelected(int taskId) const;
    
    // Auto-layout
    void ApplyAutoLayout();
    
    // Helper method to transform screen coordinates to diagram coordinates
    Point ApplyInverseTransform(const Point& screenPoint) const;
};

#endif // GANTT_DIAGRAM_H
