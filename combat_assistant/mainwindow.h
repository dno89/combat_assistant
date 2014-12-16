#ifndef MAINWINDOW_H
#define MAINWINDOW_H
////include
//Qt
#include <QMainWindow>
#include <QListWidget>
#include <QListView>
#include <QMenu>
#include <QTimer>
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
#include "monsterinsert.h"
#include "diceroll.h"
//struct
#include "structs.h"
//models
#include <InitiativeListModel.h>
#include <AnnotationsModel.h>
//others
#include "txtDatabase.h"
#include "Logger.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private slots:
    void on_pbUp_clicked();

    void on_pbUp_2_clicked();

    void keyPressEvent(QKeyEvent *event);

    void on_pbRemove_clicked();

    void on_actionAdd_PC_triggered();

    void on_lvInitiativeList_clicked(const QModelIndex &index);

    //menu show
    void ShowCharMenu(const QPoint& p);
    void ShowAnnotationMenu(const QPoint& p);

    void removeAnnotation();
    void editAnnotation();

    void change_pf();

    void insertCustomAnnotation();

    void on_lwDead_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

    void on_lwDead_clicked(const QModelIndex &index);

    void on_actionAdd_NPC_triggered();

    void on_lvInitiativeList_doubleClicked(const QModelIndex &index);

    void on_actionAdd_Custom_NPC_triggered();

    void on_actionAdd_Monster_triggered();
    void on_actionRoll_DIce_triggered();

public slots:
    void lvInitiativeList_currentChanged(const QModelIndex &cur, const QModelIndex &prev);
    void timeoutTick();

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
    InitiativeListModel m_initiative_model;
    int m_currentIndex;
    AnnotationsModel m_annotations_model;

    //context menu
    QMenu m_charMenu;
    QMenu m_annotationMenu;

    //insert windows
    npcinsert m_npcinsert_win;
//    MonsterInsert m_monster_insert_win;

    ///configurations
    std::string m_base_script;

    //timeout timer
    QTimer m_timeout;
    int m_spareTime;
    double m_playerReactionTime;

    //monster db
    bool m_enable_monster;
    txtDatabase m_monster_db;
    txtDatabase::LabelMapType m_monster_labels;
    //npc db
    bool m_enable_npc;
    txtDatabase m_npc_db;
    txtDatabase::LabelMapType m_npc_labels;

    //dice utility
    DiceRoll m_diceroll_win;

    ///logger
    DECLARE_LOGGER()

    ///my functions
    //generate unique name
    QString generateUName(const QString& name);
    //update initiative list
    //add to the initiative list
    void MenuSetup();
    void ChangePF(int val);
    void Display(const QString& uname);
    void Display(int index);
    //default: display the current selection from lvInitiativeList
    void Display();
    void DisplayDescription(const QString& uname);
    void DisplayAnnotations(const QString& uname);
    QString DescribeEntry(const QString& uname) const;
    void readConf();
    template<typename T>
    void AddEntry(const QString& uname, const T& ptr) {
        assert(m_entry_lut.count(uname) == 0);

        BaseEntry_PtrType bptr = std::dynamic_pointer_cast<BaseEntry>(ptr);
        //add it to the entry lut
        m_entry_lut[uname] = bptr;
        //add it to the initiative model
        m_initiative_model.AddInitiativeEntry(uname);
    }
    QString getCurrentUName() const;
    void changeCurrentIndex(int new_index);
    void updateAnnotations();
    bool readFromConf(const char* name, double* out_val);
    bool readFromConf(const char* name, std::string* out_val);
    int generateDice(const std::string& str);
    BaseEntry_PtrType getCurrentEntry();
    void logText(QString str);
};

#endif // MAINWINDOW_H
