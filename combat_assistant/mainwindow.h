#ifndef MAINWINDOW_H
#define MAINWINDOW_H
////include
//Qt
#include <QMainWindow>
#include <QListWidget>
#include <QListView>
#include <QMenu>
#include <QDir>
//std libs
#include <vector>
//#include <list>
#include <functional>
#include <algorithm>
#include <cassert>
#include <iostream>
//lua
#include <lua.hpp>
//other forms
#include "npcinsert.h"
#include "pcinsert.h"
//struct
#include "structs.h"

namespace Ui {
class MainWindow;
}

//model for model/view paradigm
class InitiativeListModel : public QAbstractListModel {
public:
    InitiativeListModel(std::map<QString, BaseEntry_PtrType>& entry_lut) : m_entry_lut(entry_lut)
        {}

    //virtual functions
    int rowCount(const QModelIndex &parent = QModelIndex()) const {
        return m_initiative_list.size();
    }
    int columnCount(const QModelIndex &parent = QModelIndex()) const {
        return 1;
    }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const {
        assert(index.column() == 0);

        if(role == Qt::DisplayRole) {
            QString name = m_initiative_list[index.row()];
            int iniz = GetInitiative(m_initiative_list[index.row()]);
            name += QString(" (%1)").arg(iniz);
            //std::cerr << name.toLocal8Bit().data() << std::endl;
            return name;
        } else if(role == Qt::DecorationRole) {
            QString name = m_initiative_list[index.row()];
            if(m_entry_lut.count(name)) {
                name = m_entry_lut.at(name)->ico;
            } else {
                throw std::runtime_error("InitiativeListModel ERROR: unknown name " + name.toStdString());
            }
            //name is the icon
            return QIcon(name);
        }

        return QVariant();
    }
    void AddInitiativeEntry(const QString& name) {
        m_initiative_list.push_back(name);
        Sort();
        //notify data change
        emit dataChanged(createIndex(0, 0), createIndex(m_initiative_list.size()-1, 0));
    }
    QString GetUName(int index) const {
        if( index >= 0 && index < m_initiative_list.size()) {
            return m_initiative_list[index];
        }

        return "";
    }
    bool Swap(int i1, int i2) {
        if(i1 >= 0 && i2 >= 0 && i1 < m_initiative_list.size() && i2 < m_initiative_list.size()) {
            int new_initiative = findInitiative(i2);

            findInitiative(i1) = new_initiative;

            std::swap(m_initiative_list[i1], m_initiative_list[i2]);
            emit dataChanged(createIndex(0, 0), createIndex(m_initiative_list.size()-1, 0));
            return true;
        }

        return false;
    }
    bool Remove(int index) {
        if(index >= 0 && index < m_initiative_list.size()) {
            m_initiative_list.erase(m_initiative_list.begin()+index);
            emit dataChanged(createIndex(0, 0), createIndex(m_initiative_list.size()-1, 0));
            return true;
        }

        return false;
    }
    int& findInitiative(int index) {
        const QString uname = m_initiative_list[index];

        if(m_entry_lut.count(uname)) {
            return m_entry_lut[uname]->initiative;
        } else {
            throw std::runtime_error("InitiativeListModel ERROR: unknown name " + uname.toStdString());
        }
    }


private:
    //actual data
    std::vector<QString> m_initiative_list;
    //the lookup tables
    std::map<QString, BaseEntry_PtrType>& m_entry_lut;
    //the comparator
//    bool Comp(const QString& s1, const QString& s2) {
//        int i1 = GetInitiative(s1), i2 = GetInitiative(s2);

//        return i1 > i2;
//    }

    //functions
    void Sort() {
//        std::stable_sort(m_initiative_list.begin(), m_initiative_list.end(), std::bind(&InitiativeListModel::Comp, this, std::placeholders::_1, std::placeholders::_2));
        std::stable_sort(m_initiative_list.begin(), m_initiative_list.end(), [this](const QString& s1, const QString& s2){ return GetInitiative(s1)>GetInitiative(s2); });
    }
    int GetInitiative(const QString& name) const {
        if(m_entry_lut.count(name)) {
            return m_entry_lut.at(name)->initiative;
        } else {
            throw std::runtime_error("InitiativeListModel ERROR: unknown name " + name.toStdString());
        }
    }
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private slots:
    void on_actionRefresh_Templates_triggered();

    void on_pbUp_clicked();

    void on_pbUp_2_clicked();

    void keyPressEvent(QKeyEvent *event);

    void on_pbRemove_clicked();

    void on_actionAdd_PC_triggered();

    void on_lvCharacters_clicked(const QModelIndex &index);

    //menu show
    void ShowMenu(const QPoint& p);

    void change_pf();

    void on_lwDead_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

    void on_lwDead_clicked(const QModelIndex &index);

    void on_actionAdd_NPC_triggered();

    void on_lvCharacters_doubleClicked(const QModelIndex &index);

public slots:
    void lvCharacters_currentChanged(const QModelIndex &cur, const QModelIndex &prev);

private:
    Ui::MainWindow *ui;

    ///my data
    //the initiatives entry (current and past)
    std::map<QString, BaseEntry_PtrType> m_entry_lut;

    //the name table
    std::map<QString, int> m_name_count;

    //the lua state
    lua_State* m_L;

    //my model
    InitiativeListModel m_initiative;
    int m_currentIndex;

    //context menu
    QMenu m_charMenu;

    //insert windows
    npcinsert m_npcinsert_win;
    //PCInsert m_pcinsert_win;

    ///my functions
    //generate unique name
    QString generateUName(const QString& name);
    //update initiative list
    //add to the initiative list
    void MenuSetup();
    void ChangePF(int val);
    void DisplayDescription(const QString& uname);
    QString DescribeEntry(const QString& uname) const;
};

#endif // MAINWINDOW_H
