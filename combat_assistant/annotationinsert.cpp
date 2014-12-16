#include "annotationinsert.h"
#include "ui_annotationinsert.h"

AnnotationInsert::AnnotationInsert(QWidget *parent, QString default_name, int default_duration) :
    QDialog(parent),
    ui(new Ui::AnnotationInsert)
{
    ui->setupUi(this);

    ui->leDescription->setText(default_name);
    ui->sbDuration->setValue(default_duration);
}

AnnotationInsert::~AnnotationInsert()
{
    delete ui;
}

QString AnnotationInsert::getDescription() const {
    return ui->leDescription->text();
}

int AnnotationInsert::getDuration() const {
    return ui->sbDuration->value();
}
