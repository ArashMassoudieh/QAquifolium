#ifndef DIAGRAMVIEW_H
#define DIAGRAMVIEW_H

#include <QWidget>
#include <QGraphicsView>
#include "enums.h"
#include "ray.h"

class MainWindow;
class Node;
class Edge;

struct colorCodeData
{
    bool nodes = false, edges = false;
};

class DiagramView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit DiagramView(QWidget* parent, MainWindow *_mainwindow);
    QGraphicsScene *MainGraphicsScene;
    MainWindow *mainWindow() {return  mainwindow;}
    colorCodeData colorCode;
    QString getselectedconnectfeature() {return connect_feature;} //returns the type of connector to be added.
    void setconnectfeature(QString cf) {connect_feature = cf;} //sets the type of connector to be used
    Operation_Modes setMode(Operation_Modes OMode = Operation_Modes::NormalMode, bool back = false);
private:
    MainWindow *mainwindow;
    QList<Node*> nodes();
    QList<Edge*> edges();
    QString connect_feature = "";
    Operation_Modes Operation_Mode;
    Node *resizenode;
    corners resizecorner;
    Node *Node1; // , *Node2;
    Ray *tempRay;
    int _x, _y;
    QList<Node*> nodes(const QList<QGraphicsItem*> items) const;
    QList<Edge*> edges(const QList<QGraphicsItem*>items) const;

signals:

public slots:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

};

#endif // DIAGRAMVIEW_H
