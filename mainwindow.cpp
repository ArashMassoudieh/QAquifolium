#define openhydroqual_version "1.0.5"
#define last_modified "May 27, 2021"

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#pragma comment(lib, "shell32.lib")
#endif

#define RECENT "recentFiles.txt"

#ifndef max_num_recent_files
#define max_num_recent_files 15
#endif

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <string>
#include <QDebug>
#include "enums.h"
#include "propmodel.h"
#include "delegate.h"
#include "node.h"
#include "edge.h"
#include "Script.h"
#include "QFileDialog"
#include "runtimewindow.h"
#include "plotter.h"
#include <QInputDialog>
#include "wizard_select_dialog.h"
#include <QtSvg/QGraphicsSvgItem>
#include <QtSvg/QSvgGenerator>
#include <QtSvg/QSvgRenderer>
#include "options.h"

using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QIcon mainicon(qApp->applicationDirPath() + "/../../resources/Icons/Aquifolium.png");
    setWindowIcon(mainicon);
    dView = new DiagramView(ui->centralWidget,this);
    dView->setObjectName(QStringLiteral("graphicsView"));
    ui->horizontalLayout->addWidget(dView);
    LogWindow = new logwindow(this);
    LogWindow->show();
#ifndef Win_Version
    maintemplatefilename = qApp->applicationDirPath().toStdString() + "/../../resources/main_components.json";
    entitiesfilename = qApp->applicationDirPath().toStdString() + "/../../resources/settings.json";
	system.DefaultTemplatePath() = qApp->applicationDirPath().toStdString() + "/../../resources/";
#else
    maintemplatefilename = qApp->applicationDirPath().toStdString() + "/resources/main_components.json";
    entitiesfilename = qApp->applicationDirPath().toStdString() + "/resources/settings.json";
	system.DefaultTemplatePath() = qApp->applicationDirPath().toStdString() + "/resources/";

#endif // !Win_Version
    Log("Default Template Location is set to '" + QString::fromStdString(system.DefaultTemplatePath()) + "'");
    if (system.GetQuanTemplate(maintemplatefilename)) //Read the template from modelfilename
    {
        Log("Template was successfully loaded from '" + QString::fromStdString(maintemplatefilename) + "'");
    }
    else {
        LogError("Template" + QString::fromStdString(maintemplatefilename) + "' was not loaded properly");
    }
    if (system.ReadSystemSettingsTemplate(entitiesfilename)) //Read the system settings
    {
        Log("Setting was successfully loaded from '" + QString::fromStdString(entitiesfilename) + "'");
    }
    else
    {
        LogError("Failed to load the setting file '" + QString::fromStdString(entitiesfilename) + "'");
    }
    RefreshTreeView();
    //connect(ui->treeWidget, SIGNAL(closeEvent()),ui->actionObject_Browser, SLOT())

    connect(ui->actionObject_Browser,SIGNAL(triggered()),this,SLOT(on_check_object_browser()));
    connect(ui->actionLog_Window,SIGNAL(triggered()),this,SLOT(on_check_showlogwindow()));
    connect(ui->dockWidget_3,SIGNAL(visibilityChanged(bool)),this,SLOT(on_object_browser_closed(bool)));
    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    BuildObjectsToolBar();
    connect(ui->treeWidget,SIGNAL(customContextMenuRequested(const QPoint&)),this,SLOT(preparetreeviewMenu(const QPoint&)));
    connect(ui->treeWidget,SIGNAL(itemClicked(QTreeWidgetItem*, int)),this,SLOT(onTreeSelectionChanged(QTreeWidgetItem*)));

    connect(ui->actionOpen,SIGNAL(triggered()),this,SLOT(onopen()));
    connect(ui->actionSave,SIGNAL(triggered()),this,SLOT(onsave()));
    connect(ui->actionSave_as,SIGNAL(triggered()),this,SLOT(onsaveas()));
    connect(ui->actionExport_to_SVG,SIGNAL(triggered()),this,SLOT(onexporttosvg()));
    connect(ui->actionNew_Project,SIGNAL(triggered()),this,SLOT(onnew()));
    connect(ui->actionAbout,SIGNAL(triggered()),this,SLOT(onabout()));
    connect(this,SIGNAL(closed()),this,SLOT(onclosed()));
    connect(ui->actionLoad_a_new_template,SIGNAL(triggered()),this,SLOT(loadnewtemplate()));
    connect(ui->actionAddPlugin,SIGNAL(triggered()),this,SLOT(addplugin()));
    connect(ui->actionAdd_plugin,SIGNAL(triggered()),this,SLOT(adddefaultpluging()));
    connect(ui->actionOptions,SIGNAL(triggered()),this,SLOT(optionsdialog()));
    ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->tableView, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(tablePropShowContextMenu(const QPoint&)));

    ui->tableView->setItemDelegateForColumn(1,new Delegate(this,this));
    Populate_General_ToolBar();

}

MainWindow::~MainWindow()
{
    if (rtw)
        rtw->close();
    LogWindow->close();
    delete ui;
    exit; 
}

void MainWindow::tablePropShowContextMenu(const QPoint&pos)
{
    QModelIndex i1 = ui->tableView->indexAt(pos);
    int row = i1.row();
    QModelIndex i2 = i1.sibling(row, 1);
    tableitemrightckicked = i2;
    if (i1.column() == 0)
    {
        menu = new QMenu;

        QMenu *estimatesMenu = new QMenu("Parameters");
        menu->addMenu(estimatesMenu);
        estimatesMenu->setEnabled(false);
        if (i2.data(CustomRoleCodes::Role::EstimateCode).toBool())
        {
            for (unsigned int i=0 ; i< GetSystem()->ParametersCount(); i++)
                estimatesMenu->addAction(QString::fromStdString(GetSystem()->Parameters()[i]->GetName()));// , this, SLOT(addParameter()));
            //addParameterIndex(i1); // tableProp->indexAt(pos));
            connect(estimatesMenu, SIGNAL(triggered(QAction*)), this, SLOT(addParameter(QAction*)));
            estimatesMenu->setEnabled(true);
        }

        menu->exec(ui->tableView->mapToGlobal(pos));
    }
    if (i1.column() == 1)
    {
        menu = new QMenu;

        if (ui->tableView->model()->data(tableitemrightckicked,CustomRoleCodes::TypeRole).toString().contains("ComboBox"))
        {
            QAction* action = menu->addAction("Clear");
            connect(action,SIGNAL(triggered()),this, SLOT(clearcombobox()));
            menu->exec(ui->tableView->mapToGlobal(pos));
        }
        if (ui->tableView->model()->data(tableitemrightckicked,CustomRoleCodes::TypeRole).toString().contains("DateTime"))
        {
            QAction* action = menu->addAction("Insert in numeric format");
            connect(action,SIGNAL(triggered()),this, SLOT(insertnumberasdate()));
            menu->exec(ui->tableView->mapToGlobal(pos));
        }
        if (ui->tableView->model()->data(tableitemrightckicked,CustomRoleCodes::TypeRole).toString().contains("Browser"))
        {
            QAction* action = menu->addAction("Plot");
            connect(action,SIGNAL(triggered()),this, SLOT(PlotTimeSeries()));
            menu->exec(ui->tableView->mapToGlobal(pos));
        }
    }
}

void MainWindow::clearcombobox()
{
    ui->tableView->model()->setData(tableitemrightckicked, "");
}

void MainWindow::insertnumberasdate()
{
    double x = QInputDialog::getDouble(this,"Date/Time Value: ","Date/Time value", ui->tableView->model()->data(tableitemrightckicked).toDouble(),0,20000,2);
    ui->tableView->model()->setData(tableitemrightckicked, QString::number(x));
}

void MainWindow::PlotTimeSeries()
{
    QString timeseriesfilename = ui->tableView->model()->data(tableitemrightckicked,CustomRoleCodes::fullFileNameRole).toString();
    QString objectname = ui->tableView->model()->data(tableitemrightckicked,CustomRoleCodes::ObjectName).toString();
    QString varname = ui->tableView->model()->data(tableitemrightckicked,CustomRoleCodes::VariableName).toString();

    if (GetSystem()->object(objectname.toStdString())->Variable(varname.toStdString())->GetTimeSeries() != nullptr)
    {
        Plotter* plot = Plot(*GetSystem()->object(objectname.toStdString())->Variable(varname.toStdString())->GetTimeSeries());
        plot->SetYAxisTitle(varname);
    }



}

void MainWindow::addParameter(QAction* item)
{
    QString parameter = item->text();
    ui->tableView->model()->setData(tableitemrightckicked, parameter, CustomRoleCodes::setParamRole);
    menu->hide();
    menu->setVisible(false);

}

QModelIndex MainWindow::addParameterIndex(const QModelIndex &index)
{
    static QModelIndex parameterIndex;
    if (index != QModelIndex()) parameterIndex = index;
    return parameterIndex;
}



