#ifndef ANNOTATIONINSERT_H
#define ANNOTATIONINSERT_H

#include <QDialog>

namespace Ui {
class AnnotationInsert;
}

class AnnotationInsert : public QDialog
{
    Q_OBJECT

public:
    explicit AnnotationInsert(QWidget *parent = 0);
    ~AnnotationInsert();

    ///My functions
    QString getDescription() const;
    int getDuration() const;

private:
    Ui::AnnotationInsert *ui;
};

#endif // ANNOTATIONINSERT_H
