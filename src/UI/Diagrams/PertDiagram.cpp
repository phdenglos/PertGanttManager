#include "PertDiagram.h"
#include "../../Model/Project.h"
#include "../../Model/Task.h"

using namespace Upp;

// Constants
const int DEFAULT_NODE_WIDTH = 120;
const int DEFAULT_NODE_HEIGHT = 60;
const int NODE_MARGIN = 20;
const int EDGE_ARROW_SIZE = 8;
const int SELECTION_MARGIN = 5;

// PertNode implementation
PertNode::PertNode() 
    : taskId(0), position(0, 0), size(DEFAULT_NODE_WIDTH, DEFAULT_NODE_HEIGHT), 
      color(White()), label(""), priority(3), progress(0.0) {}

PertNode::PertNode(int taskId, const Point& position, const Size& size, const Color& color,
                   const String& label, int priority, double progress)
    : taskId(taskId), position(position), size(size), color(color), 
      label(label), priority(priority), progress(progress) {}

void PertNode::Draw(Draw& draw) const {
    // Draw shadow
    draw.DrawRect(position.x + 2, position.y + 2, size.cx, size.cy, Gray());
    
    // Draw node background
    draw.DrawRect(position.x, position.y, size.cx, size.cy, color);
    
    // Draw border
    draw.DrawRect(position.x, position.y, size.cx, size.cy, Black());
    
    // Draw label
    Font font = StdFont();
    font.Bold();
    draw.DrawText(position.x + 5, position.y + 5, label, font, Black());
    
    // Draw progress bar
    if (progress > 0) {
        int progressWidth = static_cast<int>((size.cx - 10) * (progress / 100.0));
        draw.DrawRect(position.x + 5, position.y + size.cy - 20, 
                      progressWidth, 15, LtGreen());
        draw.DrawRect(position.x + 5, position.y + size.cy - 20, 
                      size.cx - 10, 15, Black());
        
        String progressText = Format("%d%%", static_cast<int>(progress));
        draw.DrawText(position.x + 5, position.y + size.cy - 20, progressText, 
                      StdFont(), Black());
    }
    
    // Draw priority indicator
    String priorityText = Format("P%d", priority);
    draw.DrawText(position.x + size.cx - 25, position.y + 5, priorityText, 
                  StdFont(), Black());
}

bool PertNode::Contains(const Point& point) const {
    return point.x >= position.x && point.x <= position.x + size.cx &&
           point.y >= position.y && point.y <= position.y + size.cy;
}

// PertEdge implementation
PertEdge::PertEdge() : fromTaskId(0), toTaskId(0), color(Black()) {}

PertEdge::PertEdge(int fromTaskId, int toTaskId, const Point& fromPoint, const Point& toPoint, const Color& color)
    : fromTaskId(fromTaskId), toTaskId(toTaskId), fromPoint(fromPoint), toPoint(toPoint), color(color) {}

void PertEdge::Draw(Draw& draw) const {
    // Draw arrow
    draw.DrawArrow(fromPoint.x, fromPoint.y, toPoint.x, toPoint.y, 2, color);
}

// PertDiagram implementation
PertDiagram::PertDiagram() 
    : zoom(1.0), offset(0, 0), nodeSize(DEFAULT_NODE_WIDTH, DEFAULT_NODE_HEIGHT),
      edgeColor(Black()), selectionColor(LtBlue()), backgroundColor(White()),
      isDragging(false), isConnecting(false), dragTaskId(0), connectFromTaskId(0) {
    SetFrame(InsetFrame());
    SetMinSize(Size(400, 300));
}

PertDiagram::~PertDiagram() {}

void PertDiagram::SetProject(const Project& project) {
    this->project = project;
    BuildNodes();
    BuildEdges();
    LayoutNodes();
    Refresh();
}

const Project& PertDiagram::GetProject() const {
    return project;
}

void PertDiagram::UpdateTaskPositions() {
    // Update positions from project tasks
    for (const Task& task : project.tasks) {
        PertNode* node = FindNode(task.id);
        if (node) {
            node->position = Point(static_cast<int>(task.xPosition), 
                                   static_cast<int>(task.yPosition));
        }
    }
    Refresh();
}