bool MainWindow::Populate_TreeWidget()
{
    ui->treeWidget->clear();
    QStringList treeitems = system.QGetAllCategoryTypes();
    for (int i=0; i<treeitems.count(); i++)
    {
        QTreeWidgetItem *Item = new QTreeWidgetItem(ui->treeWidget);
        QTreeWidgetItem(ui->treeWidget);
        Item->setText(0,treeitems[i]);
        Item->setData(0,Qt::UserRole,"main");
        ui->treeWidget->addTopLevelItem(Item);
    }
    return true;
}

void MainWindow::on_check_object_browser()
{
    //ui->actionObject_Browser->setChecked(!ui->actionObject_Browser->isChecked());
    if (ui->actionObject_Browser->isChecked())
        ui->dockWidget_3->show();
    else
        ui->dockWidget_3->hide();

}

void MainWindow::on_check_showlogwindow()
{
    //ui->actionObject_Browser->setChecked(!ui->actionObject_Browser->isChecked());
    if (ui->actionLog_Window->isChecked())
        LogWindow->show();
    else
        LogWindow->hide();

}

void MainWindow::on_object_browser_closed(bool visible)
{
    ui->actionObject_Browser->setChecked(visible);
}

bool MainWindow::BuildObjectsToolBar()
{
    ui->mainToolBar->clear();
    for (unsigned int i = 0; i < system.GetAllBlockTypes().size(); i++)
    {
        //qDebug() << QString::fromStdString(system.GetAllBlockTypes()[i]);
        QAction* action = new QAction(this);
        action->setObjectName(QString::fromStdString(system.GetAllBlockTypes()[i]));
        QIcon icon;
        if (QString::fromStdString(system.GetModel(system.GetAllBlockTypes()[i])->IconFileName()).contains("/"))
        {
            if (!QFile::exists(QString::fromStdString(system.GetModel(system.GetAllBlockTypes()[i])->IconFileName())))
                LogError("Icon file '" + QString::fromStdString(system.GetModel(system.GetAllBlockTypes()[i])->IconFileName()) + "' was not found!");
            else
                icon.addFile(QString::fromStdString(system.GetModel(system.GetAllBlockTypes()[i])->IconFileName()), QSize(), QIcon::Normal, QIcon::Off);
        }
        else
        {
            if (!QFile::exists(QString::fromStdString(qApp->applicationDirPath().toStdString() + "/../../resources/Icons/" + system.GetModel(system.GetAllBlockTypes()[i])->IconFileName())))
                LogError("Icon file '" + QString::fromStdString(qApp->applicationDirPath().toStdString() + "/../../resources/Icons/" + system.GetModel(system.GetAllBlockTypes()[i])->IconFileName()) + "' was not found!");
            else
                icon.addFile(QString::fromStdString(qApp->applicationDirPath().toStdString() + "/../../resources/Icons/" + system.GetModel(system.GetAllBlockTypes()[i])->IconFileName()), QSize(), QIcon::Normal, QIcon::Off);

        }
        action->setIcon(icon);
        action->setToolTip(QString::fromStdString(system.GetModel(system.GetAllBlockTypes()[i])->Description()));
        ui->mainToolBar->addAction(action);
        action->setText(QString::fromStdString(system.GetAllBlockTypes()[i]));
        connect(action, SIGNAL(triggered()), this, SLOT(onaddblock()));
    }
    ui->mainToolBar->addSeparator();
    for (unsigned int i = 0; i < system.GetAllLinkTypes().size(); i++)
    {
        //qDebug() << QString::fromStdString(system.GetAllLinkTypes()[i]);
        QAction* action = new QAction(this);
        action->setCheckable(true);
        action->setObjectName(QString::fromStdString(system.GetAllLinkTypes()[i]));
        QIcon icon;
        if (QString::fromStdString(system.GetModel(system.GetAllLinkTypes()[i])->IconFileName()).contains("/"))
        {
            if (!QFile::exists(QString::fromStdString(system.GetModel(system.GetAllLinkTypes()[i])->IconFileName())))
                LogError("Icon file '" + QString::fromStdString(system.GetModel(system.GetAllLinkTypes()[i])->IconFileName()) + "' was not found!");
            else
                icon.addFile(QString::fromStdString(system.GetModel(system.GetAllLinkTypes()[i])->IconFileName()), QSize(), QIcon::Normal, QIcon::Off);
        }
        else
        {
            if (!QFile::exists(QString::fromStdString(qApp->applicationDirPath().toStdString() + "/../../resources/Icons/" + system.GetModel(system.GetAllLinkTypes()[i])->IconFileName())))
                LogError("Icon file '" + QString::fromStdString(qApp->applicationDirPath().toStdString() + "/../../resources/Icons/" + system.GetModel(system.GetAllLinkTypes()[i])->IconFileName()) + "' was not found!");
            else
                icon.addFile(QString::fromStdString(qApp->applicationDirPath().toStdString() + "/../../resources/Icons/" + system.GetModel(system.GetAllLinkTypes()[i])->IconFileName()), QSize(), QIcon::Normal, QIcon::Off);

        }
        action->setIcon(icon);
        action->setToolTip(QString::fromStdString(system.GetModel(system.GetAllLinkTypes()[i])->Description()));
        ui->mainToolBar->addAction(action);
        action->setText(QString::fromStdString(system.GetAllLinkTypes()[i]));
        connect(action, SIGNAL(triggered()), this, SLOT(onaddlink()));
    }
    ui->mainToolBar->addSeparator();
    for (unsigned int j = 0; j < system.QGetAllCategoryTypes().size(); j++)
    {
        string typecategory = system.QGetAllCategoryTypes()[j].toStdString();

        if (typecategory!="Blocks" && typecategory !="Connectors" && typecategory!="Settings")
            for (unsigned int i = 0; i < system.GetAllTypesOf(typecategory).size(); i++)
            {
                string type = system.GetAllTypesOf(typecategory)[i];
                QAction* action = new QAction(this);
                action->setCheckable(false);
                action->setObjectName(QString::fromStdString(type));
                QIcon icon;
                icon.addFile(QString::fromStdString(qApp->applicationDirPath().toStdString() + "/../../resources/Icons/" + system.GetModel(type)->IconFileName()), QSize(), QIcon::Normal, QIcon::Off);
                if (QString::fromStdString(system.GetModel(type)->IconFileName()).contains("/"))
                {
                    if (!QFile::exists(QString::fromStdString(system.GetModel(type)->IconFileName())))
                        LogError("Icon file '" + QString::fromStdString(system.GetModel(type)->IconFileName()) + "' was not found!");
                    else
                        icon.addFile(QString::fromStdString(system.GetModel(type)->IconFileName()), QSize(), QIcon::Normal, QIcon::Off);
                }
                else
                {
                    if (!QFile::exists(QString::fromStdString(qApp->applicationDirPath().toStdString() + "/../../resources/Icons/" + system.GetModel(type)->IconFileName())))
                        LogError("Icon file '" + QString::fromStdString(qApp->applicationDirPath().toStdString() + "/../../resources/Icons/" + system.GetModel(type)->IconFileName()) + "' was not found!");
                    else
                        icon.addFile(QString::fromStdString(qApp->applicationDirPath().toStdString() + "/../../resources/Icons/" + system.GetModel(type)->IconFileName()), QSize(), QIcon::Normal, QIcon::Off);
                }


                action->setIcon(icon);
                action->setToolTip(QString::fromStdString(system.GetModel(type)->Description()));
                ui->mainToolBar->addAction(action);
                action->setText(QString::fromStdString(type));
                if (typecategory=="Sources")
                    connect(action, SIGNAL(triggered()), this, SLOT(onaddsource()));
                else if (typecategory == "Parameters")
                    connect(action, SIGNAL(triggered()), this, SLOT(onaddparameter()));
                else if (typecategory == "Objective Functions")
                    connect(action, SIGNAL(triggered()), this, SLOT(onaddobjectivefunction()));
                else if (typecategory == "Observations")
                    connect(action, SIGNAL(triggered()), this, SLOT(onaddobservation()));
                else if (typecategory == "Constituents")
                    connect(action, SIGNAL(triggered()), this, SLOT(onaddconstituent()));
                else if (typecategory == "Reactions")
                    connect(action, SIGNAL(triggered()), this, SLOT(onaddreaction()));
                else if (typecategory == "Reaction Parameters")
                    connect(action, SIGNAL(triggered()), this, SLOT(onaddreactionparameter()));
                else
                    connect(action, SIGNAL(triggered()), this, SLOT(onaddentity()));
            }
        ui->mainToolBar->addSeparator();
    }

    return true;
}


