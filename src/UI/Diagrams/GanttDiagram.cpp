#include "GanttDiagram.h"
#include "../../Model/Project.h"
#include "../../Model/Task.h"

using namespace Upp;

// Constants
const int DEFAULT_BAR_HEIGHT = 20;
const int DEFAULT_ROW_HEIGHT = 30;
const int DEFAULT_HEADER_HEIGHT = 40;
const int TIMELINE_MARGIN = 50;
const int SELECTION_MARGIN = 3;
const int GRID_LINE_SPACING = 7; // days

// GanttBar implementation
GanttBar::GanttBar() 
    : taskId(0), row(0), label(""), startDate(Null), endDate(Null), 
      color(White()), progress(0.0), priority(3) {}

GanttBar::GanttBar(int taskId, int row, const String& label, const Time& startDate, const Time& endDate,
                 const Color& color, double progress, int priority)
    : taskId(taskId), row(row), label(label), startDate(startDate), endDate(endDate),
      color(color), progress(progress), priority(priority) {}

void GanttBar::Draw(Draw& draw, int x, int y, int width, int height, const Time& timelineStart) const {
    // Calculate bar position and size
    int barX = x + GetXPosition(width, timelineStart, Time::GetCurrent());
    int barWidth = GetWidth(width, timelineStart, Time::GetCurrent());
    
    if (barWidth < 1) barWidth = 1;
    
    // Draw background
    draw.DrawRect(x, y, width, height, White());
    
    // Draw bar outline
    draw.DrawRect(barX, y + 2, barWidth, height - 4, Black());
    
    // Draw filled part based on progress
    if (progress > 0) {
        int filledWidth = static_cast<int>(barWidth * (progress / 100.0));
        if (filledWidth > 0) {
            draw.DrawRect(barX, y + 2, filledWidth, height - 4, color);
        }
    } else {
        // If no progress, fill with light color
        draw.DrawRect(barX, y + 2, barWidth, height - 4, color);
    }
    
    // Draw label
    draw.DrawText(x + 5, y + 2, label, StdFont(), textColor);
    
    // Draw dates
    if (!startDate.IsNull()) {
        String startText = startDate.ToString("%Y-%m-%d");
        draw.DrawText(barX, y + 2, startText, StdFont().Italic(), textColor);
    }
    
    if (!endDate.IsNull()) {
        String endText = endDate.ToString("%Y-%m-%d");
        draw.DrawText(barX + barWidth - 50, y + 2, endText, StdFont().Italic(), textColor);
    }
    
    // Draw progress text
    String progressText = Format("%d%%", static_cast<int>(progress));
    draw.DrawText(barX + barWidth - 30, y + 2, progressText, StdFont(), textColor);
}

bool GanttBar::Contains(const Point& point, int x, int y, int width, int height, const Time& timelineStart) const {
    int barX = x + GetXPosition(width, timelineStart, Time::GetCurrent());
    int barWidth = GetWidth(width, timelineStart, Time::GetCurrent());
    
    return point.x >= barX && point.x <= barX + barWidth &&
           point.y >= y && point.y <= y + height;
}

int GanttBar::GetXPosition(int chartWidth, const Time& timelineStart, const Time& timelineEnd) const {
    if (startDate.IsNull() || timelineStart.IsNull() || timelineEnd.IsNull()) {
        return 0;
    }
    
    double totalDays = (timelineEnd - timelineStart).GetDays();
    if (totalDays <= 0) return 0;
    
    double daysFromStart = (startDate - timelineStart).GetDays();
    double ratio = daysFromStart / totalDays;
    
    return static_cast<int>(ratio * chartWidth);
}

int GanttBar::GetWidth(int chartWidth, const Time& timelineStart, const Time& timelineEnd) const {
    if (startDate.IsNull() || endDate.IsNull() || timelineStart.IsNull() || timelineEnd.IsNull()) {
        return 0;
    }
    
    double totalDays = (timelineEnd - timelineStart).GetDays();
    if (totalDays <= 0) return 0;
    
    double taskDays = (endDate - startDate).GetDays();
    if (taskDays <= 0) return 1;
    
    double ratio = taskDays / totalDays;
    
    return static_cast<int>(ratio * chartWidth);
}

