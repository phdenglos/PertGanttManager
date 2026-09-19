#ifndef PERT_DIAGRAM_H
#define PERT_DIAGRAM_H

#include <CtrlLib/CtrlLib.h>
#include <vector>
#include <memory>

using namespace Upp;

class Task;
class Resource;
class Project;

// Forward declarations
class PertDiagram;

// Node representation for PERT diagram
class PertNode {
public:
    int taskId;
    Point position;
    Size size;
    Color color;
    String label;
    int priority;
    double progress;
    
    PertNode();
    PertNode(int taskId, const Point& position, const Size& size, const Color& color,
             const String& label, int priority, double progress);
    
    void Draw(Draw& draw) const;
    bool Contains(const Point& point) const;
};

// Edge representation for PERT diagram (dependencies)
class PertEdge {
public:
    int fromTaskId;
    int toTaskId;
    Point fromPoint;
    Point toPoint;
    Color color;
    
    PertEdge();
    PertEdge(int fromTaskId, int toTaskId, const Point& fromPoint, const Point& toPoint, const Color& color);
    
    void Draw(Draw& draw) const;
};

// Main PERT diagram widget
class PertDiagram : public Ctrl {
public:
    typedef PertDiagram CLASSNAME;
    
    PertDiagram();
    ~PertDiagram();
    
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
    
    // Layout and zoom
    void SetZoom(double zoom);
    double GetZoom() const;
    void SetOffset(const Point& offset);
    Point GetOffset() const;
    void CenterView();
    void FitToScreen();
    
    // Selection
    void SelectNode(int taskId);
    void SelectNodes(const Vector<int>& taskIds);
    void DeselectAll();
    const Vector<int>& GetSelectedNodes() const;
    
    // Task operations
    void AddTask(const Task& task);
    void UpdateTask(const Task& task);
    void RemoveTask(int taskId);
    void AddDependency(int fromTaskId, int toTaskId);
    void RemoveDependency(int fromTaskId, int toTaskId);
    
    // Visual settings
    void SetNodeSize(const Size& size);
    void SetEdgeColor(const Color& color);
    void SetSelectionColor(const Color& color);
    void SetBackgroundColor(const Color& color);
    
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
    
private:
    // Data
    Project project;
    Vector<PertNode> nodes;
    Vector<PertEdge> edges;
    
    // Visual state
    double zoom;
    Point offset;
    Size nodeSize;
    Color edgeColor;
    Color selectionColor;
    Color backgroundColor;
    
    // Interaction state
    Vector<int> selectedNodes;
    Point dragStartPoint;
    int dragTaskId;
    bool isDragging;
    bool isConnecting;
    int connectFromTaskId;
    Point connectStartPoint;
    
    // Helper methods
    void BuildNodes();
    void BuildEdges();
    void LayoutNodes();
    Point CalculateNodePosition(int taskId) const;
    PertNode* FindNode(int taskId);
    const PertNode* FindNode(int taskId) const;
    PertNode* FindNodeAt(const Point& point);
    void UpdateNode(int taskId, const PertNode& node);
    
    // Drawing helpers
    void DrawNodes(Draw& draw);
    void DrawEdges(Draw& draw);
    void DrawSelection(Draw& draw);
    void DrawConnectionPreview(Draw& draw);
    
    // Hit testing
    bool IsNodeSelected(int taskId) const;
    
    // Auto-layout
    void ApplyAutoLayout();
    void CalculateDependenciesDepth();
};

#endif // PERT_DIAGRAM_H
