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

using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    dView = new DiagramView(ui->centralWidget,this);
    dView->setObjectName(QStringLiteral("graphicsView"));
    ui->horizontalLayout->addWidget(dView);
#ifndef Win_Version
    modelfilename = qApp->applicationDirPath().toStdString() + "/../../resources/power_reservoirs_rules_source.json";
    entitiesfilename = qApp->applicationDirPath().toStdString() + "/../../resources/settings.json";
#else
    modelfilename = qApp->applicationDirPath().toStdString() + "/resources/power_reservoirs_rules_source.json";
    entitiesfilename = qApp->applicationDirPath().toStdString() + "/resources/settings.json";
#endif // !Win_Version
    system.GetQuanTemplate(modelfilename);  //Read the template from modelfilename
    system.ReadSystemSettingsTemplate(entitiesfilename); //Read the system settings
    RefreshTreeView();
    //connect(ui->treeWidget, SIGNAL(closeEvent()),ui->actionObject_Browser, SLOT())

    connect(ui->actionObject_Browser,SIGNAL(triggered()),this,SLOT(on_check_object_browser()));
    connect(ui->dockWidget_3,SIGNAL(visibilityChanged(bool)),this,SLOT(on_object_browser_closed(bool)));
    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    BuildObjectsToolBar();
    connect(ui->treeWidget,SIGNAL(customContextMenuRequested(const QPoint&)),this,SLOT(preparetreeviewMenu(const QPoint&)));
    connect(ui->treeWidget,SIGNAL(itemClicked(QTreeWidgetItem*, int)),this,SLOT(onTreeSelectionChanged(QTreeWidgetItem*)));

    connect(ui->actionOpen,SIGNAL(triggered()),this,SLOT(onopen()));
    connect(ui->actionSave,SIGNAL(triggered()),this,SLOT(onsave()));
    connect(ui->actionNew_Project,SIGNAL(triggered()),this,SLOT(onnew()));
    connect(this,SIGNAL(closed()),this,SLOT(onclosed()));
    connect(ui->actionLoad_a_new_template,SIGNAL(triggered()),this,SLOT(loadnewtemplate()));
    ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->tableView, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(tablePropShowContextMenu(const QPoint&)));

    ui->tableView->setItemDelegateForColumn(1,new Delegate(this,this));
    Populate_General_ToolBar();
}

