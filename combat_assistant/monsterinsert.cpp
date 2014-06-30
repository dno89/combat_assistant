#include "monsterinsert.h"
#include "ui_monsterinsert.h"
#include "utils.h"
#include <boost/regex.hpp>

MonsterInsert::MonsterInsert(const txtDatabase& db, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MonsterInsert),
    m_db(db)
{
    ui->setupUi(this);

    //setup the splitter
    ui->upperHSplitter->setStretchFactor(0, 2);
    ui->upperHSplitter->setStretchFactor(1, 3);
    ui->upperHSplitter->setStretchFactor(2, 5);

    //setup the model
    ui->lvMonsters->setModel(&m_filtered_model);

    //selection changed
    connect(ui->lvMonsters->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(lvMonster_currentChanged(const QModelIndex&,const QModelIndex&)));

    //start with all monsters
    txtDatabase::DatabaseType db2 = m_db.getDatabase();
    txtDatabase::LabelMapType lbl = m_db.getLabelMap();

    SortTxtDB(db2, lbl, "Name");
    m_filtered_model.setDatabase(db2);
}

void MonsterInsert::lvMonster_currentChanged(const QModelIndex &cur, const QModelIndex &prev) {
    int index = cur.row();
    std::string description = m_filtered_model.getField(index, m_db.getLabelMap().at("FullText")).toStdString();
    //process description
    description = formatDescription(description);

    QString html = QString(R"+(<!DOCTYPE html>
                                  <html xmlns="http://www.w3.org/1999/xhtml" lang="en">
                                      <head>
                                          <title>Monster</title>
                                          <link href="PF.css" rel="stylesheet" type="text/css"/>
                                          <meta http-equiv="Content-Type" content="text/html; charset=utf-8"/>
                                          <meta name="viewport" content="width=device-width, minimum-scale=1, maximum-scale=1"/>
                                      </head>
                                      <body>
                                      %1
                                      </body>
                                  </html>)+").arg(QString::fromStdString(description));

//    std::cout << std::endl << std::endl << html.toStdString() << std::endl << std::endl;
    ui->tbDescription->setHtml(html);
}

MonsterInsert::~MonsterInsert()
{
    delete ui;
}

txtDatabase::RecordType MonsterInsert::getSelectedMonster() const {
    return m_filtered_model.getRecord(ui->lvMonsters->currentIndex().row());
}

void MonsterInsert::on_leRegex_returnPressed() {

}

void MonsterInsert::on_leRegex_textChanged(const QString &arg1) {
    boost::regex r(arg1.toStdString(), (!ui->cbCaseSensitive->isChecked())?boost::regex::icase:0 | boost::regex::extended);
    txtDatabase::DatabaseType db = m_db.filter("Name", r);
    SortTxtDB(db, m_db.getLabelMap(), "Name");
//    txtDatabase::LabelMapType lbl = m_db.getLabelMap();
//    std::sort(db.begin(), db.end(), [&lbl](txtDatabase::DatabaseType::const_reference e1, txtDatabase::DatabaseType::const_reference e2){return stod(e1[lbl["CR"]]) < stod(e2[lbl["CR"]]);});
    m_filtered_model.setDatabase(db);

}

int MonsterInsert::getQty() const {
    return ui->sbQty->value();
}

bool MonsterInsert::isSelected() const {
    return ui->lvMonsters->selectionModel()->hasSelection();
}

void MonsterInsert::on_cbCaseSensitive_clicked() {
    on_leRegex_textChanged(ui->leRegex->text());
}
