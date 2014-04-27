#include "monsterinsert.h"
#include "ui_monsterinsert.h"

MonsterInsert::MonsterInsert(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MonsterInsert)
{
    ui->setupUi(this);
}

MonsterInsert::~MonsterInsert()
{
    delete ui;
}