MainWindow::~MainWindow()
{
    delete ui;
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
        int code = i1.data(CustomRoleCodes::Role::DescriptionCodeRole).toInt();
        QString variableName = i1.data(CustomRoleCodes::Role::VariableNameRole).toString();

        QMenu *estimatesMenu = new QMenu("Parameters");
        menu->addMenu(estimatesMenu);
        estimatesMenu->setEnabled(false);
        if (i2.data(CustomRoleCodes::Role::EstimateCode).toBool())
        {
            for (int i=0 ; i< GetSystem()->ParametersCount(); i++)
                estimatesMenu->addAction(QString::fromStdString(GetSystem()->Parameters()[i]->GetName()));// , this, SLOT(addParameter()));
            //addParameterIndex(i1); // tableProp->indexAt(pos));
            connect(estimatesMenu, SIGNAL(triggered(QAction*)), this, SLOT(addParameter(QAction*)));
            estimatesMenu->setEnabled(true);
        }

        menu->exec(ui->tableView->mapToGlobal(pos));


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

void MainWindow::on_object_browser_closed(bool visible)
{
    ui->actionObject_Browser->setChecked(visible);
}

bool MainWindow::BuildObjectsToolBar()
{
    for (unsigned int i = 0; i < system.GetAllBlockTypes().size(); i++)
    {
        qDebug() << QString::fromStdString(system.GetAllBlockTypes()[i]);
        QAction* action = new QAction(this);
        action->setObjectName(QString::fromStdString(system.GetAllBlockTypes()[i]));
        QIcon icon;
        icon.addFile(QString::fromStdString(qApp->applicationDirPath().toStdString() + "/../../resources/Icons/" + system.GetModel(system.GetAllBlockTypes()[i])->IconFileName()), QSize(), QIcon::Normal, QIcon::Off);
        action->setIcon(icon);
        ui->mainToolBar->addAction(action);
        action->setText(QString::fromStdString(system.GetAllBlockTypes()[i]));
        connect(action, SIGNAL(triggered()), this, SLOT(onaddblock()));
    }
    ui->mainToolBar->addSeparator();
    for (unsigned int i = 0; i < system.GetAllLinkTypes().size(); i++)
    {
        qDebug() << QString::fromStdString(system.GetAllLinkTypes()[i]);
        QAction* action = new QAction(this);
        action->setCheckable(true);
        action->setObjectName(QString::fromStdString(system.GetAllLinkTypes()[i]));
        QIcon icon;
        icon.addFile(QString::fromStdString(qApp->applicationDirPath().toStdString() + "/../../resources/Icons/" + system.GetModel(system.GetAllLinkTypes()[i])->IconFileName()), QSize(), QIcon::Normal, QIcon::Off);
        action->setIcon(icon);
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
                action->setIcon(icon);
                ui->mainToolBar->addAction(action);
                action->setText(QString::fromStdString(type));
                if (typecategory=="Sources")
                    connect(action, SIGNAL(triggered()), this, SLOT(onaddsource()));
                else if (typecategory == "Parameters")
                    connect(action, SIGNAL(triggered()), this, SLOT(onaddparameter()));
                else if (typecategory == "Objective Functions")
                    connect(action, SIGNAL(triggered()), this, SLOT(onaddobjectivefunction()));
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
    block.SetQuantities(system.GetMetaModel(),obj->objectName().toStdString());
    block.SetType(obj->objectName().toStdString());
    string name = CreateNewName(obj->objectName().toStdString());
    block.SetName(name);
    system.AddBlock(block);
    system.object(name)->SetName(name);
    Node *node = new Node(dView,&system);
    dView->repaint();
    system.object(name)->AssignRandomPrimaryKey();
    node->SetObject(system.object(name));
    RefreshTreeView();
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

void MainWindow::AddLink(const QString &LinkName, const QString &sourceblock, const QString &targetblock, const QString &type,  Edge* edge)
{
    Link link;
    link.SetQuantities(system.GetMetaModel(),type.toStdString());
    link.SetType(type.toStdString());
    link.SetName(LinkName.toStdString());
    system.AddLink(link,sourceblock.toStdString(),targetblock.toStdString());
    system.object(LinkName.toStdString())->SetName(LinkName.toStdString());
    system.object(LinkName.toStdString())->AssignRandomPrimaryKey();
    edge->SetObject(system.object(LinkName.toStdString()));
    foreach (QAction* action, ui->mainToolBar->actions())
    {
        action->setChecked(false);
    }
    RefreshTreeView();

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
    qDebug() << "source added! " << obj->objectName();
    system.object(name)->SetName(name);
    RefreshTreeView();
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
    qDebug() << "parameter added! " << obj->objectName();
    //system.object(name)->SetName(name);
    RefreshTreeView();
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
    qDebug() << "objective function added! " << obj->objectName();
    //system.object(name)->SetName(name);
    RefreshTreeView();

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
    for (int i=0; i<system.SettingsCount(); i++)
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

    for (int i=0; i<system.BlockCount(); i++)
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

    for (int i=0; i<system.LinksCount(); i++)
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

    for (int i=0; i<system.SourcesCount(); i++)
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

    for (int i=0; i<system.ParametersCount(); i++)
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

    qDebug()<<nd->data(0,Qt::UserRole);

    qDebug()<<pos<<nd->text(0);

    if (nd->data(0,Qt::UserRole)=="main")
    {
        if (nd->text(0)=="Parameters" || nd->text(0)=="Objective Functions")
        {
            QAction *AddAct = new QAction(QIcon(":/Resource/warning32.ico"), "Add " + nd->text(0), this);

            AddAct->setProperty("group",nd->text(0));
            AddAct->setStatusTip("Append " + nd->text(0));
            connect(AddAct, SIGNAL(triggered()), this, SLOT(onAddItemThroughTreeViewRightClick()));
            QMenu menu(this);
            menu.addAction(AddAct);
            QPoint pt(pos);
            menu.exec( tree->mapToGlobal(pos) );
        }
    return;

    }

    if (nd->data(0,Qt::UserRole)=="child")
    {
        QAction *DeleteAct = new QAction(QIcon(":/Resource/warning32.ico"), "Delete" , this);
        DeleteAct->setStatusTip("Delete");
        connect(DeleteAct, SIGNAL(triggered()), this, SLOT(onDeleteItem()));
        QMenu menu(this);
        menu.addAction(DeleteAct);
        QMenu* results = menu.addMenu("Results");
        for (unsigned int i = 0; i < system.object(nd->text(0).toStdString())->ItemswithOutput().size(); i++)
        {
            timeseriestobeshown = QString::fromStdString(system.object(nd->text(0).toStdString())->ItemswithOutput()[i]);
            QAction* graphaction = results->addAction(timeseriestobeshown);
            QVariant v = QVariant::fromValue(QString::fromStdString(system.object(nd->text(0).toStdString())->Variable(timeseriestobeshown.toStdString())->GetOutputItem()));
            graphaction->setData(v);
            //called_by_clicking_on_graphical_object = true;
            connect(graphaction, SIGNAL(triggered()), this, SLOT(showgraph()));
        }
        qDebug() << nd->data(0,CustomRoleCodes::Role::TypeRole).toString();
        if (nd->data(0,CustomRoleCodes::Role::TypeRole).toString() == "Objective Functions")
        {
            timeseriestobeshown = "Time Series";
            QAction* graphaction = results->addAction(timeseriestobeshown);
            QVariant v = QVariant::fromValue(QString::fromStdString(system.objectivefunction(nd->text(0).toStdString())->GetOutputItem()));
            graphaction->setData(v);
            //called_by_clicking_on_graphical_object = true;
            connect(graphaction, SIGNAL(triggered()), this, SLOT(showgraph()));


        }
        QPoint pt(pos);
        menu.exec( tree->mapToGlobal(pos) );

    }
}

void MainWindow::showgraph()
{
    QAction* act = qobject_cast<QAction*>(sender());
    QString item = act->data().toString();
    Plotter *plot = Plot(GetSystem()->GetOutputs()[item.toStdString()]);
    plot->SetYAxisTitle(act->text());

}

void MainWindow::onTreeSelectionChanged(QTreeWidgetItem *current)
{
    qDebug()<<current->data(0,Qt::UserRole);
    qDebug()<< current->text(0);

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
    actionzoomall->setObjectName("Open");
    QIcon iconzoomall;
    iconzoomall.addFile(QString::fromStdString(qApp->applicationDirPath().toStdString() + "/../../resources/Icons/Full_screen_view.png"), QSize(), QIcon::Normal, QIcon::Off);
    actionzoomall->setIcon(iconzoomall);
    ui->GeneraltoolBar->addAction(actionzoomall);
    actionzoomall->setText("Zoom Extends");
    actionzoomall->setToolTip("Zoom All");
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
    QAction* actionrun = new QAction(this);
    QAction* seperator = new QAction(this);
    seperator->setSeparator(true);
    ui->GeneraltoolBar->addAction(seperator);
    actionzoomall->setObjectName("Run Model");
    QIcon iconrun;
    iconrun.addFile(QString::fromStdString(qApp->applicationDirPath().toStdString() + "/../../resources/Icons/runmodel.png"), QSize(), QIcon::Normal, QIcon::Off);
    actionrun->setIcon(iconrun);
    ui->GeneraltoolBar->addAction(actionrun);
    actionrun->setText("Run Model");
    actionrun->setToolTip("Run Model");
    connect(actionrun, SIGNAL(triggered()), this, SLOT(onrunmodel()));
    QIcon iconoptimize;
    iconoptimize.addFile(QString::fromStdString(qApp->applicationDirPath().toStdString() + "/../../resources/Icons/optimize.png"), QSize(), QIcon::Normal, QIcon::Off);
    QAction* actionoptimize = new QAction(this);
    actionoptimize->setIcon(iconoptimize);
    ui->GeneraltoolBar->addAction(actionoptimize);
    actionoptimize->setText("Optimize");
    actionoptimize->setToolTip("Optimize");
    connect(actionoptimize, SIGNAL(triggered()), this, SLOT(onoptimize()));

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

    dView->fitInView(newRect,Qt::KeepAspectRatio);
}

void MainWindow::onsave()
{
    QString fileName = QFileDialog::getSaveFileName(this,
            tr("Save"), "",
            tr("script files (*.scr)"));
    if (fileName!="")
    {
        system.SavetoScriptFile(fileName.toStdString(),modelfilename);
        workingfolder = QFileInfo(fileName).canonicalPath();
    }

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
        system.ReadSystemSettingsTemplate(entitiesfilename);
        system.CreateFromScript(scr);
        system.ReadSystemSettingsTemplate(entitiesfilename);
        workingfolder = QFileInfo(fileName).canonicalPath();
    }
    RecreateGraphicItemsFromSystem();
    RefreshTreeView();
}

void MainWindow::RecreateGraphicItemsFromSystem()
{
    dView->DeleteAllItems();
    for (int i=0; i<system.BlockCount(); i++)
    {
        Node *node = new Node(dView,&system);
        dView->repaint();
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
        if (s_node && e_node);
        {
            Edge *edge = new Edge(s_node,e_node,dView );
            system.link(i)->AssignRandomPrimaryKey();
            edge->SetObject(system.link(i));
        }
    }
    onzoomall();
}

void MainWindow::onrunmodel()
{
    System copiedsystem(system);
    copiedsystem.SetSystemSettings();
    rtw = new RunTimeWindow();
    rtw->show();
    rtw->SetUpForForwardRun();
    copiedsystem.SetRunTimeWindow(rtw);
    copiedsystem.Solve(true);
    copiedsystem.GetOutputs().writetofile(workingfolder.toStdString() + "/outputs.txt");
    copiedsystem.errorhandler.Write(workingfolder.toStdString() + "/errors.txt");
    system.TransferResultsFrom(&copiedsystem);
    system.SetOutputItems();
}

void MainWindow::closeEvent (QCloseEvent *event)
{
    QMessageBox::StandardButton resBtn = QMessageBox::question( this, "QAquifolium",
                                                                tr("Are you sure?\n"),
                                                                QMessageBox::Cancel | QMessageBox::Yes,
                                                                QMessageBox::Yes);
    if (resBtn != QMessageBox::Yes) {
        event->ignore();
    } else {
        if (rtw) rtw->close();
        event->accept();
    }

}

void MainWindow::onoptimize()
{
    system.SetSystemSettings();
    optimizer = new CGA<System>(&system);
    optimizer->SetParameters(system.object("Optimizer"));
    system.SetAllParents();
    rtw = new RunTimeWindow();
    rtw->show();
    rtw->AppendText("Optimization Started ...");
    rtw->SetXRange(0,optimizer->GA_params.nGen);
    system.SetRunTimeWindow(nullptr);
    optimizer->SetRunTimeWindow(rtw);
    optimizer->optimize();
    system.TransferResultsFrom(&optimizer->Model_out);
    system.Parameters() = optimizer->Model_out.Parameters();
    system.SetOutputItems();
    rtw->AppendText("Optimization Finished!");
}

Plotter* MainWindow::Plot(CTimeSeries& plotitem)
{
    Plotter* plotter = new Plotter(this);
    plotter->PlotData(plotitem);
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
        ui->mainToolBar->clear();
        BuildObjectsToolBar();
        RefreshTreeView();
        modelfilename = fileName.toStdString();
    }

}


