#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "System.h"
#include <QTreeWidget>
#include "propmodel.h"
#include "diagramview.h"
#include "runtimewindow.h"
#include "plotter.h"
#include "GA.h"
#include "logwindow.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    PropModel *propModel() {return propmodel;}
    void resetPropModel() { propmodel = nullptr ;}
    System *GetSystem() {return &system;}
    bool AddLink(const QString &LinkName, const QString &sourceblock, const QString &targetblock, const QString &type, Edge* edge);
    void PopulatePropertyTable(QuanSet* quanset);
    void RecreateGraphicItemsFromSystem();
    void RefreshTreeView();
    Plotter* Plot(CTimeSeries& plotitem);
    string CreateNewName(string type);
    DiagramView* GetDiagramView() { return dView; }
    bool Log(const QString &s);
    bool LogError(const QString &s);
    bool LogAddDelete(const QString &s);
    void LogAllSystemErrors();
private:
    logwindow *LogWindow;
    Ui::MainWindow *ui;
    System system;
    DiagramView* dView;
    bool Populate_TreeWidget();
    bool BuildObjectsToolBar();
    PropModel *propmodel = nullptr;
    void Populate_General_ToolBar();
    string maintemplatefilename;
    vector<string> addedtemplatefilenames;
    string entitiesfilename;
    RunTimeWindow *rtw = nullptr;
    void closeEvent (QCloseEvent *event) override;
    QString workingfolder;
    QModelIndex addParameterIndex(const QModelIndex &index = QModelIndex());
    QModelIndex tableitemrightckicked;
    QMenu *menu;
    CGA<System> *optimizer;
    QString timeseriestobeshown;
    QString filename="";
    void SetFileName(const QString &_filename);
private slots:
    void on_check_object_browser();
    void on_check_showlogwindow();
    void on_object_browser_closed(bool visible);
    void onaddblock();
    void onaddlink();
    void onaddsource();
    void onaddparameter();
    void onaddentity();
    void onaddobjectivefunction();
    void preparetreeviewMenu(const QPoint &pos);
    void onTreeSelectionChanged(QTreeWidgetItem *current);
    void onzoomin();
    void onzoomout();
    void onzoomall();
    void onsave();
    void onsaveas();
    void onopen();
    void onrunmodel();
    void onoptimize();
    void onAddItemThroughTreeViewRightClick();
    void tablePropShowContextMenu(const QPoint&);
    void addParameter(QAction* item);
    void loadnewtemplate();
    void addplugin();
    void showgraph();
    void onDeleteItem();
};

#endif // MAINWINDOW_H