void MainWindow::onaddblock()
{
    
    QObject* obj = sender();
    Block block;
    //qDebug() << "Setting Quantities";
    if (!block.SetQuantities(system.GetMetaModel(),obj->objectName().toStdString()))
    {
        LogError(QString::fromStdString(block.lasterror()));
        return;
    }
    //qDebug() << "Quantities Set";
    block.SetType(obj->objectName().toStdString());
    string name = CreateNewName(obj->objectName().toStdString());
    block.SetName(name);
    //qDebug() << "Adding Block to the system";
    system.AddBlock(block);
    system.object(name)->SetName(name);
    Node *node = new Node(dView,&system);
    //qDebug() << "Node Created!";
    dView->repaint();
    //qDebug() << "DiagramView Repainted!";
    system.object(name)->AssignRandomPrimaryKey();
    system.AddAllConstituentRelateProperties(system.block(name));
    system.SetVariableParents();
    node->SetObject(system.object(name));
    RefreshTreeView();
    LogAddDelete("Block '" + QString::fromStdString(name) + "' was added!");

 }

void MainWindow::onaddlink()
{
    QObject* obj = sender();
    dView->setconnectfeature(obj->objectName());
    foreach (QAction* action, ui->mainToolBar->actions())
    {
        if (action->objectName()!=obj->objectName())
            action->setChecked(false);
        else
            action->setChecked(true);
    }

}

bool MainWindow::AddLink(const QString &LinkName, const QString &sourceblock, const QString &targetblock, const QString &type,  Edge* edge)
{
    Link link;
    if (!link.SetQuantities(system.GetMetaModel(),type.toStdString()))
    {
        LogError(QString::fromStdString(link.lasterror()));
        LogError(QString::fromStdString(system.lasterror()));
        //RecreateGraphicItemsFromSystem();
        return false;
    }
    link.SetType(type.toStdString());
    link.SetName(LinkName.toStdString());
    if (!system.AddLink(link, sourceblock.toStdString(), targetblock.toStdString()))
    {
        qDebug() << QString::fromStdString(system.lasterror());
        QMessageBox::question(this, "Link does not match the connected block!", QString::fromStdString(system.lasterror()), QMessageBox::Ok);
        LogError(QString::fromStdString(system.lasterror()));
        //RecreateGraphicItemsFromSystem();
        return false; 
    }
    system.object(LinkName.toStdString())->SetName(LinkName.toStdString());
    system.object(LinkName.toStdString())->AssignRandomPrimaryKey();
    edge->SetObject(system.object(LinkName.toStdString()));
    foreach (QAction* action, ui->mainToolBar->actions())
    {
        action->setChecked(false);
    }
    RefreshTreeView();
    system.AddAllConstituentRelateProperties(system.link(LinkName.toStdString()));
    system.SetVariableParents();

    LogAddDelete("Link '" + LinkName + "' was added!");
    return true; 
}


void MainWindow::onaddsource()
{
    QObject* obj = sender();
    Source source;
    source.SetQuantities(system.GetMetaModel(),obj->objectName().toStdString());
    source.SetType(obj->objectName().toStdString());
    string name = CreateNewName(obj->objectName().toStdString());
    source.SetName(name);
    system.AddSource(source);
    //qDebug() << "source added! " << obj->objectName();
    system.object(name)->SetName(name);
    system.AddAllConstituentRelateProperties(system.source(name));
    RefreshTreeView();
    LogAddDelete("Source '" + QString::fromStdString(name) + "' was added!");
}

void MainWindow::onaddparameter()
{
    QObject* obj = sender();
    Parameter parameter;
    string name;
    string objectname;
    if (obj->objectName()!="")
    {   name = CreateNewName(obj->objectName().toStdString());
        objectname = obj->objectName().toStdString();
    }
    else
    {   name = CreateNewName("Parameter");
        objectname = "Parameter";
    }
    parameter.SetQuantities(system.GetMetaModel(),objectname);
    parameter.SetType(objectname);
    parameter.SetName(name);
    system.Parameters().Append(name,parameter);
    //qDebug() << "parameter added! " << obj->objectName();
    //system.object(name)->SetName(name);
    RefreshTreeView();
    LogAddDelete("Parameter '" + QString::fromStdString(name) + "' was added!");
}

void MainWindow::onaddobjectivefunction()
{
    QObject* obj = sender();
    Objective_Function objective_function;

    string name;
    string objectname;
    if (obj->objectName()!="")
    {   name = CreateNewName(obj->objectName().toStdString());
        objectname = obj->objectName().toStdString();
    }
    else
    {
        name = CreateNewName("Objective Function");
        objectname = "Objective_Function";
    }

    objective_function.SetQuantities(system.GetMetaModel(),objectname);
    objective_function.SetType(objectname);

    objective_function.SetName(name);
    system.AppendObjectiveFunction(name,objective_function);
    //qDebug() << "objective function added! " << obj->objectName();
    //system.object(name)->SetName(name);
    RefreshTreeView();
    LogAddDelete("Objective Function '" + QString::fromStdString(name) + "' was added!");

}

void MainWindow::onaddobservation()
{
    QObject* obj = sender();
    Observation observation;

    string name;
    string objectname;
    if (obj->objectName()!="")
    {   name = CreateNewName(obj->objectName().toStdString());
        objectname = obj->objectName().toStdString();
    }
    else
    {
        name = CreateNewName("Observation");
        objectname = "Observation";
    }

    observation.SetQuantities(system.GetMetaModel(),objectname);
    observation.SetType(objectname);

    observation.SetName(name);
    system.AddObservation(observation);
    system.object(name)->SetName(name);
    //qDebug() << "observation added! " << obj->objectName();
    //system.object(name)->SetName(name);
    RefreshTreeView();
    LogAddDelete("Observation '" + QString::fromStdString(name) + "' was added!");

}

void MainWindow::onaddconstituent()
{
    QObject* obj = sender();
    Constituent constituent;

    string name;
    string objectname;
    if (obj->objectName()!="")
    {   name = CreateNewName(obj->objectName().toStdString());
        objectname = obj->objectName().toStdString();
    }
    else
    {
        name = CreateNewName("Constituent");
        objectname = "Constituent";
    }

    constituent.SetQuantities(system.GetMetaModel(),objectname);
    constituent.SetType(objectname);

    constituent.SetName(name);
    system.AddConstituent(constituent);
    system.object(name)->SetName(name);
    //qDebug() << "Constituent added! " << obj->objectName();
    //system.object(name)->SetName(name);
    system.AddConstituentRelateProperties(system.constituent(name));
    RefreshTreeView();
    LogAddDelete("Constituent '" + QString::fromStdString(name) + "' was added!");

}

void MainWindow::onaddreaction()
{
    QObject* obj = sender();
    Reaction reaction;

    string name;
    string objectname;
    if (obj->objectName()!="")
    {   name = CreateNewName(obj->objectName().toStdString());
        objectname = obj->objectName().toStdString();
    }
    else
    {
        name = CreateNewName("Reaction");
        objectname = "Reaction";
    }

    reaction.SetQuantities(system.GetMetaModel(),objectname);
    reaction.SetType(objectname);

    reaction.SetName(name);
    system.AddReaction(reaction);
    system.object(name)->SetName(name);
    system.AddAllConstituentRelateProperties(system.reaction(name));
    //qDebug() << "Reaction added! " << obj->objectName();
    //system.object(name)->SetName(name);
    RefreshTreeView();
    LogAddDelete("Reaction '" + QString::fromStdString(name) + "' was added!");

}

void MainWindow::onaddreactionparameter()
{
    QObject* obj = sender();
    RxnParameter reactionparameter;

    string name;
    string objectname;
    if (obj->objectName()!="")
    {   name = CreateNewName(obj->objectName().toStdString());
        objectname = obj->objectName().toStdString();
    }
    else
    {
        name = CreateNewName("Reaction Parameter");
        objectname = "ReactionParameter";
    }

    reactionparameter.SetQuantities(system.GetMetaModel(),objectname);
    reactionparameter.SetType(objectname);

    reactionparameter.SetName(name);
    system.AddReactionParameter(reactionparameter);
    system.object(name)->SetName(name);
    //qDebug() << "Reaction Parameter added! " << obj->objectName();
    //system.object(name)->SetName(name);
    RefreshTreeView();
    LogAddDelete("Reaction '" + QString::fromStdString(name) + "' was added!");

}


void MainWindow::onaddentity()
{
    //QObject* obj = sender();
    //counts[obj->objectName()] = counts[obj->objectName()] + 1;
    //qDebug() << "entity added! " << obj->objectName();
    //Entity* item = new Entity(obj->objectName(), obj->objectName() + QString::number(counts[obj->objectName()]), diagramview, QString::fromStdString(system.GetMetaModel()->GetItem(obj->objectName().toStdString())->CategoryType()));

}

