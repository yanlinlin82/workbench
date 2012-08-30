

#include <iostream>

#include <QAction>
#include <QLayout>
#include <QScrollArea>
#include <QToolBox>
#include <QTabWidget>

#include "BorderSelectionViewController.h"
#include "BrainBrowserWindow.h"
#include "BrainBrowserWindowOrientedToolBox.h"
#include "CaretAssert.h"
#include "CaretPreferences.h"
#include "ConnectivityManagerViewController.h"
#include "FociSelectionViewController.h"
#include "GuiManager.h"
#include "LabelSelectionViewController.h"
#include "OverlaySetViewController.h"
#include "SceneClass.h"
#include "SceneWindowGeometry.h"
#include "SessionManager.h"
#include "VolumeSurfaceOutlineSetViewController.h"
#include "WuQtUtilities.h"

using namespace caret;

/**
 * Construct the toolbox.
 * 
 * @param browserWindowIndex
 *    Index of browser window that contains this toolbox.
 * @param title
 *    Title for the toolbox.
 * @param location
 *    Locations allowed for this toolbox.
 */
BrainBrowserWindowOrientedToolBox::BrainBrowserWindowOrientedToolBox(const int32_t browserWindowIndex,
                                                                     const QString& title,
                                                                     const ToolBoxType toolBoxType,
                                                                     QWidget* parent)
:   QDockWidget(parent)
{
    m_browserWindowIndex = browserWindowIndex;
    
    toggleViewAction()->setText("Toolbox");
    
    m_toolBoxTitle = title;
    setWindowTitle(m_toolBoxTitle);
    
    bool isFeaturesToolBox  = false;
    bool isOverlayToolBox = false;
    Qt::Orientation orientation = Qt::Horizontal;
    switch (toolBoxType) {
        case TOOL_BOX_FEATURES:
            orientation = Qt::Vertical;
            isFeaturesToolBox = true;
            toggleViewAction()->setText("Features Toolbox");
            break;
        case TOOL_BOX_OVERLAYS_HORIZONTAL:
            orientation = Qt::Horizontal;
            isOverlayToolBox = true;
            break;
        case TOOL_BOX_OVERLAYS_VERTICAL:
            orientation = Qt::Vertical;
            isOverlayToolBox = true;
            break;
    }
    
    m_borderSelectionViewController = NULL;
    m_connectivityViewController = NULL;
    m_fociSelectionViewController = NULL;
    m_labelSelectionViewController = NULL;
    m_overlaySetViewController = NULL;
    m_timeSeriesViewController = NULL;
    m_volumeSurfaceOutlineSetViewController = NULL;

    m_tabWidget = new QTabWidget();
    
    if (isOverlayToolBox) {
        m_overlaySetViewController = new OverlaySetViewController(orientation,
                                                                      browserWindowIndex,
                                                                      this);  
//        m_tabWidget->addTab(m_overlaySetViewController ,
//                                "Layers");
        addToTabWidget(m_overlaySetViewController,
                       "Layers");
    }
    if (isOverlayToolBox) {
        m_connectivityViewController = new ConnectivityManagerViewController(orientation,
                                                                                 browserWindowIndex,
                                                                                 DataFileTypeEnum::CONNECTIVITY_DENSE);
        addToTabWidget(m_connectivityViewController, 
                             "Connectivity");
    }
    if (isOverlayToolBox) {
        m_timeSeriesViewController = new ConnectivityManagerViewController(orientation,
                                                                               browserWindowIndex,
                                                                               DataFileTypeEnum::CONNECTIVITY_DENSE_TIME_SERIES);
        addToTabWidget(m_timeSeriesViewController, 
                             "Data Series");
    }
    if (isFeaturesToolBox) {
        m_borderSelectionViewController = new BorderSelectionViewController(browserWindowIndex,
                                                                                this);
        addToTabWidget(m_borderSelectionViewController, 
                             "Borders");
    }
    
    if (isFeaturesToolBox) {
        m_fociSelectionViewController = new FociSelectionViewController(browserWindowIndex,
                                                                                this);
        addToTabWidget(m_fociSelectionViewController, 
                             "Foci");
    }
    
    if (isFeaturesToolBox) {
        m_labelSelectionViewController = new LabelSelectionViewController(browserWindowIndex,
                                                                          this);
        addToTabWidget(m_labelSelectionViewController,
                       "Labels");
    }
    
    if (isOverlayToolBox) {
        m_volumeSurfaceOutlineSetViewController = new VolumeSurfaceOutlineSetViewController(orientation,
                                                                                                m_browserWindowIndex);
        addToTabWidget(m_volumeSurfaceOutlineSetViewController, 
                             "Vol/Surf Outline");
    }
    
    /*
     * Layout the widgets
     */
//    QWidget* widget = new QWidget();
//    QVBoxLayout* layout = new QVBoxLayout(widget);
//    WuQtUtilities::setLayoutMargins(layout, 0, 0);
//    layout->addWidget(tabBar);
//    layout->addWidget(scrollArea);
    
    setWidget(m_tabWidget);

    if (orientation == Qt::Horizontal) {
        setMinimumHeight(200);
        setMaximumHeight(800);
    }
    else {
        if (isOverlayToolBox) {
            setMinimumWidth(300);
            setMaximumWidth(800);
        }
        else {
            setMinimumWidth(200);
            setMaximumWidth(800);
        }
    }

    QObject::connect(this, SIGNAL(topLevelChanged(bool)),
                     this, SLOT(floatingStatusChanged(bool)));
}

