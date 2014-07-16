#ifndef DICEROLL_H
#define DICEROLL_H

#include <QDialog>

namespace Ui {
class DiceRoll;
}

class DiceRoll : public QDialog
{
    Q_OBJECT
public:
    explicit DiceRoll(QWidget *parent = 0);
    ~DiceRoll();

    QString getDiceText() const;

protected:
    void showEvent(QShowEvent * event);

private:
    Ui::DiceRoll *ui;
};

#endif // DICEROLL_H