void MainWindow::RefreshTreeView()
{
    Populate_TreeWidget();
    if (propmodel != nullptr)
        delete  propmodel;
    propmodel = nullptr;
    for (unsigned int i=0; i<system.SettingsCount(); i++)
    {
        QString TypeCategory = QString::fromStdString(system.Setting(i)->TypeCategory());
        QList<QTreeWidgetItem*> MatchedItems = ui->treeWidget->findItems(QString::fromStdString(system.Setting(i)->TypeCategory()),Qt::MatchExactly);
        if (MatchedItems.size()==0)
            qDebug() << "No category called '" + TypeCategory + "' was found!";
        else if (MatchedItems.size()>1)
            qDebug() << "More than one category called '" + TypeCategory + "' was found!";
        else {
            QTreeWidgetItem *treeitem = ui->treeWidget->findItems(TypeCategory,Qt::MatchExactly)[0];
            QTreeWidgetItem *treechlditem = new QTreeWidgetItem(treeitem);
            treechlditem->setData(0,Qt::UserRole,"child");
            treechlditem->setData(0,CustomRoleCodes::Role::TypeRole,TypeCategory);
            treechlditem->setText(0,QString::fromStdString(system.Setting(i)->GetName()));
            treeitem->addChild(treechlditem);
        }
    }

    for (unsigned int i=0; i<system.BlockCount(); i++)
    {
        QString TypeCategory = QString::fromStdString(system.block(i)->TypeCategory());
        QList<QTreeWidgetItem*> MatchedItems = ui->treeWidget->findItems(QString::fromStdString(system.block(i)->TypeCategory()),Qt::MatchExactly);
        if (MatchedItems.size()==0)
            qDebug() << "No category called '" + TypeCategory + "' was found!";
        else if (MatchedItems.size()>1)
            qDebug() << "More than one category called '" + TypeCategory + "' was found!";
        else {
            QTreeWidgetItem *treeitem = ui->treeWidget->findItems(TypeCategory,Qt::MatchExactly)[0];
            QTreeWidgetItem *treechlditem = new QTreeWidgetItem(treeitem);
            treechlditem->setData(0,Qt::UserRole,"child");
            treechlditem->setData(0,CustomRoleCodes::Role::TypeRole,TypeCategory);
            treechlditem->setText(0,QString::fromStdString(system.block(i)->GetName()));
            treeitem->addChild(treechlditem);
        }
    }

    for (unsigned int i=0; i<system.LinksCount(); i++)
    {
        QString TypeCategory = QString::fromStdString(system.link(i)->TypeCategory());
        QList<QTreeWidgetItem*> MatchedItems = ui->treeWidget->findItems(QString::fromStdString(system.link(i)->TypeCategory()),Qt::MatchExactly);
        if (MatchedItems.size()==0)
            qDebug() << "No category called '" + TypeCategory + "' was found!";
        else if (MatchedItems.size()>1)
            qDebug() << "More than one category called '" + TypeCategory + "' was found!";
        else {
            QTreeWidgetItem *treeitem = ui->treeWidget->findItems(TypeCategory,Qt::MatchExactly)[0];
            QTreeWidgetItem *treechlditem = new QTreeWidgetItem(treeitem);
            treechlditem->setData(0,Qt::UserRole,"child");
            treechlditem->setData(0,CustomRoleCodes::Role::TypeRole,TypeCategory);
            treechlditem->setText(0,QString::fromStdString(system.link(i)->GetName()));
            treeitem->addChild(treechlditem);
        }
    }

    for (unsigned int i=0; i<system.SourcesCount(); i++)
    {
        QString TypeCategory = QString::fromStdString(system.source(i)->TypeCategory());
        QList<QTreeWidgetItem*> MatchedItems = ui->treeWidget->findItems(QString::fromStdString(system.source(i)->TypeCategory()),Qt::MatchExactly);
        if (MatchedItems.size()==0)
            qDebug() << "No category called '" + TypeCategory + "' was found!";
        else if (MatchedItems.size()>1)
            qDebug() << "More than one category called '" + TypeCategory + "' was found!";
        else {
            QTreeWidgetItem *treeitem = ui->treeWidget->findItems(TypeCategory,Qt::MatchExactly)[0];

            QTreeWidgetItem *treechlditem = new QTreeWidgetItem(treeitem);
            treechlditem->setData(0,CustomRoleCodes::Role::TypeRole,TypeCategory);
            treechlditem->setData(0,Qt::UserRole,"child");
            treechlditem->setText(0,QString::fromStdString(system.source(i)->GetName()));
            treeitem->addChild(treechlditem);
        }
    }

    for (unsigned int i=0; i<system.ParametersCount(); i++)
    {
        QString TypeCategory = QString::fromStdString(system.Parameters()[i]->TypeCategory());
        QList<QTreeWidgetItem*> MatchedItems = ui->treeWidget->findItems(QString::fromStdString(system.Parameters()[i]->TypeCategory()),Qt::MatchExactly);
        if (MatchedItems.size()==0)
            qDebug() << "No category called '" + TypeCategory + "' was found!";
        else if (MatchedItems.size()>1)
            qDebug() << "More than one category called '" + TypeCategory + "' was found!";
        else {
            QTreeWidgetItem *treeitem = ui->treeWidget->findItems(TypeCategory,Qt::MatchExactly)[0];
            QTreeWidgetItem *treechlditem = new QTreeWidgetItem(treeitem);
            treechlditem->setData(0,Qt::UserRole,"child");
            treechlditem->setData(0,CustomRoleCodes::Role::TypeRole,TypeCategory);
            treechlditem->setText(0,QString::fromStdString(system.Parameters()[i]->GetName()));
            treeitem->addChild(treechlditem);
        }
    }

    for (unsigned int i=0; i<system.ObjectiveFunctionsCount(); i++)
    {
        QString TypeCategory = QString::fromStdString(system.ObjectiveFunctions()[i]->TypeCategory());
        QList<QTreeWidgetItem*> MatchedItems = ui->treeWidget->findItems(QString::fromStdString(system.ObjectiveFunctions()[i]->TypeCategory()),Qt::MatchExactly);
        if (MatchedItems.size()==0)
            qDebug() << "No category called '" + TypeCategory + "' was found!";
        else if (MatchedItems.size()>1)
            qDebug() << "More than one category called '" + TypeCategory + "' was found!";
        else {
            QTreeWidgetItem *treeitem = ui->treeWidget->findItems(TypeCategory,Qt::MatchExactly)[0];
            QTreeWidgetItem *treechlditem = new QTreeWidgetItem(treeitem);
            treechlditem->setData(0,CustomRoleCodes::Role::TypeRole,TypeCategory);
            treechlditem->setData(0,Qt::UserRole,"child");
            treechlditem->setText(0,QString::fromStdString(system.ObjectiveFunctions()[i]->GetName()));
            treeitem->addChild(treechlditem);
        }
    }

    for (unsigned int i=0; i<system.ObservationsCount(); i++)
    {
        QString TypeCategory = QString::fromStdString(system.observation(i)->TypeCategory());
        QList<QTreeWidgetItem*> MatchedItems = ui->treeWidget->findItems(QString::fromStdString(system.observation(i)->TypeCategory()),Qt::MatchExactly);
        if (MatchedItems.size()==0)
            qDebug() << "No category called '" + TypeCategory + "' was found!";
        else if (MatchedItems.size()>1)
            qDebug() << "More than one category called '" + TypeCategory + "' was found!";
        else {
            QTreeWidgetItem *treeitem = ui->treeWidget->findItems(TypeCategory,Qt::MatchExactly)[0];
            QTreeWidgetItem *treechlditem = new QTreeWidgetItem(treeitem);
            treechlditem->setData(0,CustomRoleCodes::Role::TypeRole,TypeCategory);
            treechlditem->setData(0,Qt::UserRole,"child");
            treechlditem->setText(0,QString::fromStdString(system.observation(i)->GetName()));
            treeitem->addChild(treechlditem);
        }
    }

    for (unsigned int i=0; i<system.ConstituentsCount(); i++)
    {
        QString TypeCategory = QString::fromStdString(system.constituent(i)->TypeCategory());
        QList<QTreeWidgetItem*> MatchedItems = ui->treeWidget->findItems(QString::fromStdString(system.constituent(i)->TypeCategory()),Qt::MatchExactly);
        if (MatchedItems.size()==0)
            qDebug() << "No category called '" + TypeCategory + "' was found!";
        else if (MatchedItems.size()>1)
            qDebug() << "More than one category called '" + TypeCategory + "' was found!";
        else {
            QTreeWidgetItem *treeitem = ui->treeWidget->findItems(TypeCategory,Qt::MatchExactly)[0];
            QTreeWidgetItem *treechlditem = new QTreeWidgetItem(treeitem);
            treechlditem->setData(0,CustomRoleCodes::Role::TypeRole,TypeCategory);
            treechlditem->setData(0,Qt::UserRole,"child");
            treechlditem->setText(0,QString::fromStdString(system.constituent(i)->GetName()));
            treeitem->addChild(treechlditem);
        }
    }

    for (unsigned int i=0; i<system.ReactionParametersCount(); i++)
    {
        QString TypeCategory = QString::fromStdString(system.reactionparameter(i)->TypeCategory());
        QList<QTreeWidgetItem*> MatchedItems = ui->treeWidget->findItems(QString::fromStdString(system.reactionparameter(i)->TypeCategory()),Qt::MatchExactly);
        if (MatchedItems.size()==0)
            qDebug() << "No category called '" + TypeCategory + "' was found!";
        else if (MatchedItems.size()>1)
            qDebug() << "More than one category called '" + TypeCategory + "' was found!";
        else {
            QTreeWidgetItem *treeitem = ui->treeWidget->findItems(TypeCategory,Qt::MatchExactly)[0];
            QTreeWidgetItem *treechlditem = new QTreeWidgetItem(treeitem);
            treechlditem->setData(0,CustomRoleCodes::Role::TypeRole,TypeCategory);
            treechlditem->setData(0,Qt::UserRole,"child");
            treechlditem->setText(0,QString::fromStdString(system.reactionparameter(i)->GetName()));
            treeitem->addChild(treechlditem);
        }
    }

    for (unsigned int i=0; i<system.ReactionsCount(); i++)
    {
        QString TypeCategory = QString::fromStdString(system.reaction(i)->TypeCategory());
        QList<QTreeWidgetItem*> MatchedItems = ui->treeWidget->findItems(QString::fromStdString(system.reaction(i)->TypeCategory()),Qt::MatchExactly);
        if (MatchedItems.size()==0)
            qDebug() << "No category called '" + TypeCategory + "' was found!";
        else if (MatchedItems.size()>1)
            qDebug() << "More than one category called '" + TypeCategory + "' was found!";
        else {
            QTreeWidgetItem *treeitem = ui->treeWidget->findItems(TypeCategory,Qt::MatchExactly)[0];
            QTreeWidgetItem *treechlditem = new QTreeWidgetItem(treeitem);
            treechlditem->setData(0,CustomRoleCodes::Role::TypeRole,TypeCategory);
            treechlditem->setData(0,Qt::UserRole,"child");
            treechlditem->setText(0,QString::fromStdString(system.reaction(i)->GetName()));
            treeitem->addChild(treechlditem);
        }
    }
}