// GanttDiagram implementation
GanttDiagram::GanttDiagram() 
    : zoom(1.0), offset(0, 0), barHeight(DEFAULT_BAR_HEIGHT), 
      rowHeight(DEFAULT_ROW_HEIGHT), headerHeight(DEFAULT_HEADER_HEIGHT),
      selectionColor(LtBlue()), backgroundColor(White()), gridColor(LtGray()), 
      textColor(Black()), isDragging(false), isConnecting(false), 
      dragTaskId(0), connectFromTaskId(0), showResourceProfile(false) {
    SetFrame(InsetFrame());
    SetMinSize(Size(600, 400));
    
    // Initialize timeline
    timelineStart = Time::GetCurrent();
    timelineEnd = timelineStart + 30; // 30 days
}

GanttDiagram::~GanttDiagram() {}

void GanttDiagram::SetProject(const Project& project) {
    this->project = project;
    BuildBars();
    FitTimelineToProject();
    Refresh();
}

const Project& GanttDiagram::GetProject() const {
    return project;
}

void GanttDiagram::UpdateTaskPositions() {
    // Update positions from project tasks
    BuildBars();
    Refresh();
}

void GanttDiagram::Paint(Draw& draw) {
    Size sz = GetSize();
    
    // Draw background
    draw.DrawRect(0, 0, sz.cx, sz.cy, backgroundColor);
    
    // Apply zoom and offset
    draw.Offset(offset);
    draw.Scale(zoom);
    
    // Draw header
    DrawHeader(draw);
    
    // Draw bars
    DrawBars(draw);
    
    // Draw dependencies
    DrawDependencies(draw);
    
    // Draw selection
    DrawSelection(draw);
    
    // Draw connection preview if connecting
    if (isConnecting) {
        DrawConnectionPreview(draw);
    }
    
    // Draw resource profile if enabled
    if (showResourceProfile) {
        DrawResourceProfile(draw);
    }
}

