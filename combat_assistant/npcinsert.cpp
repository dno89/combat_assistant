#include "npcinsert.h"
#include "ui_npcinsert.h"

npcinsert::npcinsert(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::npcinsert)
{
    ui->setupUi(this);
}

npcinsert::~npcinsert()
{
    delete ui;
}

QString npcinsert::GetUName() const {
    return ui->teName->toPlainText();
}

std::string npcinsert::GetPF() const {
    return ui->tePF->toPlainText().toStdString();
}

int npcinsert::GetIni() const {
    return ui->sbIni->value();
}

bool npcinsert::GetRD20() const {
    return ui->cbRd20->checkState() == Qt::Checked;
}