string MainWindow::CreateNewName(string type)
{
    int i=1;
    string newname = type + " (" + aquiutils::numbertostring(i) + ")";
    while (system.object(newname)!=nullptr)
    {
        newname = type + " (" + aquiutils::numbertostring(i++) + ")";
    }
    return newname;

}

void MainWindow::preparetreeviewMenu(const QPoint &pos)
{
    QTreeWidget *tree = ui->treeWidget;

    QTreeWidgetItem *nd = tree->itemAt( pos );

    //qDebug()<<nd->data(0,Qt::UserRole);

    //qDebug()<<pos<<nd->text(0);

    if (nd->data(0,Qt::UserRole)=="main")
    {
        if (nd->text(0)=="Parameters" || nd->text(0)=="Objective Functions" || nd->text(0)=="Reactions" || nd->text(0)=="Reaction Parameters" || nd->text(0)=="Constituents" || nd->text(0)=="Observations")
        {
            menu = new QMenu(this);
            QAction *AddAct = new QAction(QIcon(":/Resource/warning32.ico"), "Add " + nd->text(0), this);
            AddAct->setProperty("group",nd->text(0));
            AddAct->setStatusTip("Append " + nd->text(0));
            connect(AddAct, SIGNAL(triggered()), this, SLOT(onAddItemThroughTreeViewRightClick()));
            menu->addAction(AddAct);
            QPoint pt(pos);
        }
        if (nd->data(0,CustomRoleCodes::Role::TypeRole).toString() == "Objective Functions")
        {
            QMenu* results = menu->addMenu("Results");
            timeseriestobeshown = "Time Series";
            QAction* graphaction = results->addAction(timeseriestobeshown);
            QVariant v = QVariant::fromValue(QString::fromStdString(system.objectivefunction(nd->text(0).toStdString())->GetOutputItem()));
            graphaction->setData(v);
            //called_by_clicking_on_graphical_object = true;
            connect(graphaction, SIGNAL(triggered()), this, SLOT(showgraph()));
        }
        if (nd->data(0,CustomRoleCodes::Role::TypeRole).toString() == "Observations")
        {
            QMenu* results = menu->addMenu("Results");
            timeseriestobeshown = "Modeled vs Measured";
            QAction* graphaction = results->addAction(timeseriestobeshown);
            QVariant v = QVariant::fromValue(QString::fromStdString(system.observation(nd->text(0).toStdString())->GetOutputItem()));
            graphaction->setData(v);
            graphaction->setProperty("object",QString::fromStdString(system.observation(nd->text(0).toStdString())->GetName()));
            //called_by_clicking_on_graphical_object = true;
            connect(graphaction, SIGNAL(triggered()), this, SLOT(showgraph()));
        }
        if (nd->data(0, CustomRoleCodes::Role::TypeRole).toString() == "Sources")
        {
            menu = new QMenu(this);
            timeseriestobeshown = "Precipitation";
            if (GetSystem()->source(nd->text(0).toStdString())->Variable("timeseries")!=nullptr)
            {
                if (GetSystem()->source(nd->text(0).toStdString())->Variable("timeseries")->GetTimeSeries()!=nullptr)
                {   QAction* graphaction = menu->addAction(timeseriestobeshown);
                    QVariant v = QVariant::fromValue(nd->text(0));
                    graphaction->setData(v);
                    //called_by_clicking_on_graphical_object = true;
                    connect(graphaction, SIGNAL(triggered()), this, SLOT(showgraph()));
                }
            }
        }
        if (menu)
            menu->exec( tree->mapToGlobal(pos) );
        if (menu)
            delete(menu);
        return;

    }

    if (nd->data(0,Qt::UserRole)=="child")
    {
        QAction *DeleteAct = new QAction(QIcon(":/Resource/warning32.ico"), "Delete" , this);
        DeleteAct->setStatusTip("Delete");
        DeleteAct->setData(nd->text(0));
        connect(DeleteAct, SIGNAL(triggered()), this, SLOT(onDeleteItem()));
        QMenu menu(this);
        menu.addAction(DeleteAct);
        QMenu* results = menu.addMenu("Results");
        if (system.object(nd->text(0).toStdString())==nullptr)
        {
            RefreshTreeView();
            return;
        }
        for (unsigned int i = 0; i < system.object(nd->text(0).toStdString())->ItemswithOutput().size(); i++)
        {
            timeseriestobeshown = QString::fromStdString(system.object(nd->text(0).toStdString())->ItemswithOutput()[i]);
            QAction* graphaction = results->addAction(timeseriestobeshown);
            QVariant v = QVariant::fromValue(QString::fromStdString(system.object(nd->text(0).toStdString())->Variable(timeseriestobeshown.toStdString())->GetOutputItem()));
            graphaction->setData(v);
            //called_by_clicking_on_graphical_object = true;
            connect(graphaction, SIGNAL(triggered()), this, SLOT(showgraph()));
        }

        menu.exec( tree->mapToGlobal(pos) );

    }
}

void MainWindow::showgraph()
{
    QAction* act = qobject_cast<QAction*>(sender());
    QString item = act->data().toString();
    if (timeseriestobeshown == "Precipitation")
    {
        if (GetSystem()->source(item.toStdString())->Variable("timeseries")->GetTimeSeries() != nullptr)
        {
            Plotter* plot = Plot(*GetSystem()->source(item.toStdString())->Variable("timeseries")->GetTimeSeries());
            plot->SetYAxisTitle(act->text());
        }
    return;
    }
    if (timeseriestobeshown == "Modeled vs Measured")
    {
        QString object = act->property("object").toString();
        if (GetSystem()->observation(object.toStdString())->Variable("observed_data")->GetTimeSeries() != nullptr)
        {
            if (GetSystem()->GetOutputs().Contains(item.toStdString()))
            {
                Plotter* plot = Plot(GetSystem()->GetOutputs()[item.toStdString()],*GetSystem()->observation(object.toStdString())->Variable("observed_data")->GetTimeSeries());
                plot->SetYAxisTitle(act->text());
            }
            else
            {
                Plotter* plot = Plot(*GetSystem()->observation(object.toStdString())->Variable("observed_data")->GetTimeSeries());
                plot->SetYAxisTitle(act->text());
            }
        }
        else if (GetSystem()->GetOutputs().Contains(item.toStdString()))
        {
            Plotter* plot = Plot(GetSystem()->GetOutputs()[item.toStdString()]);
            plot->SetYAxisTitle(act->text());
        }
    }
    else
    {    
        if (GetSystem()->GetOutputs()[item.toStdString()].n==0)
        {
            QMessageBox::question(this, "Time Series is empty!", "The result for this quantity is empty!", QMessageBox::Ok);
            return;
        }
        Plotter* plot = Plot(GetSystem()->GetOutputs()[item.toStdString()]);
        plot->SetYAxisTitle(act->text());
    }

}

void MainWindow::onDeleteItem()
{
    QAction* act = qobject_cast<QAction*>(sender());
    QString item = act->data().toString();
    dView->deleteselectednode(item);
    
}

