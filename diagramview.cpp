#include "diagramview.h"
#include "mainwindow.h"
#include "node.h"
#include "edge.h"
#include "QMouseEvent"
#include "QDebug"
#include "QMenu"

DiagramView::DiagramView(QWidget* parent, MainWindow *_mainwindow) : QGraphicsView(parent)
{
    mainwindow = _mainwindow;
    MainGraphicsScene = new QGraphicsScene(this);
    MainGraphicsScene->setItemIndexMethod(QGraphicsScene::NoIndex);
    MainGraphicsScene->setSceneRect(0, 0, 2000, 2000);
    setScene(MainGraphicsScene);
    setCacheMode(CacheBackground);
    setViewportUpdateMode(BoundingRectViewportUpdate);
    setRenderHint(QPainter::Antialiasing);
    setTransformationAnchor(AnchorUnderMouse);
    setMode(0);
    QObject::connect(MainGraphicsScene, SIGNAL(changed(const QList<QRectF>)), this, SLOT(sceneChanged()));

}

void DiagramView::mousePressEvent(QMouseEvent *event)
{
    if (mainWindow()->propModel())
        mainWindow()->resetPropModel();
    Node *node = qgraphicsitem_cast<Node*> (itemAt(event->pos())); //Get the item at the position
    if (node)
        if (node->itemType==Object_Types::Block)
        {
            qDebug() << "Name: "<< node->Name()<<" Flag:" << node->flags() << "enabled:" << node->isEnabled() << "active:" << node->isActive();
        }
    Edge *edge = qgraphicsitem_cast<Edge*> (itemAt(event->pos())); //Get the item at the position
    if (edge)
        if (edge->itemType==Object_Types::Connector)
        {
            qDebug() << "Name: " << edge->Name() << " Flag:" << edge->flags() << "enabled:" << edge->isEnabled() << "active:" << edge->isActive();
        }

    if (event->buttons() == Qt::MiddleButton && Operation_Mode == Operation_Modes::NormalMode)
    {
        setMode(Operation_Modes::Pan);
        QMouseEvent *newEvent = new QMouseEvent(event->type(), event->localPos(), Qt::LeftButton, Qt::MouseButtons(1), event->modifiers());
        QGraphicsView::mousePressEvent(newEvent);
        delete newEvent;
        return;
    }
    QGraphicsView::mousePressEvent(event); //Call the ancestor
    if (Operation_Mode == Operation_Modes::NormalMode)
    {
        setDragMode((node || edge) ? QGraphicsView::NoDrag : QGraphicsView::RubberBandDrag);

        if (node)
            if (node->itemType == Object_Types::Block)
            {
                int xx = mapToScene(event->pos()).x();	int yy = int(mapToScene(event->pos()).y());
                if (event->buttons() == Qt::LeftButton)
                {
                    if (event->modifiers() && Qt::ControlModifier) {
//						node->setFlag(QGraphicsItem::ItemIsMovable, false);
//						node->setSelected(true);
                    }
                    else if (node->corner(xx, yy)) {
                        setDragMode(QGraphicsView::NoDrag);
                        setMode(Operation_Modes::resizeNode);
                        resizenode = node;
                        resizecorner = node->corner(xx, yy);
                        node->setFlag(QGraphicsItem::ItemIsMovable, false);
                    }
                    else if (node->edge(xx, yy) && getselectedconnectfeature()!="") {
                        node->setFlag(QGraphicsItem::ItemIsMovable, false);
                        Node1 = node;
                        tempRay = new Ray();
                        MainGraphicsScene->addItem(tempRay);
                        setMode(Operation_Modes::Node1_selected);
                    }
                    else
                        node->setFlag(QGraphicsItem::ItemIsMovable, true);
                }
            }
        if (event->buttons() == Qt::LeftButton && Operation_Mode == Operation_Modes::Draw_Connector)
        {
            if (node) {
                if (node->itemType == Object_Types::Block) {
                    node->setFlag(QGraphicsItem::ItemIsMovable, false);
                    Node1 = node;
                    tempRay = new Ray();
                    MainGraphicsScene->addItem(tempRay);
                    setMode(Operation_Modes::Node1_selected);
                }
            }
        }
    }

    //QGraphicsView::mousePressEvent(event);
}
void DiagramView::mouseMoveEvent(QMouseEvent *event)
{
    //	//qDebug() << "Mouse MOVE, button: " << event->button() << ", modifier: " << event->modifiers() << ", buttons: " << event->buttons();
    _x = mapToScene(event->pos()).x();
    _y = mapToScene(event->pos()).y();
    int xx = _x;// mapToScene(event->pos()).x();
    int yy = _y;// mapToScene(event->pos()).y();

    if (event->buttons() == Qt::MiddleButton && Operation_Mode == Operation_Modes::Pan)
    {
        QMouseEvent *newEvent = new QMouseEvent(event->type(), event->localPos(), Qt::LeftButton, Qt::MouseButtons(1), event->modifiers());
        //		QGraphicsView::mousePressEvent(newEvent);
        QGraphicsView::mouseMoveEvent(newEvent);
        delete newEvent;
        return;
    }

    if (event->buttons() == Qt::LeftButton && Operation_Mode == Operation_Modes::NormalMode && dragMode() == DragMode::RubberBandDrag)
    {
        QGraphicsView::mouseMoveEvent(event);
        //qDebug() << event->x() << rubberBandRect().x();
        if (event->x() > rubberBandRect().x()) //Dragging to the right
        {
            for (Edge* item : edges(items(rubberBandRect())))
                item->setFlag(QGraphicsItem::ItemIsSelectable, false);
        }
        else
        {
            for (Node* item : nodes(items(rubberBandRect())))
                item->setFlag(QGraphicsItem::ItemIsSelectable, false);

        }
        return;
    }

    bool cursorModeNormal = true;
    setToolTip("");
    QGraphicsView::mouseMoveEvent(event);
    QString txt;
    Node *n1 = qgraphicsitem_cast<Node*> (itemAt(event->pos())); //Get the item at the position
    if (n1) //itemType == Object_Types::Block)
    {
        txt = QString("%1: %2").arg(QString::fromStdString(n1->object()->GetType())).arg(QString::fromStdString(n1->object()->GetName()));
        QString toolTip = QString("Type: %1\nName: %2").arg(QString::fromStdString(n1->object()->GetType())).arg(QString::fromStdString(n1->object()->GetName()));

        //if (n1->errors.count()) toolTip.append(QString("\n%1 Error(s)").arg(n1->errors.count()));
        //if (n1->warnings.count()) toolTip.append(QString("\n%1 Warning(s)").arg(n1->warnings.count()));
        setToolTip(toolTip);
    }
    //for (Node* n in Nodes())
    //	if (n!=n1) n->setBold(false);
    Edge *e1 = qgraphicsitem_cast<Edge*> (itemAt(event->pos())); //Get the item at the position
    if (e1)
        //	if (c2 && c2->itemType == Object_Types::Connector && c2->dist(mapToScene(event->pos())) < 120) //GUI == "Connector"
    {
        //		e1->setBold(true);
        //		e1->update();
        //txt = QString("%1, %2: %3").arg(c2->ObjectType().GuiObject).arg(c2->ObjectType().ObjectType).arg(c2->Name());
        QString toolTip = QString("%1, %2").arg(QString::fromStdString(n1->object()->GetType())).arg(QString::fromStdString(n1->object()->GetName()));
        //QString toolTip = QString("Type: %1\nName: %2").arg(c1->ObjectType().ObjectType).arg(c1->Name());
        //toolTip.append(QString("\nBottom Elevation: %1").arg(c1->val("z0").toStringUnit()));
        //if (c1->errors.count()) toolTip.append(QString("\n%1 Error(s)").arg(c1->errors.count()));
        //if (c1->warnings.count()) toolTip.append(QString("\n%1 Warning(s)").arg(c1->warnings.count()));
        setToolTip(toolTip);

    }
    //for (Edge*e in Edges())
    //	if (e!=e1) e->setBold(false);
    //update();
    emit Mouse_Pos(_x, _y, txt);
    if (Operation_Mode == Operation_Modes::Node1_selected)
    {
        Node *child = qgraphicsitem_cast<Node*> (itemAt(event->pos())); //Get the item at the position
        if (child)
            if ((child->itemType == Object_Types::Block) && (Node1->Name() != child->Name()))
            {
                tempRay->setValidation(true);
                tempRay->adjust(Node1, child);
            }
        if (!child)
        {
            tempRay->setValidation(false);
            QPointF p = QPointF(mapToScene(event->pos()));
            tempRay->adjust(Node1, &p);
        }
    }
    if (Operation_Mode == Operation_Modes::NormalMode && dragMode()==DragMode::NoDrag)
    {
        Node *node = qgraphicsitem_cast<Node*> (itemAt(event->pos()));

        if (node)
            if (node->itemType == Object_Types::Block)
            {
                if ((node->corner(xx, yy) == topleft) || (node->corner(xx, yy) == bottomright))
                {
                    setCursor(Qt::SizeFDiagCursor);
                    cursorModeNormal = false;
                }
                if ((node->corner(xx, yy) == topright) || (node->corner(xx, yy) == bottomleft))
                {
                    setCursor(Qt::SizeBDiagCursor);
                    cursorModeNormal = false;
                }

                if (node->corner(xx, yy) == none)
                {	if (node->edge(xx, yy) != noside)
                    {
                        setCursor(Qt::CrossCursor);
                        cursorModeNormal = false;
                    }
                    else
                    {
                        setCursor(Qt::ArrowCursor);
                        cursorModeNormal = false;
                    }
                }
            }
    }
    if ((Operation_Mode == Operation_Modes::resizeNode) && (event->buttons() == Qt::LeftButton))
    {
        int xx = mapToScene(event->pos()).x();
        int yy = mapToScene(event->pos()).y();
        int px = resizenode->x();
        int py = resizenode->y();
        int pw = resizenode->Width();
        int ph = resizenode->Height();
        int minH = resizenode->minH, minW = resizenode->minW;

        if (resizecorner == topleft && (px - xx + pw) > minW && (py - yy + ph) > minH)
        {
            resizenode->setX(xx);
            resizenode->setWidth(px - xx + pw);
            resizenode->setY(yy);
            resizenode->setHeight(py - yy + ph);
        }
        if (resizecorner == bottomleft && (px - xx + pw) > minW && (yy - py) > minH)
        {
            resizenode->setX(xx);
            resizenode->setWidth(px - xx + pw);
            //resizenode->setY(yy);
            resizenode->setHeight(yy - py);
        }
        if (resizecorner == topright && (xx - px) > minW && (py - yy + ph) > minH)
        {
            //resizenode->setX(xx);
            resizenode->setWidth(xx - px);
            resizenode->setY(yy);
            resizenode->setHeight(py - yy + ph);
        }
        if (resizecorner == bottomright && (xx - px) > minW && (yy - py) > minH)
        {
            //resizenode->setX(xx);
            resizenode->setWidth(xx - px);
            //resizenode->setY(yy);
            resizenode->setHeight(yy - py);
        }
        resizenode->update();
        for(Edge *edge : resizenode->edges())
            edge->adjust();
    }
    if (cursorModeNormal)
        setCursor(Qt::ArrowCursor);
}
void DiagramView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && Operation_Mode == Operation_Modes::NormalMode && dragMode() == DragMode::RubberBandDrag)
    {
        for (Node* item : Nodes())
            item->setFlag(QGraphicsItem::ItemIsSelectable, true);
        for (Edge* item : Edges())
            item->setFlag(QGraphicsItem::ItemIsSelectable, true);
    }
    if (event->button() == Qt::MiddleButton && Operation_Mode == Operation_Modes::Pan)
    {
        setMode(Operation_Modes::NormalMode);
        QMouseEvent *newEvent = new QMouseEvent(event->type(), event->localPos(), Qt::LeftButton, Qt::MouseButtons(1), event->modifiers());
        QGraphicsView::mouseReleaseEvent(newEvent);
        delete newEvent;
        return;

    }
    QGraphicsView::mouseReleaseEvent(event); //Call the ancestor

    switch (Operation_Mode) {
    case Operation_Modes::resizeNode:
    {
        setMode(NormalMode, true);
        break;
    }
    case Operation_Modes::NormalMode:
    {
        if (dragMode() != DragMode::RubberBandDrag)
        {

        }
        Node *node = qgraphicsitem_cast<Node*> (itemAt(event->pos())); //Get the item at the position
        Edge *edge = qgraphicsitem_cast<Edge*> (itemAt(event->pos())); //Get the item at the position
        if (event->button() == Qt::LeftButton && dragMode()!=DragMode::RubberBandDrag)
            if (event->modifiers()) {
                if (node)
                if (node->itemType==Object_Types::Block)
                {
                    if (selectedNodes().contains(node))
                        node->setSelected(true);
                    else
                        node->setSelected(false);
                }
                if (edge)
                if (edge->itemType==Object_Types::Connector)
                {
                    if (selectedEdges().contains(edge))
                        edge->setSelected(false);
                    else
                        edge->setSelected(true);
                }
            }
            else {
                if (node)
                    for (Node * node : Nodes())
                            node->setFlag(QGraphicsItem::ItemIsMovable, false);
                if (edge)
                {
                    if (edge->dist(mapToScene(event->pos())) < 120)
                        edge->setSelected(true);
                }
                //if (!node && !edge) deselectAll();
            }
            break;
    }
    case Operation_Modes::Node1_selected:
    {
        Node1->setFlag(QGraphicsItem::ItemIsMovable);
        setMode(1);
        MainGraphicsScene->removeItem(tempRay);
        delete tempRay;
        Node *child = static_cast<Node*> (itemAt(event->pos())); //Get the item at the position
        if (!child)	break;
        if (child->itemType != Object_Types::Block) break;
        if (Node1 != child) new Edge(Node1, child, connect_feature, this);
        break;
    }
    //	default:
    }
    bool changed = false;
    for (Node *n : Nodes())
    {
        if (specs[n->Name()]["x"].toFloat() != n->x() ||
            specs[n->Name()]["y"].toFloat() != n->y() ||
            specs[n->Name()]["w"].toInt() != n->Width() ||
            specs[n->Name()]["h"].toInt() != n->Height())
        {
            changed = true;
            specs[n->Name()]["x"] = QString::number(n->x());
            specs[n->Name()]["y"] = QString::number(n->y());
            specs[n->Name()]["w"] = QString::number(n->Width());
            specs[n->Name()]["h"] = QString::number(n->Height());
            n->object()->SetVal("x",n->x());
            n->object()->SetVal("y",n->x());
            n->object()->SetVal("Width", n->Width());
            n->object()->SetVal("Height", n->Height());
        }
    }
    //if (changed)
        //gwChanged();

}
void DiagramView::updateNodeCoordinates()
{
    for (Node *n : Nodes())
    {
        if (specs[n->Name()]["x"].toFloat() != n->x() ||
            specs[n->Name()]["y"].toFloat() != n->y() ||
            specs[n->Name()]["w"].toFloat() != n->Width() ||
            specs[n->Name()]["h"].toFloat() != n->Height())
        {
            specs[n->Name()]["x"] = QString::number(n->x());
            specs[n->Name()]["y"] = QString::number(n->y());
            specs[n->Name()]["w"] = QString::number(n->Width());
            specs[n->Name()]["h"] = QString::number(n->Height());
        }
    }
}

