#ifndef NPCINSERT_H
#define NPCINSERT_H

#include <QDialog>

namespace Ui {
class npcinsert;
}

class npcinsert : public QDialog
{
    Q_OBJECT
    
public:
    explicit npcinsert(QWidget *parent = 0);
    ~npcinsert();

    QString GetUName() const;
    int GetIni() const;
    std::string GetPF() const;
    bool GetRD20() const;
    
private:
    Ui::npcinsert *ui;
};

#endif // NPCINSERT_H
