#ifndef MONSTERINSERT_H
#define MONSTERINSERT_H

#include <QDialog>
#include <QListView>
#include "txtDatabase.h"
#include "structs.h"

namespace Ui {
class MonsterInsert;
}

class MonsterListModel : public QAbstractListModel {
public:
    MonsterListModel(int default_field_index = 0) : m_findex(default_field_index)
        {}

    //virtual functions
    int rowCount(const QModelIndex &parent = QModelIndex()) const {
        return m_db.size();
    }
    int columnCount(const QModelIndex &parent = QModelIndex()) const {
        return 1;
    }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const {
        assert(index.column() == 0);

        if(m_db.size() == 0) return QVariant();

        if(role == Qt::DisplayRole) {
            int i = index.row();
            return QString::fromStdString(m_db[i][m_findex]);
        } else if(role == Qt::DecorationRole) {
            return QIcon("");
        }

        return QVariant();
    }
    void Refresh() {
        QModelIndex first = this->createIndex(0, 0, nullptr), last = this->createIndex(m_db.size()-1, 0, nullptr);
        emit dataChanged(first, last);
    }
    QString getField(int record_index, int field_index) const {
        return QString::fromStdString(m_db[record_index][field_index]);
    }
    txtDatabase::RecordType getRecord(int index) const {
        return m_db[index];
    }
    void setDatabase(const txtDatabase::DatabaseType& db) {
        m_db = db;
        Refresh();
    }

private:
    ///data
    txtDatabase::DatabaseType m_db;
    int m_findex;
};

class MonsterInsert : public QDialog
{
    Q_OBJECT

public:
    explicit MonsterInsert(const txtDatabase& db, QWidget *parent = 0);
    ~MonsterInsert();

    txtDatabase::RecordType getSelectedMonster() const;
    int getQty() const;
public slots:
    void lvMonster_currentChanged(const QModelIndex& cur, const QModelIndex& prev);
private slots:
    void on_leRegex_returnPressed();

    void on_leRegex_textChanged(const QString &arg1);

private:
    Ui::MonsterInsert *ui;
    const txtDatabase& m_db;
    MonsterListModel m_filtered_model;
};

#endif // MONSTERINSERT_H