QList<Node*> DiagramView::nodes(const QList<QGraphicsItem*> items) const
{
    QList<Node *> nodes;
    for(QGraphicsItem *item : items)
    {
        if (Node *node = qgraphicsitem_cast<Node *>(item))
            nodes << node;
    }
    return nodes;
}
QList<Edge*> DiagramView::edges(const QList<QGraphicsItem*>items) const
{
    QList<Edge *> edges;
    for(QGraphicsItem *item : items)
    {
        if (Edge *edge = qgraphicsitem_cast<Edge *>(item))
            edges << edge;
    }
    return edges;
}

QList<Node*> DiagramView::selectedNodes() const
{
    QList<Node *> nodes;
    for(QGraphicsItem *item : scene()->items())
    {
        if (Node *node = qgraphicsitem_cast<Node *>(item))
            if (node->isSelected())
                nodes << node;
    }
    return nodes;
}

QList<Edge*> DiagramView::selectedEdges() const
{
    QList<Edge *> edges;
    for(QGraphicsItem *item : scene()->items())
    {
        if (Edge *edge = qgraphicsitem_cast<Edge *>(item))
            if (edge->isSelected())
                edges << edge;
    }
    return edges;
}

QList<Node*> DiagramView::Nodes() const
{
    QList<Node *> nodes;
    foreach(QGraphicsItem *item, scene()->items()) {
        if (Node *node = qgraphicsitem_cast<Node *>(item))
        {
            nodes << node;
        }
    }
    return(nodes);
}

