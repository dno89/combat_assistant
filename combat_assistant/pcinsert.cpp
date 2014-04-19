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

std::string PCInsert::GetName() const {
    return ui->teName->toPlainText().toLocal8Bit().data();
}

int PCInsert::GetInitiative() const {
    return ui->sbInit->value();
}