/**
 * Destructor.
 */
BrainBrowserWindowOrientedToolBox::~BrainBrowserWindowOrientedToolBox()
{
}

/**
 * Place widget into a scroll area and then into the tab widget.
 * @param page
 *    Widget that is added.
 * @param label
 *    Name corresponding to widget's tab.
 */
int 
BrainBrowserWindowOrientedToolBox::addToTabWidget(QWidget* page,
                                                  const QString& label)
{
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidget(page);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    int indx = m_tabWidget->addTab(scrollArea,
                                       label);
    return indx;
}


/**
 * Called when floating status changes.
 * @param status
 *   New floating status.
 */
void 
BrainBrowserWindowOrientedToolBox::floatingStatusChanged(bool /*status*/)
{
    QString title = m_toolBoxTitle;
//    if (status) {
//        title += (" "
//                  + QString::number(m_browserWindowIndex + 1));
//    }
    setWindowTitle(title);
}

/**
 * Create a scene for an instance of a class.
 *
 * @param sceneAttributes
 *    Attributes for the scene.  Scenes may be of different types
 *    (full, generic, etc) and the attributes should be checked when
 *    saving the scene.
 *
 * @return Pointer to SceneClass object representing the state of 
 *    this object.  Under some circumstances a NULL pointer may be
 *    returned.  Caller will take ownership of returned object.
 */
SceneClass* 
BrainBrowserWindowOrientedToolBox::saveToScene(const SceneAttributes* sceneAttributes,
                                const AString& instanceName)
{
    SceneClass* sceneClass = new SceneClass(instanceName,
                                            "BrainBrowserWindowOrientedToolBox",
                                            1);
    
    AString tabName;
    const int tabIndex = m_tabWidget->currentIndex();
    if ((tabIndex >= 0) 
        && tabIndex < m_tabWidget->count()) {
        tabName = m_tabWidget->tabText(tabIndex);
    }
    sceneClass->addString("selectedTabName",
                          tabName);
    
    /*
     * Save current widget size
     */
    QWidget* childWidget = m_tabWidget->currentWidget();
    if (childWidget != NULL) {
        SceneWindowGeometry swg(childWidget,
                                this);
        sceneClass->addClass(swg.saveToScene(sceneAttributes,
                                             "childWidget"));
    }
    
    /*
     * Save the toolbox
     */
    SceneWindowGeometry swg(this,
                            GuiManager::get()->getBrowserWindowByWindowIndex(this->m_browserWindowIndex));
    sceneClass->addClass(swg.saveToScene(sceneAttributes,
                                         "geometry"));
    
////    if (isFloating()) {
//        /*
//         * Position and size
//         */
////        SceneWindowGeometry swg(this,
////                                GuiManager::get()->getBrowserWindowByWindowIndex(this->m_browserWindowIndex));
//    SceneWindowGeometry swg(this->m_tabWidget->currentWidget(),
//                            GuiManager::get()->getBrowserWindowByWindowIndex(this->m_browserWindowIndex));
//        sceneClass->addClass(swg.saveToScene(sceneAttributes,
//                                             "geometry"));
////    }
    
    return sceneClass;
}