void PertDiagram::Paint(Draw& draw) {
    Size sz = GetSize();
    
    // Draw background
    draw.DrawRect(0, 0, sz.cx, sz.cy, backgroundColor);
    
    // Apply zoom and offset
    draw.Offset(offset);
    draw.Scale(zoom);
    
    // Draw edges first (so they appear behind nodes)
    DrawEdges(draw);
    
    // Draw nodes
    DrawNodes(draw);
    
    // Draw selection
    DrawSelection(draw);
    
    // Draw connection preview if connecting
    if (isConnecting) {
        DrawConnectionPreview(draw);
    }
}

void PertDiagram::LeftDown(Point point, dword keyflags) {
    Point adjustedPoint = ApplyInverseTransform(point);
    
    // Check if we clicked on a node
    PertNode* node = FindNodeAt(adjustedPoint);
    if (node) {
        if (!(keyflags & K_CTRL)) {
            DeselectAll();
        }
        
        if (!IsNodeSelected(node->taskId)) {
            selectedNodes.Add(node->taskId);
            WhenTaskSelected(node->taskId);
        }
        
        // Start dragging
        isDragging = true;
        dragTaskId = node->taskId;
        dragStartPoint = adjustedPoint - node->position;
        
        Refresh();
    } else {
        // Clicked on empty space
        if (!(keyflags & K_CTRL)) {
            DeselectAll();
        }
        
        Refresh();
    }
    
    Ctrl::LeftDown(point, keyflags);
}

void PertDiagram::LeftUp(Point point, dword keyflags) {
    if (isDragging) {
        isDragging = false;
        
        // Update task position in project
        PertNode* node = FindNode(dragTaskId);
        if (node) {
            Point adjustedPoint = ApplyInverseTransform(point);
            node->position = adjustedPoint - dragStartPoint;
            
            // Update project task position
            for (Task& task : project.tasks) {
                if (task.id == dragTaskId) {
                    task.xPosition = node->position.x;
                    task.yPosition = node->position.y;
                    break;
                }
            }
            
            WhenTaskMoved(dragTaskId);
            WhenDiagramChanged();
        }
    }
    
    Ctrl::LeftUp(point, keyflags);
}

void PertDiagram::MouseMove(Point point, dword keyflags) {
    Point adjustedPoint = ApplyInverseTransform(point);
    
    if (isDragging && dragTaskId > 0) {
        PertNode* node = FindNode(dragTaskId);
        if (node) {
            node->position = adjustedPoint - dragStartPoint;
            Refresh();
        }
    } else if (isConnecting) {
        // Update connection preview
        Refresh();
    }
    
    Ctrl::MouseMove(point, keyflags);
}

void PertDiagram::RightDown(Point point, dword keyflags) {
    Point adjustedPoint = ApplyInverseTransform(point);
    
    // Check if we clicked on a node for connection
    PertNode* node = FindNodeAt(adjustedPoint);
    if (node) {
        isConnecting = true;
        connectFromTaskId = node->taskId;
        connectStartPoint = adjustedPoint;
        Refresh();
    }
    
    Ctrl::RightDown(point, keyflags);
}

void PertDiagram::RightUp(Point point, dword keyflags) {
    if (isConnecting) {
        Point adjustedPoint = ApplyInverseTransform(point);
        PertNode* targetNode = FindNodeAt(adjustedPoint);
        
        if (targetNode && targetNode->taskId != connectFromTaskId) {
            // Add dependency
            AddDependency(connectFromTaskId, targetNode->taskId);
            WhenDependencyAdded(connectFromTaskId, targetNode->taskId);
            WhenDiagramChanged();
        }
        
        isConnecting = false;
        connectFromTaskId = 0;
        Refresh();
    }
    
    Ctrl::RightUp(point, keyflags);
}

void PertDiagram::MouseWheel(Point point, int zdelta, dword keyflags) {
    // Zoom in/out
    if (zdelta > 0) {
        zoom *= 1.1;
    } else {
        zoom /= 1.1;
    }
    
    // Clamp zoom
    if (zoom < 0.1) zoom = 0.1;
    if (zoom > 5.0) zoom = 5.0;
    
    Refresh();
    
    Ctrl::MouseWheel(point, zdelta, keyflags);
}