void GanttDiagram::LeftDown(Point point, dword keyflags) {
    Point adjustedPoint = ApplyInverseTransform(point);
    
    // Check if we clicked on a bar
    GanttBar* bar = FindBarAt(adjustedPoint);
    if (bar) {
        if (!(keyflags & K_CTRL)) {
            DeselectAll();
        }
        
        if (!IsBarSelected(bar->taskId)) {
            selectedBars.Add(bar->taskId);
            WhenTaskSelected(bar->taskId);
        }
        
        // Start dragging
        isDragging = true;
        dragTaskId = bar->taskId;
        dragStartPoint = adjustedPoint;
        
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

void GanttDiagram::LeftUp(Point point, dword keyflags) {
    if (isDragging) {
        isDragging = false;
        
        // Update task position in project
        GanttBar* bar = FindBar(dragTaskId);
        if (bar) {
            Point adjustedPoint = ApplyInverseTransform(point);
            
            // Calculate new dates based on position
            Time newStartDate = XToDate(adjustedPoint.x);
            
            // Find task and update its start date
            for (Task& task : project.tasks) {
                if (task.id == dragTaskId) {
                    task.startDate = newStartDate;
                    task.endDate = Task::CalculateEndDate(newStartDate, task.duration);
                    break;
                }
            }
            
            // Rebuild bars
            BuildBars();
            
            WhenTaskMoved(dragTaskId);
            WhenDiagramChanged();
        }
    }
    
    Ctrl::LeftUp(point, keyflags);
}

void GanttDiagram::MouseMove(Point point, dword keyflags) {
    Point adjustedPoint = ApplyInverseTransform(point);
    
    if (isDragging && dragTaskId > 0) {
        // For Gantt chart, we don't actually move the bar visually during drag
        // The movement is handled in LeftUp
        Refresh();
    } else if (isConnecting) {
        // Update connection preview
        Refresh();
    }
    
    Ctrl::MouseMove(point, keyflags);
}

void GanttDiagram::RightDown(Point point, dword keyflags) {
    Point adjustedPoint = ApplyInverseTransform(point);
    
    // Check if we clicked on a bar for connection
    GanttBar* bar = FindBarAt(adjustedPoint);
    if (bar) {
        isConnecting = true;
        connectFromTaskId = bar->taskId;
        connectStartPoint = adjustedPoint;
        Refresh();
    }
    
    Ctrl::RightDown(point, keyflags);
}

void GanttDiagram::RightUp(Point point, dword keyflags) {
    if (isConnecting) {
        Point adjustedPoint = ApplyInverseTransform(point);
        GanttBar* targetBar = FindBarAt(adjustedPoint);
        
        if (targetBar && targetBar->taskId != connectFromTaskId) {
            // Add dependency
            AddDependency(connectFromTaskId, targetBar->taskId);
            WhenDependencyAdded(connectFromTaskId, targetBar->taskId);
            WhenDiagramChanged();
        }
        
        isConnecting = false;
        connectFromTaskId = 0;
        Refresh();
    }
    
    Ctrl::RightUp(point, keyflags);
}

void GanttDiagram::MouseWheel(Point point, int zdelta, dword keyflags) {
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

void GanttDiagram::KeyDown(dword key, int count) {
    // Handle arrow keys for moving selected bars
    if (!selectedBars.IsEmpty()) {
        int dx = 0;
        
        switch (key) {
            case K_LEFT:  dx = -1; break; // Move back one day
            case K_RIGHT: dx = 1; break;  // Move forward one day
            default: break;
        }
        
        if (dx != 0) {
            for (int taskId : selectedBars) {
                // Find task and update its start date
                for (Task& task : project.tasks) {
                    if (task.id == taskId) {
                        task.startDate = task.startDate + dx * 24 * 3600;
                        task.endDate = Task::CalculateEndDate(task.startDate, task.duration);
                        break;
                    }
                }
            }
            
            // Rebuild bars
            BuildBars();
            
            WhenDiagramChanged();
            Refresh();
        }
    }
    
    Ctrl::KeyDown(key, count);
}

void GanttDiagram::SetTimelineStart(const Time& start) {
    timelineStart = start;
    BuildBars();
    Refresh();
}

void GanttDiagram::SetTimelineEnd(const Time& end) {
    timelineEnd = end;
    BuildBars();
    Refresh();
}

Time GanttDiagram::GetTimelineStart() const {
    return timelineStart;
}

Time GanttDiagram::GetTimelineEnd() const {
    return timelineEnd;
}

void GanttDiagram::FitTimelineToProject() {
    if (project.tasks.IsEmpty()) {
        timelineStart = Time::GetCurrent();
        timelineEnd = timelineStart + 30;
        return;
    }
    
    Time minStart = Null;
    Time maxEnd = Null;
    
    for (const Task& task : project.tasks) {
        if (!task.startDate.IsNull()) {
            if (minStart.IsNull() || task.startDate < minStart) {
                minStart = task.startDate;
            }
        }
        
        if (!task.endDate.IsNull()) {
            if (maxEnd.IsNull() || task.endDate > maxEnd) {
                maxEnd = task.endDate;
            }
        }
    }
    
    if (!minStart.IsNull()) {
        timelineStart = minStart - 5; // Add 5 days margin
    } else {
        timelineStart = Time::GetCurrent();
    }
    
    if (!maxEnd.IsNull()) {
        timelineEnd = maxEnd + 5; // Add 5 days margin
    } else {
        timelineEnd = timelineStart + 30;
    }
    
    BuildBars();
    Refresh();
}

void GanttDiagram::SetZoom(double zoom) {
    this->zoom = zoom;
    Refresh();
}

double GanttDiagram::GetZoom() const {
    return zoom;
}

void GanttDiagram::SetOffset(const Point& offset) {
    this->offset = offset;
    Refresh();
}

Point GanttDiagram::GetOffset() const {
    return offset;
}

void GanttDiagram::CenterView() {
    Size sz = GetSize();
    offset = Point(0, sz.cy / 2);
    Refresh();
}

void GanttDiagram::FitToScreen() {
    Size sz = GetSize();
    
    // Calculate required height
    int requiredHeight = headerHeight + (bars.GetCount() * rowHeight);
    
    if (requiredHeight < sz.cy) {
        offset.y = 0;
    } else {
        offset.y = 0; // Start at top
    }
    
    offset.x = 0;
    zoom = 1.0;
    
    Refresh();
}

void GanttDiagram::SelectBar(int taskId) {
    if (!IsBarSelected(taskId)) {
        selectedBars.Add(taskId);
        WhenTaskSelected(taskId);
    }
    Refresh();
}

void GanttDiagram::SelectBars(const Vector<int>& taskIds) {
    selectedBars = taskIds;
    Refresh();
}

void GanttDiagram::DeselectAll() {
    selectedBars.Clear();
    Refresh();
}

const Vector<int>& GanttDiagram::GetSelectedBars() const {
    return selectedBars;
}

void GanttDiagram::AddTask(const Task& task) {
    // Add to project
    project.AddTask(task);
    
    // Rebuild bars
    BuildBars();
    FitTimelineToProject();
    
    WhenDiagramChanged();
    Refresh();
}

void GanttDiagram::UpdateTask(const Task& task) {
    // Update in project
    project.UpdateTask(task);
    
    // Rebuild bars
    BuildBars();
    FitTimelineToProject();
    
    WhenDiagramChanged();
    Refresh();
}

void GanttDiagram::RemoveTask(int taskId) {
    // Remove from project
    project.RemoveTask(taskId);
    
    // Remove from bars
    for (int i = 0; i < bars.GetCount(); i++) {
        if (bars[i].taskId == taskId) {
            bars.Remove(i);
            break;
        }
    }
    
    // Remove from selection
    for (int i = 0; i < selectedBars.GetCount(); i++) {
        if (selectedBars[i] == taskId) {
            selectedBars.Remove(i);
            break;
        }
    }
    
    WhenDiagramChanged();
    Refresh();
}

void GanttDiagram::AddDependency(int fromTaskId, int toTaskId) {
    // Add to project
    Task* fromTask = project.GetTaskById(fromTaskId);
    Task* toTask = project.GetTaskById(toTaskId);
    
    if (fromTask && toTask) {
        fromTask->AddDependency(toTaskId);
        
        // Rebuild bars
        BuildBars();
        WhenDiagramChanged();
        Refresh();
    }
}

void GanttDiagram::RemoveDependency(int fromTaskId, int toTaskId) {
    // Remove from project
    Task* fromTask = project.GetTaskById(fromTaskId);
    if (fromTask) {
        fromTask->RemoveDependency(toTaskId);
        
        // Rebuild bars
        BuildBars();
        WhenDiagramChanged();
        Refresh();
    }
}

void GanttDiagram::SetBarHeight(int height) {
    barHeight = height;
    Refresh();
}

void GanttDiagram::SetRowHeight(int height) {
    rowHeight = height;
    Refresh();
}

void GanttDiagram::SetHeaderHeight(int height) {
    headerHeight = height;
    Refresh();
}

void GanttDiagram::SetSelectionColor(const Color& color) {
    selectionColor = color;
    Refresh();
}

void GanttDiagram::SetBackgroundColor(const Color& color) {
    backgroundColor = color;
    Refresh();
}

void GanttDiagram::SetGridColor(const Color& color) {
    gridColor = color;
    Refresh();
}

void GanttDiagram::SetTextColor(const Color& color) {
    textColor = color;
    Refresh();
}

Point GanttDiagram::GetTaskPosition(int taskId) const {
    const GanttBar* bar = FindBar(taskId);
    if (bar) {
        return Point(bar->row * rowHeight, 0);
    }
    return Point(0, 0);
}

void GanttDiagram::SetTaskPosition(int taskId, const Point& position) {
    GanttBar* bar = FindBar(taskId);
    if (bar) {
        bar->row = position.y / rowHeight;
        
        // Update in project
        for (Task& task : project.tasks) {
            if (task.id == taskId) {
                task.yPosition = bar->row * rowHeight;
                break;
            }
        }
        
        Refresh();
    }
}

bool GanttDiagram::HasDependency(int fromTaskId, int toTaskId) const {
    const Task* fromTask = project.GetTaskById(fromTaskId);
    if (fromTask) {
        return fromTask->HasDependency(toTaskId);
    }
    return false;
}

Image GanttDiagram::GetAsImage() const {
    Size sz = GetSize();
    ImageBuffer ib(sz);
    BufferPainter painter(ib);
    
    Paint(painter);
    
    return ib;
}

void GanttDiagram::ShowResourceProfile(bool show) {
    showResourceProfile = show;
    Refresh();
}

bool GanttDiagram::IsResourceProfileShown() const {
    return showResourceProfile;
}

// Private methods
void GanttDiagram::BuildBars() {
    bars.Clear();
    
    int row = 0;
    for (const Task& task : project.tasks) {
        GanttBar bar(task.id, row, task.name, task.startDate, task.endDate,
                     task.color, task.progress, task.priority);
        bars.Add(bar);
        row++;
    }
}

void GanttDiagram::BuildDependencies() {
    // Dependencies are handled through the project model
}

void GanttDiagram::LayoutBars() {
    // Bars are laid out vertically by row
}

GanttBar* GanttDiagram::FindBar(int taskId) {
    for (GanttBar& bar : bars) {
        if (bar.taskId == taskId) {
            return &bar;
        }
    }
    return nullptr;
}

const GanttBar* GanttDiagram::FindBar(int taskId) const {
    for (const GanttBar& bar : bars) {
        if (bar.taskId == taskId) {
            return &bar;
        }
    }
    return nullptr;
}

GanttBar* GanttDiagram::FindBarAt(const Point& point) {
    Size sz = GetSize();
    int chartWidth = sz.cx - TIMELINE_MARGIN;
    
    for (GanttBar& bar : bars) {
        int y = headerHeight + bar.row * rowHeight;
        if (bar.Contains(point, TIMELINE_MARGIN, y, chartWidth, rowHeight, timelineStart)) {
            return &bar;
        }
    }
    return nullptr;
}

void GanttDiagram::DrawHeader(Draw& draw) {
    Size sz = GetSize();
    int chartWidth = sz.cx - TIMELINE_MARGIN;
    
    // Draw header background
    draw.DrawRect(0, 0, sz.cx, headerHeight, LtGray());
    
    // Draw timeline labels
    DrawTimeline(draw, 0, chartWidth);
    
    // Draw grid lines
    draw.DrawLine(TIMELINE_MARGIN, headerHeight, TIMELINE_MARGIN, headerHeight + bars.GetCount() * rowHeight, gridColor);
}

void GanttDiagram::DrawBars(Draw& draw) {
    Size sz = GetSize();
    int chartWidth = sz.cx - TIMELINE_MARGIN;
    
    for (GanttBar& bar : bars) {
        int y = headerHeight + bar.row * rowHeight;
        bar.Draw(draw, TIMELINE_MARGIN, y, chartWidth, rowHeight, timelineStart);
    }
}

void GanttDiagram::DrawDependencies(Draw& draw) {
    // Draw dependency lines between tasks
    for (const Task& task : project.tasks) {
        for (int depId : task.dependencies) {
            GanttBar* fromBar = FindBar(task.id);
            GanttBar* toBar = FindBar(depId);
            
            if (fromBar && toBar) {
                int fromY = headerHeight + fromBar->row * rowHeight + rowHeight / 2;
                int toY = headerHeight + toBar->row * rowHeight + rowHeight / 2;
                
                int fromX = TIMELINE_MARGIN + fromBar->GetXPosition(GetSize().cx - TIMELINE_MARGIN, 
                                                                  timelineStart, timelineEnd) + 
                           fromBar->GetWidth(GetSize().cx - TIMELINE_MARGIN, 
                                            timelineStart, timelineEnd);
                int toX = TIMELINE_MARGIN + toBar->GetXPosition(GetSize().cx - TIMELINE_MARGIN, 
                                                                timelineStart, timelineEnd);
                
                // Draw arrow from end of fromBar to start of toBar
                draw.DrawArrow(fromX, fromY, toX, toY, 1, gridColor);
            }
        }
    }
}

void GanttDiagram::DrawSelection(Draw& draw) {
    Size sz = GetSize();
    int chartWidth = sz.cx - TIMELINE_MARGIN;
    
    for (int taskId : selectedBars) {
        const GanttBar* bar = FindBar(taskId);
        if (bar) {
            int y = headerHeight + bar.row * rowHeight;
            int barX = TIMELINE_MARGIN + bar->GetXPosition(chartWidth, timelineStart, timelineEnd);
            int barWidth = bar->GetWidth(chartWidth, timelineStart, timelineEnd);
            
            // Draw selection rectangle
            draw.DrawRect(barX - SELECTION_MARGIN, y - SELECTION_MARGIN,
                          barWidth + SELECTION_MARGIN * 2,
                          rowHeight + SELECTION_MARGIN * 2,
                          selectionColor);
        }
    }
}

void GanttDiagram::DrawConnectionPreview(Draw& draw) {
    if (connectFromTaskId > 0) {
        GanttBar* fromBar = FindBar(connectFromTaskId);
        if (fromBar) {
            Point mousePoint = ApplyInverseTransform(GetMousePos());
            
            int fromY = headerHeight + fromBar->row * rowHeight + rowHeight / 2;
            int fromX = TIMELINE_MARGIN + fromBar->GetXPosition(GetSize().cx - TIMELINE_MARGIN,
                                                                timelineStart, timelineEnd) +
                       fromBar->GetWidth(GetSize().cx - TIMELINE_MARGIN,
                                        timelineStart, timelineEnd);
            
            draw.DrawArrow(fromX, fromY, mousePoint.x, mousePoint.y, 1, gridColor);
        }
    }
}

void GanttDiagram::DrawResourceProfile(Draw& draw) {
    // Draw resource utilization profile at the bottom
    Size sz = GetSize();
    int profileHeight = 100;
    int y = sz.cy - profileHeight;
    
    // Draw background
    draw.DrawRect(0, y, sz.cx, profileHeight, LtGray());
    
    // Draw title
    draw.DrawText(10, y + 10, "Resource Profile", StdFont().Bold(), textColor);
    
    // Calculate and draw resource utilization
    // This is a simplified version - in a real implementation, you would
    // calculate the utilization for each resource over time
    
    int x = TIMELINE_MARGIN;
    int width = sz.cx - TIMELINE_MARGIN;
    
    // Draw axis
    draw.DrawLine(x, y + 30, x + width, y + 30, Black());
    
    // For each resource, draw utilization curve
    for (const Resource& resource : project.resources) {
        // Draw resource name
        draw.DrawText(x, y + 40, resource.name, StdFont(), textColor);
        
        // In a real implementation, you would calculate and draw the utilization curve
        // For now, just draw a placeholder
        draw.DrawLine(x, y + 50, x + width, y + 50, resource.color);
        
        y += 20;
    }
}

void GanttDiagram::DrawTimeline(Draw& draw, int y, int width) {
    // Calculate number of days in timeline
    double totalDays = (timelineEnd - timelineStart).GetDays();
    if (totalDays <= 0) return;
    
    // Draw date labels at reasonable intervals
    int labelInterval = 7; // days
    if (totalDays > 60) labelInterval = 14;
    if (totalDays > 180) labelInterval = 30;
    
    // Draw grid lines and labels
    for (int day = 0; day <= static_cast<int>(totalDays); day += labelInterval) {
        Time date = timelineStart + day * 24 * 3600;
        int x = TIMELINE_MARGIN + static_cast<int>((static_cast<double>(day) / totalDays) * width);
        
        // Draw grid line
        draw.DrawLine(x, y, x, y + headerHeight, gridColor);
        
        // Draw date label
        String dateLabel = date.ToString("%m/%d");
        draw.DrawText(x - 20, y + 5, dateLabel, StdFont().Small(), textColor);
    }
}

String GanttDiagram::FormatDate(const Time& date) const {
    if (date.IsNull()) return "";
    return date.ToString("%Y-%m-%d");
}

int GanttDiagram::DateToX(const Time& date) const {
    if (date.IsNull() || timelineStart.IsNull() || timelineEnd.IsNull()) {
        return TIMELINE_MARGIN;
    }
    
    double totalDays = (timelineEnd - timelineStart).GetDays();
    if (totalDays <= 0) return TIMELINE_MARGIN;
    
    double daysFromStart = (date - timelineStart).GetDays();
    double ratio = daysFromStart / totalDays;
    
    Size sz = GetSize();
    int chartWidth = sz.cx - TIMELINE_MARGIN;
    
    return TIMELINE_MARGIN + static_cast<int>(ratio * chartWidth);
}

Time GanttDiagram::XToDate(int x) const {
    Size sz = GetSize();
    int chartWidth = sz.cx - TIMELINE_MARGIN;
    
    if (chartWidth <= 0) return timelineStart;
    
    double ratio = static_cast<double>(x - TIMELINE_MARGIN) / chartWidth;
    double totalDays = (timelineEnd - timelineStart).GetDays();
    
    double daysFromStart = ratio * totalDays;
    
    return timelineStart + static_cast<int>(daysFromStart) * 24 * 3600;
}

bool GanttDiagram::IsBarSelected(int taskId) const {
    return selectedBars.Find(taskId) >= 0;
}

void GanttDiagram::ApplyAutoLayout() {
    // For Gantt chart, auto-layout means sorting tasks by start date
    std::sort(bars.begin(), bars.end(), [](const GanttBar& a, const GanttBar& b) {
        if (a.startDate.IsNull() && b.startDate.IsNull()) return a.taskId < b.taskId;
        if (a.startDate.IsNull()) return false;
        if (b.startDate.IsNull()) return true;
        return a.startDate < b.startDate;
    });
    
    // Update row numbers
    for (int i = 0; i < bars.GetCount(); i++) {
        bars[i].row = i;
    }
    
    Refresh();
}

Point GanttDiagram::ApplyInverseTransform(const Point& screenPoint) const {
    Point diagramPoint = screenPoint - offset;
    return Point(static_cast<int>(diagramPoint.x / zoom), 
                static_cast<int>(diagramPoint.y / zoom));
}
