#ifndef MONSTERINSERT_H
#define MONSTERINSERT_H

#include <QDialog>

namespace Ui {
class MonsterInsert;
}

class MonsterInsert : public QDialog
{
    Q_OBJECT

public:
    explicit MonsterInsert(QWidget *parent = 0);
    ~MonsterInsert();

private:
    Ui::MonsterInsert *ui;
};

#endif // MONSTERINSERT_H