void PertDiagram::KeyDown(dword key, int count) {
    // Handle arrow keys for moving selected nodes
    if (!selectedNodes.IsEmpty()) {
        int dx = 0, dy = 0;
        
        switch (key) {
            case K_LEFT:  dx = -10; break;
            case K_RIGHT: dx = 10; break;
            case K_UP:    dy = -10; break;
            case K_DOWN:  dy = 10; break;
            default: break;
        }
        
        if (dx != 0 || dy != 0) {
            for (int taskId : selectedNodes) {
                PertNode* node = FindNode(taskId);
                if (node) {
                    node->position.x += dx;
                    node->position.y += dy;
                    
                    // Update project task position
                    for (Task& task : project.tasks) {
                        if (task.id == taskId) {
                            task.xPosition = node->position.x;
                            task.yPosition = node->position.y;
                            break;
                        }
                    }
                }
            }
            
            WhenDiagramChanged();
            Refresh();
        }
    }
    
    Ctrl::KeyDown(key, count);
}

void PertDiagram::SetZoom(double zoom) {
    this->zoom = zoom;
    Refresh();
}

double PertDiagram::GetZoom() const {
    return zoom;
}

void PertDiagram::SetOffset(const Point& offset) {
    this->offset = offset;
    Refresh();
}

Point PertDiagram::GetOffset() const {
    return offset;
}

void PertDiagram::CenterView() {
    Size sz = GetSize();
    offset = Point(sz.cx / 2, sz.cy / 2);
    Refresh();
}

void PertDiagram::FitToScreen() {
    if (nodes.IsEmpty()) {
        offset = Point(0, 0);
        zoom = 1.0;
        return;
    }
    
    // Calculate bounds
    int minX = INT_MAX, minY = INT_MAX;
    int maxX = INT_MIN, maxY = INT_MIN;
    
    for (const PertNode& node : nodes) {
        minX = min(minX, node.position.x);
        minY = min(minY, node.position.y);
        maxX = max(maxX, node.position.x + node.size.cx);
        maxY = max(maxY, node.position.y + node.size.cy);
    }
    
    int width = maxX - minX;
    int height = maxY - minY;
    
    if (width <= 0) width = 1;
    if (height <= 0) height = 1;
    
    Size sz = GetSize();
    double zoomX = static_cast<double>(sz.cx) / (width + 40);
    double zoomY = static_cast<double>(sz.cy) / (height + 40);
    zoom = min(zoomX, zoomY) * 0.9;
    
    offset.x = sz.cx / 2 - (minX + width / 2) * zoom;
    offset.y = sz.cy / 2 - (minY + height / 2) * zoom;
    
    Refresh();
}

void PertDiagram::SelectNode(int taskId) {
    if (!IsNodeSelected(taskId)) {
        selectedNodes.Add(taskId);
        WhenTaskSelected(taskId);
    }
    Refresh();
}

void PertDiagram::SelectNodes(const Vector<int>& taskIds) {
    selectedNodes = taskIds;
    Refresh();
}

void PertDiagram::DeselectAll() {
    selectedNodes.Clear();
    Refresh();
}

const Vector<int>& PertDiagram::GetSelectedNodes() const {
    return selectedNodes;
}

void PertDiagram::AddTask(const Task& task) {
    // Add to project
    project.AddTask(task);
    
    // Rebuild nodes and edges
    BuildNodes();
    BuildEdges();
    LayoutNodes();
    
    WhenDiagramChanged();
    Refresh();
}

void PertDiagram::UpdateTask(const Task& task) {
    // Update in project
    project.UpdateTask(task);
    
    // Rebuild nodes and edges
    BuildNodes();
    BuildEdges();
    
    WhenDiagramChanged();
    Refresh();
}

void PertDiagram::RemoveTask(int taskId) {
    // Remove from project
    project.RemoveTask(taskId);
    
    // Remove from nodes
    for (int i = 0; i < nodes.GetCount(); i++) {
        if (nodes[i].taskId == taskId) {
            nodes.Remove(i);
            break;
        }
    }
    
    // Remove edges involving this task
    for (int i = 0; i < edges.GetCount(); i++) {
        if (edges[i].fromTaskId == taskId || edges[i].toTaskId == taskId) {
            edges.Remove(i);
            i--;
        }
    }
    
    // Remove from selection
    for (int i = 0; i < selectedNodes.GetCount(); i++) {
        if (selectedNodes[i] == taskId) {
            selectedNodes.Remove(i);
            break;
        }
    }
    
    WhenDiagramChanged();
    Refresh();
}

