#include "Exporter.h"
#include "../Model/Project.h"
#include "../UI/Diagrams/PertDiagram.h"
#include "../UI/Diagrams/GanttDiagram.h"

using namespace Upp;

// ExportResult implementation
ExportResult::ExportResult() : success(false) {}

void ExportResult::Clear() {
    success = false;
    message.Clear();
    filePath.Clear();
}

// Base Exporter implementation
Exporter::Exporter() : resolution(96), quality(100), compression(true) {}

Exporter::~Exporter() {}

void Exporter::SetResolution(int dpi) {
    resolution = dpi;
}

int Exporter::GetResolution() const {
    return resolution;
}

void Exporter::SetQuality(int quality) {
    this->quality = quality;
    if (this->quality < 0) this->quality = 0;
    if (this->quality > 100) this->quality = 100;
}

int Exporter::GetQuality() const {
    return quality;
}

void Exporter::SetCompression(bool enable) {
    compression = enable;
}

bool Exporter::GetCompression() const {
    return compression;
}

// PNGExporter implementation
PNGExporter::PNGExporter() {}

PNGExporter::~PNGExporter() {}

ExportResult PNGExporter::Export(const Project& project, const String& filePath) {
    ExportResult result;
    
    // For project export, we would typically export a diagram
    // For now, we'll just return an error
    result.success = false;
    result.message = "PNG export requires a diagram. Use Export(diagram, filePath) instead.";
    result.filePath = filePath;
    
    return result;
}

ExportResult PNGExporter::Export(const PertDiagram& diagram, const String& filePath) {
    ExportResult result;
    
    try {
        Image diagramImage = diagram.GetAsImage();
        if (SaveImageToPNG(diagramImage, filePath)) {
            result.success = true;
            result.message = "PERT diagram exported successfully";
            result.filePath = filePath;
        } else {
            result.success = false;
            result.message = "Failed to save PNG file";
            result.filePath = filePath;
        }
    } catch (const std::exception& e) {
        result.success = false;
        result.message = e.what();
        result.filePath = filePath;
    }
    
    return result;
}

ExportResult PNGExporter::Export(const GanttDiagram& diagram, const String& filePath) {
    ExportResult result;
    
    try {
        Image diagramImage = diagram.GetAsImage();
        if (SaveImageToPNG(diagramImage, filePath)) {
            result.success = true;
            result.message = "Gantt diagram exported successfully";
            result.filePath = filePath;
        } else {
            result.success = false;
            result.message = "Failed to save PNG file";
            result.filePath = filePath;
        }
    } catch (const std::exception& e) {
        result.success = false;
        result.message = e.what();
        result.filePath = filePath;
    }
    
    return result;
}

Vector<String> PNGExporter::GetSupportedExtensions() const {
    Vector<String> extensions;
    extensions.Add(".png");
    return extensions;
}

String PNGExporter::GetFormatDescription() const {
    return "PNG Image";
}