QList<Edge *> DiagramView::Edges() const
{
    QList<Edge *> edges;
    for(QGraphicsItem *item : scene()->items())
    {
        if (Edge *edge = qgraphicsitem_cast<Edge *>(item))
            edges << edge;
    }
    return(edges);
}

Operation_Modes DiagramView::setMode(int i)
{
    return(setMode(Operation_Modes::NormalMode, true));
}

Operation_Modes DiagramView::setMode(Operation_Modes OMode, bool back)
{
    static QList<Operation_Modes> Modes;
    bool select, move;
    if (!back)
    {
        Operation_Mode = OMode;
        Modes.push_back(OMode);
    }
    else
    {
        if (Modes.size() < 2)
        {
            Operation_Mode = Operation_Modes::NormalMode;
            setModeCursor();
            return(Operation_Mode);
        }
        Modes.pop_back();
        Operation_Mode = Modes[Modes.size()-1];
    }
    switch (Operation_Mode) {
    case Operation_Modes::Draw_Connector:
        setModeCursor();
        setDragMode(QGraphicsView::NoDrag);
        move = false;
        select = false;
        break;
    case Operation_Modes::Pan:
        setModeCursor();
        setDragMode(QGraphicsView::ScrollHandDrag);
        move = true;
        select = false;
        break;
/*	case Operation_Modes::Pan_Started:
        setCursor(Qt::ClosedHandCursor);
        break;*/
    case Operation_Modes::Node1_selected:
        setModeCursor();
        setDragMode(QGraphicsView::NoDrag);
        move = false;
        select = false;
        break;
    case Operation_Modes::resizeNode:
        //setCursor(Qt::CrossCursor);
        move = false;
        select = false;
        break;
    case Operation_Modes::NormalMode:
        setModeCursor();
        setDragMode(QGraphicsView::RubberBandDrag);
        select = true;
        move = false;
        break;
    }
    foreach(Node *node, Nodes())
    {
        node->setFlag(QGraphicsItem::ItemIsMovable, move);
        node->setFlag(QGraphicsItem::ItemIsSelectable, select);
    }
    foreach(Edge *edge, Edges())
        edge->setFlag(QGraphicsItem::ItemIsSelectable, select);
    return (Operation_Mode);
}