void PertDiagram::AddDependency(int fromTaskId, int toTaskId) {
    // Add to project
    Task* fromTask = project.GetTaskById(fromTaskId);
    Task* toTask = project.GetTaskById(toTaskId);
    
    if (fromTask && toTask) {
        fromTask->AddDependency(toTaskId);
        
        // Rebuild edges
        BuildEdges();
        WhenDiagramChanged();
        Refresh();
    }
}

void PertDiagram::RemoveDependency(int fromTaskId, int toTaskId) {
    // Remove from project
    Task* fromTask = project.GetTaskById(fromTaskId);
    if (fromTask) {
        fromTask->RemoveDependency(toTaskId);
        
        // Rebuild edges
        BuildEdges();
        WhenDiagramChanged();
        Refresh();
    }
}

void PertDiagram::SetNodeSize(const Size& size) {
    nodeSize = size;
    for (PertNode& node : nodes) {
        node.size = size;
    }
    Refresh();
}

void PertDiagram::SetEdgeColor(const Color& color) {
    edgeColor = color;
    for (PertEdge& edge : edges) {
        edge.color = color;
    }
    Refresh();
}

void PertDiagram::SetSelectionColor(const Color& color) {
    selectionColor = color;
    Refresh();
}

void PertDiagram::SetBackgroundColor(const Color& color) {
    backgroundColor = color;
    Refresh();
}

Point PertDiagram::GetTaskPosition(int taskId) const {
    const PertNode* node = FindNode(taskId);
    if (node) {
        return node->position;
    }
    return Point(0, 0);
}

void PertDiagram::SetTaskPosition(int taskId, const Point& position) {
    PertNode* node = FindNode(taskId);
    if (node) {
        node->position = position;
        
        // Update in project
        for (Task& task : project.tasks) {
            if (task.id == taskId) {
                task.xPosition = position.x;
                task.yPosition = position.y;
                break;
            }
        }
        
        Refresh();
    }
}

bool PertDiagram::HasDependency(int fromTaskId, int toTaskId) const {
    const Task* fromTask = project.GetTaskById(fromTaskId);
    if (fromTask) {
        return fromTask->HasDependency(toTaskId);
    }
    return false;
}

Image PertDiagram::GetAsImage() const {
    Size sz = GetSize();
    ImageBuffer ib(sz);
    BufferPainter painter(ib);
    
    Paint(painter);
    
    return ib;
}

// Private methods
void PertDiagram::BuildNodes() {
    nodes.Clear();
    
    for (const Task& task : project.tasks) {
        Point position(static_cast<int>(task.xPosition), static_cast<int>(task.yPosition));
        PertNode node(task.id, position, nodeSize, task.color, task.name, task.priority, task.progress);
        nodes.Add(node);
    }
}

void PertDiagram::BuildEdges() {
    edges.Clear();
    
    for (const Task& task : project.tasks) {
        for (int depId : task.dependencies) {
            PertNode* fromNode = FindNode(task.id);
            PertNode* toNode = FindNode(depId);
            
            if (fromNode && toNode) {
                // Calculate connection points
                Point fromPoint = Point(fromNode->position.x + fromNode->size.cx / 2,
                                        fromNode->position.y + fromNode->size.cy);
                Point toPoint = Point(toNode->position.x + toNode->size.cx / 2,
                                      toNode->position.y);
                
                PertEdge edge(task.id, depId, fromPoint, toPoint, edgeColor);
                edges.Add(edge);
            }
        }
    }
}

void PertDiagram::LayoutNodes() {
    // Simple layout: arrange nodes in a grid
    if (nodes.IsEmpty()) return;
    
    int cols = static_cast<int>(ceil(sqrt(nodes.GetCount())));
    int rows = static_cast<int>(ceil(static_cast<double>(nodes.GetCount()) / cols));
    
    int index = 0;
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            if (index < nodes.GetCount()) {
                nodes[index].position = Point(col * (nodeSize.cx + NODE_MARGIN), 
                                               row * (nodeSize.cy + NODE_MARGIN));
                index++;
            }
        }
    }
}

Point PertDiagram::CalculateNodePosition(int taskId) const {
    // Find dependencies and calculate position based on them
    const Task* task = project.GetTaskById(taskId);
    if (!task) return Point(0, 0);
    
    // If task has dependencies, position it below the latest dependency
    if (!task->dependencies.IsEmpty()) {
        int maxY = 0;
        for (int depId : task->dependencies) {
            const PertNode* depNode = FindNode(depId);
            if (depNode) {
                maxY = max(maxY, depNode->position.y + depNode->size.cy + NODE_MARGIN);
            }
        }
        return Point(0, maxY);
    }
    
    return Point(0, 0);
}