void MainWindow::onTreeSelectionChanged(QTreeWidgetItem *current)
{
    //qDebug()<<current->data(0,Qt::UserRole);
    //qDebug()<< current->text(0);

    if (current->data(0,Qt::UserRole)=="child")
    {   if (system.object(current->text(0).toStdString())==nullptr)
        {
            RefreshTreeView();
            return;
        }
        PopulatePropertyTable(system.object(current->text(0).toStdString())->GetVars());
    }
}

void MainWindow::PopulatePropertyTable(QuanSet* quanset)
{
    if (propmodel != nullptr)
        delete  propmodel;
    if (quanset!=nullptr)
        propmodel = new PropModel(quanset,this,this);
    else
        propmodel = nullptr;
    ui->tableView->setModel(propmodel);
}

void MainWindow::Populate_General_ToolBar()
{
    // Save //
    QAction* actionsave = new QAction(this);
    actionsave->setObjectName("Save");
    QIcon iconsave;
    iconsave.addFile(QString::fromStdString(qApp->applicationDirPath().toStdString() + "/../../resources/Icons/Save.png"), QSize(), QIcon::Normal, QIcon::Off);
    actionsave->setIcon(iconsave);
    ui->GeneraltoolBar->addAction(actionsave);
    actionsave->setText("Save");
    actionsave->setToolTip("Save");
    connect(actionsave, SIGNAL(triggered()), this, SLOT(onsave()));
    // Open //
    QAction* actionopen = new QAction(this);
    actionopen->setObjectName("Open");
    QIcon iconopen;
    iconopen.addFile(QString::fromStdString(qApp->applicationDirPath().toStdString() + "/../../resources/Icons/open.png"), QSize(), QIcon::Normal, QIcon::Off);
    actionopen->setIcon(iconopen);
    ui->GeneraltoolBar->addAction(actionopen);
    actionopen->setText("Open");
    actionopen->setToolTip("Open");
    connect(actionopen, SIGNAL(triggered()), this, SLOT(onopen()));
    // ZoomAll //
    QAction* actionzoomall = new QAction(this);
    actionzoomall->setObjectName("Zoom All");
    actionopen->setToolTip("Zoom All");
    QIcon iconzoomall;
    iconzoomall.addFile(QString::fromStdString(qApp->applicationDirPath().toStdString() + "/../../resources/Icons/Full_screen_view.png"), QSize(), QIcon::Normal, QIcon::Off);
    actionzoomall->setIcon(iconzoomall);
    ui->GeneraltoolBar->addAction(actionzoomall);
    actionzoomall->setText("Zoom Extends");
    actionzoomall->setToolTip("Zoom All");
    actionzoomall->setObjectName("Zoom All");
    connect(actionzoomall, SIGNAL(triggered()), this, SLOT(onzoomall()));
    // ZoomIn //
    QAction* actionzoomin = new QAction(this);
    actionzoomin->setObjectName("Zoom_In");
    QIcon iconzoomin;
    iconzoomin.addFile(QString::fromStdString(qApp->applicationDirPath().toStdString() + "/../../resources/Icons/zoom_in.png"), QSize(), QIcon::Normal, QIcon::Off);
    actionzoomin->setIcon(iconzoomin);
    ui->GeneraltoolBar->addAction(actionzoomin);
    actionzoomin->setText("Zoom In");
    actionzoomin->setToolTip("Zoom In");
    connect(actionzoomin, SIGNAL(triggered()), this, SLOT(onzoomin()));
    // ZoomOut //
    QAction* actionzoomout = new QAction(this);
    actionzoomall->setObjectName("Zoom_Out");
    QIcon iconzoomout;
    iconzoomout.addFile(QString::fromStdString(qApp->applicationDirPath().toStdString() + "/../../resources/Icons/zoom_out.png"), QSize(), QIcon::Normal, QIcon::Off);
    actionzoomout->setIcon(iconzoomout);
    ui->GeneraltoolBar->addAction(actionzoomout);
    actionzoomout->setText("Zoom Out");
    actionzoomout->setToolTip("Zoom Out");
    connect(actionzoomout, SIGNAL(triggered()), this, SLOT(onzoomout()));
    actionzoomwindow = new QAction(this);
    actionzoomwindow->setObjectName("Zoom Window");
    actionzoomwindow->setToolTip("Zoom Window");
    QIcon iconzoomwindow;
    iconzoomwindow.addFile(QString::fromStdString(qApp->applicationDirPath().toStdString() + "/../../resources/Icons/WindowZoom.png"), QSize(), QIcon::Normal, QIcon::Off);
    actionzoomwindow->setCheckable(true);
    actionzoomwindow->setIcon(iconzoomwindow);
    ui->GeneraltoolBar->addAction(actionzoomwindow);
    actionzoomwindow->setText("Zoom Window");
    connect(actionzoomwindow, SIGNAL(triggered()), this, SLOT(onzoomwindowtriggered()));
    QAction* seperator = new QAction(this);
    seperator->setSeparator(true);
    ui->GeneraltoolBar->addAction(seperator);



    // Pan //
    actionpan = new QAction(this);
    actionpan->setObjectName("Pan");
    actionpan->setToolTip("Pan");
    QIcon iconpan;
    iconpan.addFile(QString::fromStdString(qApp->applicationDirPath().toStdString() + "/../../resources/Icons/pan2.png"), QSize(), QIcon::Normal, QIcon::Off);
    actionpan->setCheckable(true);
    actionpan->setIcon(iconpan);
    ui->GeneraltoolBar->addAction(actionpan);
    actionpan->setText("Pan");
    connect(actionpan, SIGNAL(triggered()), this, SLOT(onpantriggered()));
    seperator->setSeparator(true);
    ui->GeneraltoolBar->addAction(seperator);


    // Run
    QIcon iconrun;
    iconrun.addFile(QString::fromStdString(qApp->applicationDirPath().toStdString() + "/../../resources/Icons/runmodel.png"), QSize(), QIcon::Normal, QIcon::Off);
    QAction* actionrun = new QAction(this);
    actionrun->setIcon(iconrun);
    ui->GeneraltoolBar->addAction(actionrun);
    actionrun->setText("Run Model");
    actionrun->setToolTip("Run Model");
    actionrun->setObjectName("Run Model");
    connect(actionrun, SIGNAL(triggered()), this, SLOT(onrunmodel()));
    QIcon iconoptimize;
    iconoptimize.addFile(QString::fromStdString(qApp->applicationDirPath().toStdString() + "/../../resources/Icons/optimize.png"), QSize(), QIcon::Normal, QIcon::Off);
    QAction* actionoptimize = new QAction(this);
    actionoptimize->setIcon(iconoptimize);
    ui->GeneraltoolBar->addAction(actionoptimize);
    actionoptimize->setText("Optimize");
    actionoptimize->setToolTip("Optimize");
    connect(actionoptimize, SIGNAL(triggered()), this, SLOT(onoptimize()));
    QIcon iconinverserun;
    iconinverserun.addFile(QString::fromStdString(qApp->applicationDirPath().toStdString() + "/../../resources/Icons/inverserun.png"), QSize(), QIcon::Normal, QIcon::Off);
    QAction* actioninverse = new QAction(this);
    actioninverse->setIcon(iconinverserun);
    ui->GeneraltoolBar->addAction(actioninverse);
    actioninverse->setText("Invese Run");
    actioninverse->setToolTip("Inverse Run");
    connect(actioninverse, SIGNAL(triggered()), this, SLOT(oninverserun()));

}

void MainWindow::onzoomin()
{
    dView->scaleView(1.25);
}

void MainWindow::onzoomout()
{
    dView->scaleView(1 / 1.25);
}

void MainWindow::onzoomall()
{
    QRectF newRect = dView->MainGraphicsScene->itemsBoundingRect();
    float width = float(newRect.width());
    float height = float(newRect.height());
    float scale = float(1.1);
    newRect.setLeft(newRect.left() - float(scale - 1) / 2 * float(width));
    newRect.setTop(newRect.top() - (scale - 1) / 2 * height);
    newRect.setWidth(qreal(width * scale));
    newRect.setHeight(qreal(height * scale));
    //qDebug()<<newRect;
    if (width>dView->MainGraphicsScene->sceneRect().width() || height>dView->MainGraphicsScene->sceneRect().height() )
        dView->MainGraphicsScene->setSceneRect(newRect);
    dView->fitInView(newRect,Qt::KeepAspectRatio);
}

void MainWindow::onabout()
{
    AboutDialog* abtdlg = new AboutDialog(this);
    abtdlg->AppendText(QString("OpenHydroQual - version: ") + QString(openhydroqual_version));
    abtdlg->AppendText(QString("Last modified: ") + QString(last_modified));
    abtdlg->AppendText("EnviroInformatics, LLC");
    abtdlg->AppendText("Plugins added:");
    for (unsigned int i=0; i<addedtemplatefilenames.size(); i++)
        abtdlg->AppendText(QString::fromStdString("    ") + QString::fromStdString(addedtemplatefilenames[i]));
    abtdlg->show();
}

