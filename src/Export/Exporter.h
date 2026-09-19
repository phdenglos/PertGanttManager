#ifndef EXPORTER_H
#define EXPORTER_H

#include <Core/Core.h>
#include <CtrlLib/CtrlLib.h>
#include <vector>
#include <memory>

using namespace Upp;

class Project;
class PertDiagram;
class GanttDiagram;

// Export format types
enum class ExportFormat {
    PNG,
    QTF,
    POWERPOINT,
    JSON
};

// Export result
struct ExportResult {
    bool success;
    String message;
    String filePath;
    
    ExportResult();
    void Clear();
};

// Base exporter class
class Exporter {
public:
    Exporter();
    virtual ~Exporter();
    
    // Export methods
    virtual ExportResult Export(const Project& project, const String& filePath) = 0;
    virtual ExportResult Export(const PertDiagram& diagram, const String& filePath) = 0;
    virtual ExportResult Export(const GanttDiagram& diagram, const String& filePath) = 0;
    
    // Get supported extensions
    virtual Vector<String> GetSupportedExtensions() const = 0;
    
    // Get format description
    virtual String GetFormatDescription() const = 0;
    
    // Set export parameters
    virtual void SetResolution(int dpi);
    virtual int GetResolution() const;
    
    virtual void SetQuality(int quality);
    virtual int GetQuality() const;
    
    virtual void SetCompression(bool enable);
    virtual bool GetCompression() const;
    
protected:
    int resolution;
    int quality;
    bool compression;
};

// PNG exporter
class PNGExporter : public Exporter {
public:
    PNGExporter();
    ~PNGExporter();
    
    // Export methods
    ExportResult Export(const Project& project, const String& filePath) override;
    ExportResult Export(const PertDiagram& diagram, const String& filePath) override;
    ExportResult Export(const GanttDiagram& diagram, const String& filePath) override;
    
    // Get supported extensions
    Vector<String> GetSupportedExtensions() const override;
    
    // Get format description
    String GetFormatDescription() const override;
    
private:
    // Helper methods
    bool SaveImageToPNG(const Image& image, const String& filePath);
};

// QTF exporter (for rich text documents)
class QTFExporter : public Exporter {
public:
    QTFExporter();
    ~QTFExporter();
    
    // Export methods
    ExportResult Export(const Project& project, const String& filePath) override;
    ExportResult Export(const PertDiagram& diagram, const String& filePath) override;
    ExportResult Export(const GanttDiagram& diagram, const String& filePath) override;
    
    // Get supported extensions
    Vector<String> GetSupportedExtensions() const override;
    
    // Get format description
    String GetFormatDescription() const override;
    
private:
    // Helper methods
    String CreateQTFDocument(const Project& project, const Image& diagramImage);
    String CreateQTFDocument(const PertDiagram& diagram);
    String CreateQTFDocument(const GanttDiagram& diagram);
};

// PowerPoint exporter
class PowerPointExporter : public Exporter {
public:
    PowerPointExporter();
    ~PowerPointExporter();
    
    // Export methods
    ExportResult Export(const Project& project, const String& filePath) override;
    ExportResult Export(const PertDiagram& diagram, const String& filePath) override;
    ExportResult Export(const GanttDiagram& diagram, const String& filePath) override;
    
    // Get supported extensions
    Vector<String> GetSupportedExtensions() const override;
    
    // Get format description
    String GetFormatDescription() const override;
    
private:
    // Helper methods
    bool SaveAsPowerPoint(const Project& project, const String& filePath);
    bool SaveDiagramAsPowerPoint(const Image& diagramImage, const String& filePath, 
                               const String& title);
};

// JSON exporter
class JSONExporter : public Exporter {
public:
    JSONExporter();
    ~JSONExporter();
    
    // Export methods
    ExportResult Export(const Project& project, const String& filePath) override;
    ExportResult Export(const PertDiagram& diagram, const String& filePath) override;
    ExportResult Export(const GanttDiagram& diagram, const String& filePath) override;
    
    // Get supported extensions
    Vector<String> GetSupportedExtensions() const override;
    
    // Get format description
    String GetFormatDescription() const override;
    
private:
    // Helper methods
    bool SaveAsJSON(const Project& project, const String& filePath);
};

// Export manager
class ExportManager {
public:
    ExportManager();
    ~ExportManager();
    
    // Export to file
    ExportResult ExportToFile(const Project& project, const String& filePath, ExportFormat format);
    ExportResult ExportToFile(const PertDiagram& diagram, const String& filePath, ExportFormat format);
    ExportResult ExportToFile(const GanttDiagram& diagram, const String& filePath, ExportFormat format);
    
    // Get exporter for format
    Exporter* GetExporter(ExportFormat format);
    
    // Get all supported formats
    Vector<ExportFormat> GetSupportedFormats() const;
    
    // Get format from file extension
    ExportFormat GetFormatFromExtension(const String& extension) const;
    
    // Show export dialog
    bool ShowExportDialog(const Project& project, const PertDiagram& pertDiagram, 
                         const GanttDiagram& ganttDiagram);
    
private:
    // Exporters
    PNGExporter pngExporter;
    QTFExporter qtfExporter;
    PowerPointExporter pptExporter;
    JSONExporter jsonExporter;
    
    // Helper methods
    String GetDefaultExtension(ExportFormat format) const;
    String GetFilterString(ExportFormat format) const;
};

// Singleton export manager
class ExportManagerSingleton {
public:
    static ExportManager& GetInstance();
    
private:
    static ExportManager instance;
};

#endif // EXPORTER_H
