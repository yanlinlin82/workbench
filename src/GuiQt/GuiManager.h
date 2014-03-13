

#ifndef __GUI_MANAGER_H__
#define __GUI_MANAGER_H__

/*LICENSE_START*/
/*
 *  Copyright 1995-2002 Washington University School of Medicine
 *
 *  http://brainmap.wustl.edu
 *
 *  This file is part of CARET.
 *
 *  CARET is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  CARET is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with CARET; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <set>
#include <vector>
#include <stdint.h>

#include <QMap>
#include <QObject>

#include "EventListenerInterface.h"
#include "ImageCaptureMethodEnum.h"
#include "SceneableInterface.h"
#include "WuQWebView.h"

class QAction;
class QDialog;
class QWebView;
class QWidget;
class MovieDialog;
namespace caret {
    
    class Brain;
    class BrainBrowserWindow;
    class BrowserTabContent;
    class BugReportDialog;
    class ChartableBrainordinateInterface;
    class ChartingDialog;
    class CursorManager;
    class CustomViewDialog;
    class ImageFile;
    class ImageCaptureDialog;
    class InformationDisplayDialog;
    class MapSettingsEditorDialog;
    class Model;
    class PreferencesDialog;
    class Scene;
    class SceneDialog;
    class SceneFile;
    class SelectionManager;
    class SurfacePropertiesEditorDialog;
    class TileTabsConfigurationDialog;
    
    /**
     * Manages the graphical user-interface.
     */
    class GuiManager : public QObject, public EventListenerInterface, public SceneableInterface {
        
        Q_OBJECT
        
    public:
        static GuiManager* get();
        
        static void createGuiManager();
        
        static void deleteGuiManager();
        
        void beep(const int32_t numTimesToBeep = 1);
        
        Brain* getBrain();
        
        int32_t getNumberOfOpenBrainBrowserWindows() const;
        
        BrainBrowserWindow* getActiveBrowserWindow() const;
        
        std::vector<BrainBrowserWindow*> getAllOpenBrainBrowserWindows() const;
        
        BrainBrowserWindow* getBrowserWindowByWindowIndex(const int32_t browserWindowIndex);
        
        bool allowBrainBrowserWindowToClose(BrainBrowserWindow* bbw,
                                            const int32_t numberOfOpenTabs);
        
        bool exitProgram(QWidget* parent);
        
        QString applicationName() const;
        
        //BrainOpenGL* getBrainOpenGL();
        
        BrowserTabContent* getBrowserTabContentForBrowserWindow(const int32_t browserWindowIndex,
                                                                const bool allowInvalidBrowserWindowIndex);
        
        Model* getModelInBrowserWindow(const int32_t browserWindowIndex);
        
        void receiveEvent(Event* event);

        const CursorManager* getCursorManager() const;
        
        QAction* getInformationDisplayDialogEnabledAction();
        
        QAction* getSceneDialogDisplayAction();
        
        void closeAllOtherWindows(BrainBrowserWindow* browserWindow);
        
        void closeOtherWindowsAndReturnTheirTabContent(BrainBrowserWindow* browserWindow,
                                                       std::vector<BrowserTabContent*>& tabContents);
        
        void processShowBugReportDialog(BrainBrowserWindow* browserWindow,
                                        const AString& openGLInformation);
        void processShowCustomViewDialog(BrainBrowserWindow* browserWindow);
        void processShowImageCaptureDialog(BrainBrowserWindow* browserWindow);
        void processShowMovieDialog(BrainBrowserWindow* browserWindow);
        void processShowPreferencesDialog(BrainBrowserWindow* browserWindow);
        void processShowInformationDisplayDialog(const bool forceDisplayOfDialog);
        void processShowTileTabsConfigurationDialog(BrainBrowserWindow* browserWindow);
                
        void processShowSceneDialog(BrainBrowserWindow* browserWindow);
        
        void processShowSurfacePropertiesEditorDialog(BrainBrowserWindow* browserWindow);
        
        void processShowSceneDialogAndScene(BrainBrowserWindow* browserWindow,
                                            SceneFile* sceneFile,
                                            Scene* scene);
        
        void processShowAllenDataBaseWebView(BrainBrowserWindow* browserWindow);
        void processShowConnectomeDataBaseWebView(BrainBrowserWindow* browserWindow);
        
        ChartingDialog *getChartingDialog(ChartableBrainordinateInterface *id);
        void updateAnimationStartTime(double value); 
        
        virtual SceneClass* saveToScene(const SceneAttributes* sceneAttributes,
                                        const AString& instanceName);
        
        virtual void restoreFromScene(const SceneAttributes* sceneAttributes,
                                      const SceneClass* sceneClass);
        
        AString getNameOfDataFileToOpenAfterStartup() const;

        void processIdentification(SelectionManager* selectionManager,
                                   QWidget* parentWidget);
		void updateMatrixViewDialogs();
        
    public slots:
        void processBringAllWindowsToFront();
        void processShowHelpOnlineWindow();
        void processShowSearchHelpOnlineWindow();
        void processShowInformationWindow();
        void processTileWindows();
        
        void showHideInfoWindowSelected(bool);
        
        void sceneDialogDisplayActionToggled(bool);
        
    private slots:
        void sceneDialogWasClosed();
        
    private:
        GuiManager(QObject* parent = 0);
        
        virtual ~GuiManager();
        
        
        GuiManager(const GuiManager&);
        GuiManager& operator=(const GuiManager&);
        
        BrainBrowserWindow* newBrainBrowserWindow(QWidget* parent,
                                                  BrowserTabContent* browserTabContent,
                                                  const bool createDefaultTabs);
        
        void reparentNonModalDialogs(BrainBrowserWindow* closingBrainBrowserWindow);
        
        void showHideSceneDialog(const bool status,
                                 BrainBrowserWindow* parentBrainBrowserWindow);
        
        void removeNonModalDialog(QWidget* dialog);
        
        /** One instance of the GuiManager */
        static GuiManager* singletonGuiManager;
        
        /** 
         * Contains pointers to Brain Browser windows
         * As BrainBrowser windows are closed, some of
         * the elements may be NULL.
         */
        std::vector<BrainBrowserWindow*> m_brainBrowserWindows;
        
        /** Name of application */
        QString nameOfApplication;
        
        /** Skips confirmation of browser window closing when all tabs are moved to one window */
        bool allowBrowserWindowsToCloseWithoutConfirmation;
        
        /* Performs OpenGL drawing commands */
        //BrainOpenGL* brainOpenGL;
        
        /* Editor for map settings. */
        std::set<MapSettingsEditorDialog*> m_mappingSettingsEditors;
        
        TileTabsConfigurationDialog* m_tileTabsConfigurationDialog;
        
        CustomViewDialog* m_customViewDialog;
        
        ImageCaptureDialog* imageCaptureDialog;

        MovieDialog* movieDialog;
        
        PreferencesDialog* preferencesDialog;       
        
        InformationDisplayDialog* m_informationDisplayDialog;
        
        SceneDialog* sceneDialog;
        
        QAction* m_sceneDialogDisplayAction;
        
        SurfacePropertiesEditorDialog* m_surfacePropertiesEditorDialog;
        
        WuQWebView* connectomeDatabaseWebView;
        
        CursorManager* cursorManager;
        
        QAction* m_informationDisplayDialogEnabledAction;
        
        BugReportDialog* m_bugReportDialog;
        
        /** 
         * Tracks non-modal dialogs that are created only one time
         * and may need to be reparented if the original parent, a
         * BrainBrowserWindow is closed in which case the dialog
         * is reparented to a different BrainBrowserWindow.
         */
        std::vector<QWidget*> nonModalDialogs;
        
        QMap<ChartableBrainordinateInterface *,ChartingDialog *> chartingDialogs;
        
        /**
         * If Workbench is started by double-clicking a data file in
         * the Mac OSX Finder, this will contain the name of the data
         * file.  When the event is received, Workbench has not yet
         * created windows.  After creating the first Browser Window,
         * the values of this string is requested, and if valid,
         * the data file is opened.
         */
        AString m_nameOfDataFileToOpenAfterStartup;
    };
    
#ifdef __GUI_MANAGER_DEFINE__
    GuiManager* GuiManager::singletonGuiManager = NULL;
#endif // __GUI_MANAGER_DEFINE__
}

#endif // __GUI_MANAGER_H__