void MainWindow::onpantriggered()
{
    if (actionpan->isChecked())
    {
        dView->setMode(Operation_Modes::Pan);
    }
    else
    {
        dView->setMode(Operation_Modes::NormalMode);
    }


    dView->setModeCursor();
}

void MainWindow::onzoomwindowtriggered()
{
    if (actionzoomwindow->isChecked())
    {
        dView->setMode(Operation_Modes::ZoomWindow);
    }
    else
    {
        dView->setMode(Operation_Modes::NormalMode);
    }

    dView->setModeCursor();
}

void MainWindow::onsaveas()
{
    QString fileName = QFileDialog::getSaveFileName(this,
            tr("Save"), "",
            tr("script files (*.scr)"));
    if (fileName!="")
    {
        //qDebug() << fileName.split('.');

        if (!fileName.contains("."))
            fileName = fileName + ".scr";
        else if (fileName.split('.')[fileName.split('.').size()-1]!="scr" )
        {
            fileName = fileName + ".scr";
        }
        system.SavetoScriptFile(fileName.toStdString(),maintemplatefilename, addedtemplatefilenames);

        workingfolder = QFileInfo(fileName).canonicalPath();
        SetFileName(fileName);
        addToRecentFiles(fileName,true);
    }

}

void MainWindow::onexporttosvg()
{
    QString fileName = QFileDialog::getSaveFileName(this,
            tr("Save"), "",
            tr("svg files (*.svg)"));
    QString svgfileName;
    if (fileName!="")
    {
        //qDebug() << fileName.split('.');

        if (!fileName.contains("."))
            svgfileName = fileName + ".svg";
        else if (fileName.split('.')[fileName.split('.').size()-1]!="svg" )
        {
            svgfileName = fileName + ".svg";
        }
        else
            svgfileName = fileName;
        saveSceneToSvg(svgfileName);
    }

}

void MainWindow::onsave()
{
    if (filename!="")
    {   system.SavetoScriptFile(filename.toStdString(),maintemplatefilename, addedtemplatefilenames);
        addToRecentFiles(filename,true);
    }
    else
        onsaveas();

}


void MainWindow::onopen()
{
    QString fileName = QFileDialog::getOpenFileName(this,
            tr("Open"), "",
            tr("Script files (*.scr);; All files (*.*)"));


    if (fileName!="")
    {
        Script scr(fileName.toStdString(),&system);
        system.clear();
        system.CreateFromScript(scr,entitiesfilename);
        workingfolder = QFileInfo(fileName).canonicalPath();
        SetFileName(fileName);
        addToRecentFiles(fileName,true);
    }
    addedtemplatefilenames = system.addedtemplates; 
    PopulatePropertyTable(nullptr);
    RecreateGraphicItemsFromSystem();
    RefreshTreeView();
    BuildObjectsToolBar();
    LogAllSystemErrors();

}

bool MainWindow::LoadModel(QString fileName)
{
    bool success = true;
    if (fileName!="")
    {
        Script scr(fileName.toStdString(),&system);
        system.clear();
        system.CreateFromScript(scr,entitiesfilename);
        workingfolder = QFileInfo(fileName).canonicalPath();
        SetFileName(fileName);
        addToRecentFiles(fileName,true);
    }
    addedtemplatefilenames = system.addedtemplates;
    PopulatePropertyTable(nullptr);
    RecreateGraphicItemsFromSystem();
    RefreshTreeView();
    BuildObjectsToolBar();
    LogAllSystemErrors();

    return success; 
}


void MainWindow::LogAllSystemErrors(ErrorHandler *errs)
{
    if (errs==nullptr)
    {   for (int i=0; i<system.GetErrorHandler()->Count(); i++)
            LogError(QString::fromStdString(system.GetErrorHandler()->at(i)->description));
        system.GetErrorHandler()->clear();
    }
    else
        for (int i=0; i<errs->Count(); i++)
             LogError(QString::fromStdString(errs->at(i)->description));

}

void MainWindow::RecreateGraphicItemsFromSystem(bool zoom_all)
{
    dView->DeleteAllItems();
    for (unsigned int i=0; i<system.BlockCount(); i++)
    {
        Node *node = new Node(dView,&system);
        system.block(i)->AssignRandomPrimaryKey();
        node->SetObject(system.block(i));
        node->setX(system.block(i)->GetVal("x"));
        node->setY(system.block(i)->GetVal("y"));
        node->setWidth(system.block(i)->GetVal("_width"));
        node->setHeight(system.block(i)->GetVal("_height"));

    }
    for (unsigned int i=0; i<system.LinksCount(); i++)
    {
        Node *s_node = dView->node(QString::fromStdString(system.block(system.link(i)->s_Block_No())->GetName()));
        Node *e_node = dView->node(QString::fromStdString(system.block(system.link(i)->e_Block_No())->GetName()));
        if (s_node && e_node)
        {
            Edge *edge = new Edge(s_node,e_node,dView );
            system.link(i)->AssignRandomPrimaryKey();
            edge->SetObject(system.link(i));
        }
    }
    dView->repaint();
    if (zoom_all)
        onzoomall();
}

void MainWindow::onrunmodel()
{
    ErrorHandler errs = system.VerifyAllQuantities();
    if (errs.Count()!=0)
    {
        LogAllSystemErrors(&errs);
        QMessageBox::question(this, "Errors!", "There are errors in the values assigned to some of the variables. Check the log window for more details.", QMessageBox::Ok);
        return;
    }
    System copiedsystem(system);
    copiedsystem.SetSystemSettings();
    if (copiedsystem.GetSolverSettings().write_solution_details)
        copiedsystem.SetSolutionLogger(workingfolder.toStdString() + "/solution_details.txt");
    rtw = new RunTimeWindow(this);
    rtw->show();
    rtw->SetUpForForwardRun();
    copiedsystem.SetRunTimeWindow(rtw);
    copiedsystem.Solve(true);
    copiedsystem.GetOutputs().writetofile(workingfolder.toStdString() + "/outputs.txt");
    copiedsystem.GetObservedOutputs().writetofile(workingfolder.toStdString() + "/observedoutputs.txt");
    copiedsystem.errorhandler.Write(workingfolder.toStdString() + "/errors.txt");
    if (copiedsystem.GetSolutionLogger())
        copiedsystem.GetSolutionLogger()->Close();
    system.TransferResultsFrom(&copiedsystem);
    system.SetOutputItems();

}

void MainWindow::closeEvent (QCloseEvent *event)
{
    QMessageBox::StandardButton resBtn = QMessageBox::question( this, "OpenHydroQual",
                                                                tr("Are you sure?\n"),
                                                                QMessageBox::Cancel | QMessageBox::Yes,
                                                                QMessageBox::Yes);
    if (resBtn != QMessageBox::Yes) {
        event->ignore();
    } else {
        system.stop_triggered = true;
        if (rtw) rtw->close();
        if (LogWindow) LogWindow->close();
        event->accept();
    }
    exit; 
}

void MainWindow::onoptimize()
{
    ErrorHandler errs = system.VerifyAllQuantities();
    if (system.ParametersCount()==0)
    {
        LogAllSystemErrors(&errs);
        QMessageBox::question(this, "Errors!", "No parameters have been defined!", QMessageBox::Ok);
        return;
    }
    if (errs.Count()!=0)
    {
        LogAllSystemErrors(&errs);
        QMessageBox::question(this, "Errors!", "There are errors in the values assigned to some of the variables. Check the log window for more details.", QMessageBox::Ok);
        return;
    }
    system.SetSystemSettings();
    optimizer = new CGA<System>(&system);
    optimizer->SetParameters(system.object("Optimizer"));
    optimizer->filenames.pathname = workingfolder.toStdString() + "/";
    system.SetAllParents();
    rtw = new RunTimeWindow(this);
    rtw->show();
    rtw->AppendText("Optimization Started ...");
    rtw->SetXRange(0,optimizer->GA_params.nGen);
    rtw->SetUpForInverseRun(); 
    system.SetRunTimeWindow(nullptr);
    optimizer->SetRunTimeWindow(rtw);
    system.SetParameterEstimationMode(parameter_estimation_options::optimize);
    optimizer->optimize();
    system.TransferResultsFrom(&optimizer->Model_out);
    system.Parameters() = optimizer->Model_out.Parameters();
    system.SetOutputItems();
    system.SetParameterEstimationMode();
    rtw->AppendText("Optimization Finished!");
}