bool PNGExporter::SaveImageToPNG(const Image& image, const String& filePath) {
    // U++ provides built-in PNG support
    try {
        SaveFile(filePath, image);
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

// QTFExporter implementation
QTFExporter::QTFExporter() {}

QTFExporter::~QTFExporter() {}

ExportResult QTFExporter::Export(const Project& project, const String& filePath) {
    ExportResult result;
    
    try {
        // Create a simple QTF document with project information
        String qtfContent = CreateQTFDocument(project, Image());
        
        if (SaveFile(filePath, qtfContent)) {
            result.success = true;
            result.message = "Project exported to QTF successfully";
            result.filePath = filePath;
        } else {
            result.success = false;
            result.message = "Failed to save QTF file";
            result.filePath = filePath;
        }
    } catch (const std::exception& e) {
        result.success = false;
        result.message = e.what();
        result.filePath = filePath;
    }
    
    return result;
}

ExportResult QTFExporter::Export(const PertDiagram& diagram, const String& filePath) {
    ExportResult result;
    
    try {
        String qtfContent = CreateQTFDocument(diagram);
        
        if (SaveFile(filePath, qtfContent)) {
            result.success = true;
            result.message = "PERT diagram exported to QTF successfully";
            result.filePath = filePath;
        } else {
            result.success = false;
            result.message = "Failed to save QTF file";
            result.filePath = filePath;
        }
    } catch (const std::exception& e) {
        result.success = false;
        result.message = e.what();
        result.filePath = filePath;
    }
    
    return result;
}

ExportResult QTFExporter::Export(const GanttDiagram& diagram, const String& filePath) {
    ExportResult result;
    
    try {
        String qtfContent = CreateQTFDocument(diagram);
        
        if (SaveFile(filePath, qtfContent)) {
            result.success = true;
            result.message = "Gantt diagram exported to QTF successfully";
            result.filePath = filePath;
        } else {
            result.success = false;
            result.message = "Failed to save QTF file";
            result.filePath = filePath;
        }
    } catch (const std::exception& e) {
        result.success = false;
        result.message = e.what();
        result.filePath = filePath;
    }
    
    return result;
}

Vector<String> QTFExporter::GetSupportedExtensions() const {
    Vector<String> extensions;
    extensions.Add(".qtf");
    extensions.Add(".rtf");
    return extensions;
}

String QTFExporter::GetFormatDescription() const {
    return "QTF/Rich Text Document";
}

String QTFExporter::CreateQTFDocument(const Project& project, const Image& diagramImage) {
    String qtf;
    
    // QTF header
    qtf += "{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang1036\\uc1\\xn\n";
    
    // Title
    qtf += "{\\b\\fs24 Project: " + project.name + "}\n\\par\n\n";
    
    // Project description
    qtf += "{\\b Description:}\n\\par\n" + project.description + "\n\\par\n\n";
    
    // Project statistics
    qtf += "{\\b Project Statistics}\n\\par\n";
    qtf += "Total Tasks: " + AsString(project.tasks.GetCount()) + "\n\\par\n";
    qtf += "Total Resources: " + AsString(project.resources.GetCount()) + "\n\\par\n";
    qtf += "Total Workload: " + AsString(project.GetTotalWorkload()) + " person-days\n\\par\n";
    qtf += "Completed: " + AsString(project.GetCompletedWorkload()) + " person-days\n\\par\n";
    qtf += "Overall Progress: " + AsString(project.GetOverallProgress()) + "%\n\\par\n\n";
    
    // Tasks
    qtf += "{\\b Tasks}\n\\par\n";
    for (const Task& task : project.tasks) {
        qtf += "\\bullet " + task.name + " (P" + AsString(task.priority) + ", " + 
               AsString(task.progress) + "% complete)\n\\par\n";
    }
    qtf += "\n\\par\n";
    
    // Resources
    qtf += "{\\b Resources}\n\\par\n";
    for (const Resource& resource : project.resources) {
        qtf += "\\bullet " + resource.name + " (Max: " + AsString(resource.maxUtilizationRate) + "%)\n\\par\n";
    }
    
    // Close document
    qtf += "}\n";
    
    return qtf;
}

String QTFExporter::CreateQTFDocument(const PertDiagram& diagram) {
    const Project& project = diagram.GetProject();
    
    String qtf;
    
    // QTF header
    qtf += "{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang1036\\uc1\\xn\n";
    
    // Title
    qtf += "{\\b\\fs24 PERT Diagram: " + project.name + "}\n\\par\n\n";
    
    // Diagram description
    qtf += "This document contains the PERT diagram for the project.\n\\par\n\n";
    
    // Task list
    qtf += "{\\b Tasks}\n\\par\n";
    for (const Task& task : project.tasks) {
        qtf += "\\bullet " + task.name + "\n\\par\n";
    }
    
    // Dependencies
    qtf += "{\\b Dependencies}\n\\par\n";
    for (const Task& task : project.tasks) {
        if (!task.dependencies.IsEmpty()) {
            qtf += task.name + " depends on:\n\\par\n";
            for (int depId : task.dependencies) {
                const Task* depTask = project.GetTaskById(depId);
                if (depTask) {
                    qtf += "  \\bullet " + depTask->name + "\n\\par\n";
                }
            }
        }
    }
    
    // Close document
    qtf += "}\n";
    
    return qtf;
}

String QTFExporter::CreateQTFDocument(const GanttDiagram& diagram) {
    const Project& project = diagram.GetProject();
    
    String qtf;
    
    // QTF header
    qtf += "{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang1036\\uc1\\xn\n";
    
    // Title
    qtf += "{\\b\\fs24 Gantt Chart: " + project.name + "}\n\\par\n\n";
    
    // Diagram description
    qtf += "This document contains the Gantt chart for the project.\n\\par\n\n";
    
    // Timeline
    qtf += "{\\b Timeline}\n\\par\n";
    qtf += "Start: " + diagram.GetTimelineStart().ToString() + "\n\\par\n";
    qtf += "End: " + diagram.GetTimelineEnd().ToString() + "\n\\par\n\n";
    
    // Tasks
    qtf += "{\\b Tasks}\n\\par\n";
    for (const Task& task : project.tasks) {
        qtf += "\\bullet " + task.name + "\n\\par\n";
        qtf += "  Start: " + task.startDate.ToString() + "\n\\par\n";
        qtf += "  End: " + task.endDate.ToString() + "\n\\par\n";
        qtf += "  Progress: " + AsString(task.progress) + "%\n\\par\n";
    }
    
    // Close document
    qtf += "}\n";
    
    return qtf;
}

// PowerPointExporter implementation
PowerPointExporter::PowerPointExporter() {}

PowerPointExporter::~PowerPointExporter() {}

ExportResult PowerPointExporter::Export(const Project& project, const String& filePath) {
    ExportResult result;
    
    try {
        if (SaveAsPowerPoint(project, filePath)) {
            result.success = true;
            result.message = "Project exported to PowerPoint successfully";
            result.filePath = filePath;
        } else {
            result.success = false;
            result.message = "Failed to save PowerPoint file";
            result.filePath = filePath;
        }
    } catch (const std::exception& e) {
        result.success = false;
        result.message = e.what();
        result.filePath = filePath;
    }
    
    return result;
}

ExportResult PowerPointExporter::Export(const PertDiagram& diagram, const String& filePath) {
    ExportResult result;
    
    try {
        Image diagramImage = diagram.GetAsImage();
        if (SaveDiagramAsPowerPoint(diagramImage, filePath, "PERT Diagram - " + diagram.GetProject().name)) {
            result.success = true;
            result.message = "PERT diagram exported to PowerPoint successfully";
            result.filePath = filePath;
        } else {
            result.success = false;
            result.message = "Failed to save PowerPoint file";
            result.filePath = filePath;
        }
    } catch (const std::exception& e) {
        result.success = false;
        result.message = e.what();
        result.filePath = filePath;
    }
    
    return result;
}

ExportResult PowerPointExporter::Export(const GanttDiagram& diagram, const String& filePath) {
    ExportResult result;
    
    try {
        Image diagramImage = diagram.GetAsImage();
        if (SaveDiagramAsPowerPoint(diagramImage, filePath, "Gantt Chart - " + diagram.GetProject().name)) {
            result.success = true;
            result.message = "Gantt diagram exported to PowerPoint successfully";
            result.filePath = filePath;
        } else {
            result.success = false;
            result.message = "Failed to save PowerPoint file";
            result.filePath = filePath;
        }
    } catch (const std::exception& e) {
        result.success = false;
        result.message = e.what();
        result.filePath = filePath;
    }
    
    return result;
}

Vector<String> PowerPointExporter::GetSupportedExtensions() const {
    Vector<String> extensions;
    extensions.Add(".ppt");
    extensions.Add(".pptx");
    return extensions;
}

String PowerPointExporter::GetFormatDescription() const {
    return "PowerPoint Presentation";
}

bool PowerPointExporter::SaveAsPowerPoint(const Project& project, const String& filePath) {
    // For a full project export to PowerPoint, we would create multiple slides
    // For now, we'll just return false as this requires more complex implementation
    // In a real application, you might use a library like libpptx or COM automation
    
    return false;
}

bool PowerPointExporter::SaveDiagramAsPowerPoint(const Image& diagramImage, const String& filePath, 
                                               const String& title) {
    // This would require a PowerPoint library
    // For now, we'll just save as PNG with a .ppt extension as a placeholder
    // In a real application, you would use a proper PowerPoint library
    
    try {
        SaveFile(filePath, diagramImage);
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

// JSONExporter implementation
JSONExporter::JSONExporter() {}

JSONExporter::~JSONExporter() {}

ExportResult JSONExporter::Export(const Project& project, const String& filePath) {
    ExportResult result;
    
    try {
        if (SaveAsJSON(project, filePath)) {
            result.success = true;
            result.message = "Project exported to JSON successfully";
            result.filePath = filePath;
        } else {
            result.success = false;
            result.message = "Failed to save JSON file";
            result.filePath = filePath;
        }
    } catch (const std::exception& e) {
        result.success = false;
        result.message = e.what();
        result.filePath = filePath;
    }
    
    return result;
}

ExportResult JSONExporter::Export(const PertDiagram& diagram, const String& filePath) {
    // Export the project data (diagram is just a view)
    return Export(diagram.GetProject(), filePath);
}

ExportResult JSONExporter::Export(const GanttDiagram& diagram, const String& filePath) {
    // Export the project data (diagram is just a view)
    return Export(diagram.GetProject(), filePath);
}

Vector<String> JSONExporter::GetSupportedExtensions() const {
    Vector<String> extensions;
    extensions.Add(".json");
    return extensions;
}

String JSONExporter::GetFormatDescription() const {
    return "JSON Data File";
}

bool JSONExporter::SaveAsJSON(const Project& project, const String& filePath) {
    try {
        // Create JSON from project
        Json json;
        project.ToJson(json);
        
        // Save to file
        String content = json.ToString();
        SaveFile(filePath, content);
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

// ExportManager implementation
ExportManager::ExportManager() {}

ExportManager::~ExportManager() {}

ExportResult ExportManager::ExportToFile(const Project& project, const String& filePath, ExportFormat format) {
    Exporter* exporter = GetExporter(format);
    if (exporter) {
        return exporter->Export(project, filePath);
    }
    
    ExportResult result;
    result.success = false;
    result.message = "Unsupported export format";
    result.filePath = filePath;
    return result;
}

ExportResult ExportManager::ExportToFile(const PertDiagram& diagram, const String& filePath, ExportFormat format) {
    Exporter* exporter = GetExporter(format);
    if (exporter) {
        return exporter->Export(diagram, filePath);
    }
    
    ExportResult result;
    result.success = false;
    result.message = "Unsupported export format";
    result.filePath = filePath;
    return result;
}

ExportResult ExportManager::ExportToFile(const GanttDiagram& diagram, const String& filePath, ExportFormat format) {
    Exporter* exporter = GetExporter(format);
    if (exporter) {
        return exporter->Export(diagram, filePath);
    }
    
    ExportResult result;
    result.success = false;
    result.message = "Unsupported export format";
    result.filePath = filePath;
    return result;
}

Exporter* ExportManager::GetExporter(ExportFormat format) {
    switch (format) {
        case ExportFormat::PNG: return &pngExporter;
        case ExportFormat::QTF: return &qtfExporter;
        case ExportFormat::POWERPOINT: return &pptExporter;
        case ExportFormat::JSON: return &jsonExporter;
        default: return nullptr;
    }
}

Vector<ExportFormat> ExportManager::GetSupportedFormats() const {
    Vector<ExportFormat> formats;
    formats.Add(ExportFormat::PNG);
    formats.Add(ExportFormat::QTF);
    formats.Add(ExportFormat::POWERPOINT);
    formats.Add(ExportFormat::JSON);
    return formats;
}

ExportFormat ExportManager::GetFormatFromExtension(const String& extension) const {
    String ext = extension.ToLower();
    
    if (ext == ".png") return ExportFormat::PNG;
    if (ext == ".qtf" || ext == ".rtf") return ExportFormat::QTF;
    if (ext == ".ppt" || ext == ".pptx") return ExportFormat::POWERPOINT;
    if (ext == ".json") return ExportFormat::JSON;
    
    return ExportFormat::PNG; // Default
}

bool ExportManager::ShowExportDialog(const Project& project, const PertDiagram& pertDiagram, 
                                    const GanttDiagram& ganttDiagram) {
    // This would show a file dialog and export the selected format
    // For now, we'll just return false
    
    return false;
}

String ExportManager::GetDefaultExtension(ExportFormat format) const {
    switch (format) {
        case ExportFormat::PNG: return ".png";
        case ExportFormat::QTF: return ".qtf";
        case ExportFormat::POWERPOINT: return ".pptx";
        case ExportFormat::JSON: return ".json";
        default: return ".png";
    }
}

String ExportManager::GetFilterString(ExportFormat format) const {
    switch (format) {
        case ExportFormat::PNG: return "PNG Images (*.png)";
        case ExportFormat::QTF: return "QTF Documents (*.qtf, *.rtf)";
        case ExportFormat::POWERPOINT: return "PowerPoint Presentations (*.ppt, *.pptx)";
        case ExportFormat::JSON: return "JSON Files (*.json)";
        default: return "All Files (*.*)";
    }
}

// Singleton implementation
ExportManager ExportManagerSingleton::instance;

ExportManager& ExportManagerSingleton::GetInstance() {
    return instance;
}