PertNode* PertDiagram::FindNode(int taskId) {
    for (PertNode& node : nodes) {
        if (node.taskId == taskId) {
            return &node;
        }
    }
    return nullptr;
}

const PertNode* PertDiagram::FindNode(int taskId) const {
    for (const PertNode& node : nodes) {
        if (node.taskId == taskId) {
            return &node;
        }
    }
    return nullptr;
}

PertNode* PertDiagram::FindNodeAt(const Point& point) {
    for (PertNode& node : nodes) {
        if (node.Contains(point)) {
            return &node;
        }
    }
    return nullptr;
}

void PertDiagram::UpdateNode(int taskId, const PertNode& node) {
    for (PertNode& existingNode : nodes) {
        if (existingNode.taskId == taskId) {
            existingNode = node;
            return;
        }
    }
}

void PertDiagram::DrawNodes(Draw& draw) {
    for (const PertNode& node : nodes) {
        node.Draw(draw);
    }
}

void PertDiagram::DrawEdges(Draw& draw) {
    for (const PertEdge& edge : edges) {
        edge.Draw(draw);
    }
}

void PertDiagram::DrawSelection(Draw& draw) {
    for (int taskId : selectedNodes) {
        const PertNode* node = FindNode(taskId);
        if (node) {
            // Draw selection rectangle
            draw.DrawRect(node->position.x - SELECTION_MARGIN, 
                          node->position.y - SELECTION_MARGIN,
                          node->size.cx + SELECTION_MARGIN * 2,
                          node->size.cy + SELECTION_MARGIN * 2,
                          selectionColor);
        }
    }
}

void PertDiagram::DrawConnectionPreview(Draw& draw) {
    if (connectFromTaskId > 0) {
        const PertNode* fromNode = FindNode(connectFromTaskId);
        if (fromNode) {
            Point fromPoint = Point(fromNode->position.x + fromNode->size.cx / 2,
                                    fromNode->position.y + fromNode->size.cy);
            
            // Draw line to mouse position
            Point mousePoint = ApplyInverseTransform(GetMousePos());
            draw.DrawArrow(fromPoint.x, fromPoint.y, mousePoint.x, mousePoint.y, 2, edgeColor);
        }
    }
}

bool PertDiagram::IsNodeSelected(int taskId) const {
    return selectedNodes.Find(taskId) >= 0;
}

void PertDiagram::ApplyAutoLayout() {
    // Implement topological sorting based layout
    CalculateDependenciesDepth();
    
    // Sort nodes by dependency depth
    Vector<PertNode> sortedNodes = nodes;
    std::sort(sortedNodes.begin(), sortedNodes.end(), 
        [this](const PertNode& a, const PertNode& b) {
            const Task* taskA = project.GetTaskById(a.taskId);
            const Task* taskB = project.GetTaskById(b.taskId);
            
            if (!taskA || !taskB) return false;
            
            // If A depends on B, A should be after B
            if (taskA->HasDependency(taskB->id)) return false;
            if (taskB->HasDependency(taskA->id)) return true;
            
            // Otherwise, sort by priority
            return taskA->priority < taskB->priority;
        });
    
    // Layout nodes in sorted order
    int x = 0, y = 0;
    for (PertNode& node : sortedNodes) {
        node.position = Point(x, y);
        y += nodeSize.cy + NODE_MARGIN;
        
        // If we reach the bottom, move to next column
        if (y > GetSize().cy - nodeSize.cy) {
            y = 0;
            x += nodeSize.cx + NODE_MARGIN;
        }
    }
    
    nodes = sortedNodes;
    BuildEdges();
    Refresh();
}

void PertDiagram::CalculateDependenciesDepth() {
    // Calculate depth for each task based on dependencies
    // This is used for layout
}

// Helper method to transform screen coordinates to diagram coordinates
Point PertDiagram::ApplyInverseTransform(const Point& screenPoint) const {
    Point diagramPoint = screenPoint - offset;
    return Point(static_cast<int>(diagramPoint.x / zoom), 
                static_cast<int>(diagramPoint.y / zoom));
}