void MainWindow::oninverserun()
{
    ErrorHandler errs = system.VerifyAllQuantities();
    if (system.ParametersCount()==0)
    {
        LogAllSystemErrors(&errs);
        QMessageBox::question(this, "Errors!", "No parameters have been defined!", QMessageBox::Ok);
        return;
    }
    if (errs.Count()!=0)
    {
        LogAllSystemErrors(&errs);
        QMessageBox::question(this, "Errors!", "There are errors in the values assigned to some of the variables. Check the log window for more details.", QMessageBox::Ok);
        return;
    }
    system.SetSystemSettings();
    optimizer = new CGA<System>(&system);
    optimizer->SetParameters(system.object("Optimizer"));
    optimizer->filenames.pathname = workingfolder.toStdString() + "/";
    system.SetAllParents();
    rtw = new RunTimeWindow(this);
    rtw->show();
    rtw->AppendText("Parameter Estimation Started ...");
    rtw->SetXRange(0,optimizer->GA_params.nGen);
    rtw->SetUpForInverseRun();
    system.SetRunTimeWindow(nullptr);
    system.SetParameterEstimationMode(parameter_estimation_options::inverse_model);
    optimizer->SetRunTimeWindow(rtw);
    optimizer->optimize();
    system.TransferResultsFrom(&optimizer->Model_out);
    system.Parameters() = optimizer->Model_out.Parameters();
    system.SetParameterEstimationMode();
    system.SetOutputItems();
    rtw->AppendText("Parameter Estimation Finished!");
}


Plotter* MainWindow::Plot(CTimeSeries& plotitem)
{
    Plotter* plotter = new Plotter(this);
    plotter->PlotData(plotitem);
    plotter->show();
    return plotter;
}

Plotter* MainWindow::Plot(CTimeSeries& plotmodeled, CTimeSeries& plotobserved)
{
    Plotter* plotter = new Plotter(this);
    if (plotmodeled.n>0)
        plotter->PlotData(plotmodeled,"line");
    if (plotobserved.n>0)
    plotter->AddData(plotobserved,"dots");
    plotter->show();
    return plotter;
}

void MainWindow::onAddItemThroughTreeViewRightClick()
{
    QObject* obj = sender();
    if (obj->property("group")=="Parameters")
        onaddparameter();
    if (obj->property("group")=="Objective Functions")
        onaddobjectivefunction();
    if (obj->property("group")=="Reactions")
        onaddreaction();
    if (obj->property("group")=="Reaction Parameters")
        onaddreactionparameter();
    if (obj->property("group")=="Constituents")
        onaddconstituent();
    if (obj->property("group")=="Observations")
        onaddobservation();


    //counts[obj->objectName()] = counts[obj->objectName()] + 1;
        //qDebug() << "entity added! " << obj->objectName();
        //Entity* item = new Entity(obj->objectName(), obj->objectName() + QString::number(counts[obj->objectName()]), diagramview, QString::fromStdString(system.GetMetaModel()->GetItem(obj->objectName().toStdString())->CategoryType()));


}

void MainWindow::loadnewtemplate()
{
    QString fileName = QFileDialog::getOpenFileName(this,
            tr("Open"), "",
            tr("Script files (*.json);; All files (*.*)"));


    if (fileName!="")
    {
        system.clear();
        system.GetMetaModel()->Clear();
        system.GetQuanTemplate(fileName.toStdString());
        system.ReadSystemSettingsTemplate(entitiesfilename);
        ui->mainToolBar->clear();
        BuildObjectsToolBar();
        RefreshTreeView();
        maintemplatefilename = fileName.toStdString();
    }

}

void MainWindow::addplugin()
{
    QString fileName = QFileDialog::getOpenFileName(this,
            tr("Open"), "",
            tr("Script files (*.json);; All files (*.*)"));


    if (fileName!="")
    {
        if (system.AppendQuanTemplate(fileName.toStdString()))
        {   ui->mainToolBar->clear();
            BuildObjectsToolBar();
            RefreshTreeView();
            addedtemplatefilenames.push_back(fileName.toStdString());
        }
        else
        {
            LogError("Error in file '" + filename + "' :" +  QString::fromStdString(system.GetMetaModel()->GetLastError()));
        }
    }

}

void MainWindow::addplugin(const QString &fileName)
{

    if (fileName!="")
    {
        if (system.AppendQuanTemplate(fileName.toStdString()))
        {   ui->mainToolBar->clear();
            BuildObjectsToolBar();
            RefreshTreeView();
            addedtemplatefilenames.push_back(fileName.toStdString());
        }
        else
        {
            LogError("Error in file '" + filename + "' :" +  QString::fromStdString(system.GetMetaModel()->GetLastError()));
        }
    }

}

void MainWindow::adddefaultpluging()
{
    Wizard_select_dialog *Wizard_Select_Form = new Wizard_select_dialog(this);
    Wizard_Select_Form->show();

}

void MainWindow::optionsdialog()
{
    OptionsDialog *Options_Dialog = new OptionsDialog(this);
    Options_Dialog->show();

}

bool MainWindow::Log(const QString &s)
{
    LogWindow->AppendText(s);
    return true;
}

bool MainWindow::LogError(const QString &s)
{
    LogWindow->AppendError(s);
    ui->actionLog_Window->setChecked(true);
    LogWindow->show();
    return true;
}

bool MainWindow::LogAddDelete(const QString &s)
{
    LogWindow->AppendBlue(s);
    return true;
}

void MainWindow::SetFileName(const QString &_filename)
{
    filename = _filename;
    if (filename.split('/').size()>0)
        setWindowTitle("OpenHydroQual: " + filename.split('/')[filename.split('/').size()-1]);

}

void MainWindow::readRecentFilesList()
{
//	//qDebug() << localAppFolderAddress();
//	QString add = localAppFolderAddress();
    ifstream file(localAppFolderAddress().toStdString()+RECENT);
    int count = 0;
    if (file.good())
    {
        string line;
        while (!file.eof())
        {
            getline(file, line);
            count++;
        }
        file.close();
    }

    file.open(localAppFolderAddress().toStdString() + RECENT);
    int n = 0;
    if (file.good())
    {
        string line;
        while (!file.eof())
        {
            getline(file, line);
            n++;
            QString fileName = QString::fromStdString(line);
            //qDebug() << fileName; QString::fromStdString(line);
            if (n>count-max_num_recent_files)
                addToRecentFiles(fileName, false);

        }
        file.close();

    }
}
void MainWindow::addToRecentFiles(QString fileName, bool addToFile)
{
    bool rewriteFile = false;
    if (recentFiles.contains(fileName) && fileName.trimmed() != "")
        if (recentFiles.indexOf(fileName) != recentFiles.count()-1)
        {
            ui->menuRecent->removeAction(ui->menuRecent->actions()[recentFiles.size() - 1 - recentFiles.indexOf(fileName)]);
            recentFiles.removeOne(fileName);
            addToFile = false;
            rewriteFile = true;
        }

    if (!recentFiles.contains(fileName) && fileName.trimmed() != "")
    {
        recentFiles.append(fileName);
        //		QAction * a = ui->menuRecent->addAction(fileName);// , this, SLOT(recentItem()));
        QAction * fileNameAction = new QAction(fileName, nullptr);
        if (ui->menuRecent->actions().size())
            ui->menuRecent->insertAction(ui->menuRecent->actions()[0], fileNameAction);
        else
            ui->menuRecent->addAction(fileNameAction);
        QObject::connect(fileNameAction, SIGNAL(triggered()), this, SLOT(on_actionRecent_triggered()));

        if (addToFile)
        {
            ofstream file(localAppFolderAddress().toStdString() + RECENT, fstream::app);
            if (file.good())
                file << fileName.toStdString() << std::endl;
            file.close();
        }
        if (rewriteFile)
            writeRecentFilesList();
    }
}

void MainWindow::writeRecentFilesList()
{
    ofstream file(localAppFolderAddress().toStdString() + RECENT);
    if (file.good())
    {
        foreach (QString fileName , recentFiles)
            file << fileName.toStdString() << std::endl;
    }
    file.close();
}

QString localAppFolderAddress() {
    #ifdef _WIN32
    TCHAR szPath[MAX_PATH];

    if (SUCCEEDED(SHGetFolderPath(NULL,
        CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE,
        NULL,
        0,
        szPath)))
    {
        return QString("%1/").arg(QString::fromStdWString(szPath));
        //PathAppend(szPath, TEXT("New Doc.txt"));
        //HANDLE hFile = CreateFile(szPath, ...);
    }
#else
    return QString();
#endif
}

void MainWindow::on_actionRecent_triggered()
{
    QAction* a = static_cast<QAction*> (QObject::sender());
    QString fileName = a->text();
    if (LoadModel(fileName))
    {
        addToRecentFiles(fileName, false);
    }

}

void MainWindow::removeFromRecentList(QAction* selectedFileAction)
{
    recentFiles.removeAll(selectedFileAction->text());
    ui->menuRecent->removeAction(selectedFileAction);
    writeRecentFilesList();
}

void MainWindow::saveSceneToSvg(const QString &filename) {

    QSize sceneSize = GetDiagramView()->MainGraphicsScene->sceneRect().size().toSize();

    QSvgGenerator generator;
    generator.setFileName(filename);
    generator.setSize(sceneSize);
    generator.setViewBox(QRect(0, 0, sceneSize.width(), sceneSize.height()));
    generator.setDescription(QObject::tr("My canvas exported to Svg"));
    generator.setTitle(filename);
    QPainter painter;
    painter.begin(&generator);
    GetDiagramView()->MainGraphicsScene->render(&painter);
    painter.end();


}