/**
 * Restore the state of an instance of a class.
 * 
 * @param sceneAttributes
 *    Attributes for the scene.  Scenes may be of different types
 *    (full, generic, etc) and the attributes should be checked when
 *    restoring the scene.
 *
 * @param sceneClass
 *     SceneClass containing the state that was previously 
 *     saved and should be restored.
 */
void 
BrainBrowserWindowOrientedToolBox::restoreFromScene(const SceneAttributes* sceneAttributes,
                                     const SceneClass* sceneClass)
{
    if (sceneClass == NULL) {
        return;
    }
    
    const AString tabName = sceneClass->getStringValue("selectedTabName",
                                                       "");
    for (int32_t i = 0; i < m_tabWidget->count(); i++) {
        if (m_tabWidget->tabText(i) == tabName) {
            m_tabWidget->setCurrentIndex(i);
            break;
        }
    }
    
    /*
     * Restore current widget size
     */
    QWidget* childWidget = m_tabWidget->currentWidget();
    QSize childMinSize;
    QSize childSize;
    if (childWidget != NULL) {
        childMinSize = childWidget->minimumSize();
        SceneWindowGeometry swg(childWidget,
                                this);
        swg.restoreFromScene(sceneAttributes,
                             sceneClass->getClass("childWidget"));
        childSize = childWidget->size();
    }
    
    if (isFloating() && isVisible()) {
        SceneWindowGeometry swg(this,
                                GuiManager::get()->getBrowserWindowByWindowIndex(this->m_browserWindowIndex));
        swg.restoreFromScene(sceneAttributes,
                             sceneClass->getClass("geometry"));
    }
    else {
//        if (childWidget != NULL) {
//            childWidget->setMinimumSize(childSize);
//            SceneWindowGeometry swg(this,
//                                    GuiManager::get()->getBrowserWindowByWindowIndex(this->m_browserWindowIndex));
//            swg.restoreFromScene(sceneAttributes,
//                                 sceneClass->getClass("geometry"));
//            childWidget->setMinimumSize(childMinSize);
//        }
    }
//    if (isVisible()) {
//        SceneWindowGeometry swg(this,
//                                GuiManager::get()->getBrowserWindowByWindowIndex(this->m_browserWindowIndex));
//        swg.restoreFromScene(sceneAttributes,
//                             sceneClass->getClass("geometry"));
//    }
    
//    //if (isFloating() && isVisible()) {
//    if (isVisible()) {
//        /*
//         * Position and size
//         */
//        SceneWindowGeometry swg(this->m_tabWidget->currentWidget(),
//                                GuiManager::get()->getBrowserWindowByWindowIndex(this->m_browserWindowIndex));
////        SceneWindowGeometry swg(this,
////                                GuiManager::get()->getBrowserWindowByWindowIndex(this->m_browserWindowIndex));
//        if (isFloating() == false) {
//            swg.setWidthHeightOnly(true);
//        }
//        swg.restoreFromScene(sceneAttributes, sceneClass->getClass("geometry"));
//    }
}



