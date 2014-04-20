#ifndef PCINSERT_H
#define PCINSERT_H

#include <QDialog>

namespace Ui {
class PCInsert;
}

class PCInsert : public QDialog
{
    Q_OBJECT
    
public:
    explicit PCInsert(QWidget *parent = 0);
    ~PCInsert();
    
    QString GetName() const;
    int GetInitiative() const;

private:
    Ui::PCInsert *ui;
};

#endif // PCINSERT_H
