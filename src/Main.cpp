#include <CtrlLib/CtrlLib.h>
#include "UI/Dialogs/MainWindow.h"

using namespace Upp;

GUI_APP_MAIN
{
    // Set application name and organization
    SetAppName("PertGanttManager");
    SetAppOrganization("PertGantt");
    
    // Enable DPI awareness
    SetDPIAware();
    
    // Create and initialize main window
    MainWindow mainWindow;
    
    if (mainWindow.Initialize()) {
        // Run the application
        mainWindow.Run();
    } else {
        // Initialization failed
        Exclamation("Failed to initialize application. Please check your installation.");
    }
}
