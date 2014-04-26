#include "annotationinsert.h"
#include "ui_annotationinsert.h"

AnnotationInsert::AnnotationInsert(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AnnotationInsert)
{
    ui->setupUi(this);
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
