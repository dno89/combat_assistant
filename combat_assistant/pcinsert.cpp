#include "pcinsert.h"
#include "ui_pcinsert.h"

PCInsert::PCInsert(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PCInsert)
{
    ui->setupUi(this);
}

PCInsert::~PCInsert()
{
    delete ui;
}

QString PCInsert::GetName() const {
    return ui->leName->text();
//    return ui->teName->toPlainText();
}

int PCInsert::GetInitiative() const {
    return ui->sbInit->value();
}