Operation_Modes DiagramView::setModeCursor()
{
    switch (Operation_Mode) {
    case Operation_Modes::Draw_Connector:
        setCursor(Qt::CrossCursor);
        break;
    case Operation_Modes::Pan:
        setCursor(Qt::OpenHandCursor);
        break;
    case Operation_Modes::Node1_selected:
        setCursor(Qt::CrossCursor);
        break;
    case Operation_Modes::resizeNode:
        break;
    case Operation_Modes::NormalMode:
        setCursor(Qt::ArrowCursor);
        break;
    }
    return (Operation_Mode);
}


void DiagramView::nodeContextMenuRequested(Node* n, QPointF pos, QMenu *menu)
{
    bool called_by_clicking_on_graphical_object = false;
    if (!menu) {
        menu = new QMenu;
        menu->addAction("Delete");
        called_by_clicking_on_graphical_object = true;
    }
    menu->addAction("Select");
    QMap < QAction *, QStringList> menuKey;

    QAction *selectedAction;
    if (called_by_clicking_on_graphical_object)
        selectedAction = menu->exec(mapToGlobal(mapFromScene(pos.toPoint())));
    else
        selectedAction = menu->exec(pos.toPoint());

    if (selectedAction != nullptr)
    {
        if (selectedAction->text() == "Select")
            n->setSelected(true);
        qDebug()<<selectedAction->text();
        if (selectedAction->text().left(6) == "Delete")
        {
        qDebug()<<"Here we have to delete the object";
        // Need work
        };


        if (menuKey.keys().contains(selectedAction))
        {

        }
    }

}

void DiagramView::edgeContextMenuRequested(Edge* e, QPointF pos, QMenu *menu)
{
    QAction *deleteAction;
    bool called_by_clicking_on_graphical_object = false;
    if (!menu) {
        menu = new QMenu();
        deleteAction = menu->addAction("Delete");
        called_by_clicking_on_graphical_object = true;
    }
    QAction *markAction = menu->addAction("Select");

    QAction *selectedAction;
    if (called_by_clicking_on_graphical_object)
        selectedAction = menu->exec(mapToGlobal(mapFromScene(pos.toPoint())));
    else
        selectedAction = menu->exec(pos.toPoint());


}
