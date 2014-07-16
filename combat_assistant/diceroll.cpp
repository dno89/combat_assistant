#include "diceroll.h"
#include "ui_diceroll.h"
#include <QShowEvent>

DiceRoll::DiceRoll(QWidget *parent) :
    QDialog(parent, Qt::FramelessWindowHint),
    ui(new Ui::DiceRoll)
{
    ui->setupUi(this);
    setSizeGripEnabled(false);
//    setWindowFlags(Qt::WindowStaysOnTopHint);
    //setWindowFlags(Qt::FramelessWindowHint);
}

DiceRoll::~DiceRoll()
{
    delete ui;
}

QString DiceRoll::getDiceText() const {
    return ui->leDice->text();
}

void DiceRoll::showEvent(QShowEvent * event) {
    ui->leDice->selectAll();
}
