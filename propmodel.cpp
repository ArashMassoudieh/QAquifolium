#include "propmodel.h"
#include <QDebug>
#include <QFont>
#include <QBrush>
#include "enums.h"

PropModel::PropModel(QuanSet* _quanset, QObject *parent)
    : QAbstractTableModel(parent)
{
    quanset = _quanset;
}

int PropModel::rowCount(const QModelIndex &i) const
{
   return int (quanset->size());
}

int PropModel::columnCount(const QModelIndex &i) const
{
    return 2;
}

QVariant PropModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    int col = index.column();
   // generate a log message when this method gets called
   qDebug() << QString("row %1, col%2, role %3")
           .arg(row).arg(col).arg(role);

   switch (role) {
   case Qt::DisplayRole:
       if (col == 0) return QString::fromStdString(quanset->GetVar(index.row())->GetName());
       if (col == 1) return QString::fromStdString(quanset->GetVar(index.row())->GetProperty());
       break;
   case Qt::FontRole:
       if (col == 1) {
           QFont boldFont;
           boldFont.setBold(true);
           return boldFont;
       }
       break;
   case Qt::BackgroundRole:
       if (col == 0)
           return QBrush(Qt::red);
       break;
   case Qt::CheckStateRole:
       //if (row == 1 && col == 0) //add a checkbox to cell(1,0)
       //    return Qt::Checked;
       break;
   case CustomRoleCodes::TypeRole:
       return QString::fromStdString(quanset->GetVar(index.row())->Delegate());
       break;
   }
   return QVariant();
}

QVariant PropModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Horizontal) {
            switch (section)
            {
            case 0:
                return QString("Property");
            case 1:
                return QString("Value");
            }
        }
    }
    return QVariant();
}

bool PropModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
     if (role == Qt::EditRole)
     {
         QString result = value.toString();
         quanset->GetVar(index.row())->SetProperty(result.toStdString());
         emit editCompleted(result);
     }
     return true;
}

Qt::ItemFlags PropModel::flags(const QModelIndex & /*index*/) const
{
    return Qt::ItemIsSelectable |  Qt::ItemIsEditable | Qt::ItemIsEnabled ;
}

