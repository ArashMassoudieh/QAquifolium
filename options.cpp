#include "options.h"
#include "ui_Options.h"
#include "mainwindow.h"
#include "node.h"

OptionsDialog::OptionsDialog(MainWindow *_parent) :
    QDialog(_parent),
    ui(new Ui::Dialog)
{
    parent = _parent;
    ui->setupUi(this);
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(on_ok_clicked()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(on_cancel_clicked()));
    ui->doubleSpinBox->setValue(parent->GetDiagramView()->fontfactor);
}

OptionsDialog::~OptionsDialog()
{
    delete ui;
}

void OptionsDialog::on_ok_clicked()
{
    if (parent)
    {
        parent->GetDiagramView()->fontfactor = ui->doubleSpinBox->value();
        parent->GetDiagramView()->update();
        parent->GetDiagramView()->scene()->update(parent->GetDiagramView()->sceneRect());
        for (int i=0; i<parent->GetDiagramView()->Nodes().size(); i++)
            parent->GetDiagramView()->Nodes()[i]->update();

    }
    this->close();
}

void OptionsDialog::on_cancel_clicked()
{
    this->close();
}

