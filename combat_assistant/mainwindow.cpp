#include "mainwindow.h"
#include "ui_mainwindow.h"

///Qt
#include <QFile>
#include <QKeyEvent>
///std
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <strstream>
#include <string>
#include <cstring>
#include <iomanip>
#include <random>
#include <memory>
#include "utils.h"
///other forms
#include <pcinsert.h>
#include <npcinsert.h>
#include <annotationinsert.h>
#include <monsterinsert.h>
///boost
#include <boost/regex.hpp>

//HARD CONF
static const char* conf_file = "conf.lua";

using namespace std;

//global
static std::default_random_engine eng(time(NULL));
static std::uniform_int_distribution<short> d20(1, 20);

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_initiative_model(m_entry_lut, m_currentIndex), m_npcinsert_win(this),
    m_monster_db("\t"),
    m_diceroll_win(this),
    INIT_LOGGER("/tmp/combat_assistant", true, "mainwindow", true)
//    m_monster_insert_win(m_monster_db, this)
{
    ui->setupUi(this);

    //setup the splitter
    ui->mainSplitter->setStretchFactor(0, 20);
    ui->mainSplitter->setStretchFactor(1, 60);
    ui->mainSplitter->setStretchFactor(2, 20);

    ///LUA SETUP
    //setup the lua state
    m_L = luaL_newstate();
    //open the standard libraries
    luaL_openlibs(m_L);

    //read the configuration file
    readConf();

    //open the base script
    if(luaL_loadfile(m_L, m_base_script.c_str()) || lua_pcall(m_L, 0, 0, 0)) {
        std::cerr << "Error while loading/executing the base script: " << m_base_script << std::endl;
        std::cerr << lua_tostring(m_L, -1) << std::endl;
        exit(1);
    }

    //setup data/model
    ui->lvInitiativeList->setModel(&m_initiative_model);
    ui->lvAnnotations->setModel(&m_annotations_model);

    //timer setup
    m_timeout.setInterval(100);
    connect(&m_timeout, SIGNAL(timeout()), SLOT(timeoutTick()));

    //current item selection slot
    connect(ui->lvInitiativeList->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(lvInitiativeList_currentChanged(const QModelIndex&,const QModelIndex&)));


    //menu setup
    MenuSetup();
    //custom menu
    ui->lvInitiativeList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->lvInitiativeList, SIGNAL(customContextMenuRequested(const QPoint&)), SLOT(ShowCharMenu(const QPoint&)));

    ui->lvAnnotations->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->lvAnnotations, SIGNAL(customContextMenuRequested(QPoint)), SLOT(ShowAnnotationMenu(QPoint)));

    m_currentIndex = 0;
}

void MainWindow::readConf() {
    if(luaL_loadfile(m_L, conf_file) || lua_pcall(m_L, 0, 0, 0)) {
        std::cerr << "Error while loading/executing the configuration script: " << conf_file << std::endl;
        std::cerr << lua_tostring(m_L, -1) << std::endl;
        exit(1);
    }

    //read the base script location
    if(!readFromConf("base_script", &m_base_script)) {
        throw std::runtime_error("MainWindow ERROR: base_script property not found.. please set this in the configuration script.");
    }

    //read the allowed reaction time
    if(!readFromConf("reaction_time", &m_playerReactionTime)) {
        std::cerr << "No reaction_time property found, assuming a reaction time of 10s" << std::endl;
        m_playerReactionTime = 10.0;
    }

    //monster db
    std::string monster_db_fname;
    if(readFromConf("monster_db", &monster_db_fname)) {
        try {
            m_monster_db.loadDatabase(monster_db_fname.c_str());
            m_monster_labels = m_monster_db.getLabelMap();
            DPUSHC("Monster Database")
            DTRACE(m_monster_db.getRecordCount())
            for(auto& kv : m_monster_labels) {
                LOGGER() << kv.first << ": " << kv.second;
            }
            DPOPC()
            m_enable_monster = true;
        } catch (std::runtime_error& e) {
            std::cerr << "Something wrong happened reading monster database '" << monster_db_fname << "': " << e.what() << std::endl;
            m_enable_monster = false;
        }
    } else {
        std::cerr << "No monster_db in conf.lua: monster database not enabled" << std::endl;
        m_enable_monster = false;
    }

    //npc db
    std::string npc_db_fname;
    if(readFromConf("npc_db", &npc_db_fname)) {
        try {
            m_npc_db.loadDatabase(npc_db_fname.c_str());
            m_npc_labels = m_npc_db.getLabelMap();
            DPUSHC("NPC Database")
            DTRACE(m_npc_db.getRecordCount())
            for(auto& kv : m_npc_labels) {
                LOGGER() << kv.first << ": " << kv.second;
            }
            DPOPC()
            m_enable_npc = true;
        } catch (std::runtime_error& e) {
            std::cerr << "Something wrong happened reading npc database '" << monster_db_fname << "': " << e.what() << std::endl;
            m_enable_npc = false;
        }
    } else {
        std::cerr << "No npc_db in conf.lua: npc database not enabled" << std::endl;
        m_enable_monster = false;
    }
}

bool MainWindow::readFromConf(const char *name, double *out_val) {
    lua_getglobal(m_L, name);
    if(lua_isnumber(m_L, -1)) {
        *out_val = lua_tonumber(m_L, -1);
        lua_pop(m_L, 1);
        lua_pushnil(m_L);
        lua_setglobal(m_L, name);
        return true;
    } return false;
}

bool MainWindow::readFromConf(const char *name, std::string *out_val) {
    lua_getglobal(m_L, name);
    if(lua_isstring(m_L, -1)) {
        *out_val = lua_tostring(m_L, -1);
        lua_pop(m_L, 1);
        lua_pushnil(m_L);
        lua_setglobal(m_L, name);
        return true;
    } else return false;
}

MainWindow::~MainWindow()
{
    delete ui;
}

//support function
int GetNumberField(lua_State* l, const char* field) {
    //suppose that the table is at index -1

    //push the field
    lua_getfield(l, -1, field);

    if(!lua_isnumber(l, -1)) {
        throw std::runtime_error("Field " + std::string(field) + " is not a number!!");
    }

    //get the result
    int res = lua_tonumber(l, -1);

    //pop it from the stack
    lua_pop(l, 1);

    return res;
}

int GetNumberIndex(lua_State* l, int index) {
    //suppose that the table is at index -1

    //push the field
    lua_rawgeti(l, -1, index);

    if(!lua_isnumber(l, -1)) {
        throw std::runtime_error("Index is not a number!!");
    }

    //get the result
    int res = lua_tonumber(l, -1);

    //pop it from the stack
    lua_pop(l, 1);

    return res;
}

std::string GetStringField(lua_State* l, const char* field) {
    //suppose that the table is at index -1

    //push the field
    lua_getfield(l, -1, field);

    if(!lua_isstring(l, -1)) {
        throw std::runtime_error("Field " + std::string(field) + " is not a string!!");
    }

    //get the result
    std::string res(lua_tostring(l, -1));

    //pop it from the stack
    lua_pop(l, 1);

    return res;
}

std::string GetStringNumber(lua_State* l, int index) {
    //suppose that the table is at index -1

    //push the field
    lua_rawgeti(l, -1, index);

    if(!lua_isstring(l, -1)) {
        throw std::runtime_error("Index is not a string!!");
    }

    //get the result
    std::string res(lua_tostring(l, -1));

    //pop it from the stack
    lua_pop(l, 1);

    return res;
}

QString MainWindow::generateUName(const QString& name) {
    QString lname = name.toLower();

    //std::cerr << lname.toStdString() << std::endl;

    //set the correct name
    if(m_name_count.count(lname) == 0) {
        m_name_count[lname] = 1;
    } else {
        ++m_name_count[lname];
    }
//    QString().ar
//    std::ostrstream uname;
//    uname << name << " #" << m_name_count[name] << '\0';

    return QString("%1#%2").arg(lname).arg(m_name_count[lname]);
}

//void MainWindow::AddToInitiative(const std::string& name, int initiative, const std::string& ico) {
//    new QListWidgetItem(QIcon(ico.c_str()), QString(name.c_str()), ui->lwCharacters);
//}

void MainWindow::on_pbUp_clicked()
{
    int index = ui->lvInitiativeList->currentIndex().row();
    if(m_initiative_model.Swap(index, index-1)) {
        ui->lvInitiativeList->setCurrentIndex(m_initiative_model.index(index-1, 0));

        //copy the initiative value

    }
}

void MainWindow::on_pbUp_2_clicked()
{
    int index = ui->lvInitiativeList->currentIndex().row();
    if(m_initiative_model.Swap(index, index+1)) {
        ui->lvInitiativeList->setCurrentIndex(m_initiative_model.index(index+1, 0));
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    //std::cerr << "Keyboard event" << std::endl;
    //std::cerr << event->key() << std::endl;
    if(event->key() == Qt::Key_Return) {
        //int index = ui->lvInitiativeList->currentIndex().row();

//        std::cerr << "index: " << index << ", rowCount: " << m_initiative.rowCount() << std::endl;

        if(m_currentIndex+1 < m_initiative_model.rowCount()) {
            changeCurrentIndex(m_currentIndex+1);
        } else if(m_currentIndex >= 0 && m_currentIndex == m_initiative_model.rowCount()-1) {
            changeCurrentIndex(0);
        }
    } else if(event->key() == Qt::Key_Backspace) {
        //std::cerr << "enter pressed" << std::endl;
        changeCurrentIndex(m_currentIndex);
    } else if(event->key() == Qt::Key_Plus) {
        ChangePF(1);
    } else if(event->key() == Qt::Key_Minus) {
        ChangePF(-1);
    }
}

void MainWindow::changeCurrentIndex(int new_index) {
    if(m_initiative_model.rowCount() == 0) return;

    //update the current index annotations
    if(new_index != m_currentIndex) {
        updateAnnotations();
    }
    m_currentIndex = new_index;
    ui->lvInitiativeList->setCurrentIndex(m_initiative_model.index(m_currentIndex, 0));
    Display();

    //timer
    HPEntry_PtrType hp = dynamic_pointer_cast<HPEntry>(m_entry_lut.at(getCurrentUName()));
    DescriptionEntry_PtrType desc = dynamic_pointer_cast<DescriptionEntry>(m_entry_lut.at(getCurrentUName()));

    if(!(hp || desc)) {
        //start the timer
        m_spareTime = m_playerReactionTime*1000;
        ui->lcdTimeout->setStyleSheet("");
        m_timeout.start();
    } else {
        m_timeout.stop();
        ui->lcdTimeout->setStyleSheet("");
        ui->lcdTimeout->display(0.0);
    }
}

void MainWindow::updateAnnotations() {
    for(auto it = m_entry_lut.at(m_initiative_model.GetUName(m_currentIndex))->annotations.begin(); it != m_entry_lut.at(m_initiative_model.GetUName(m_currentIndex))->annotations.end();) {
        if(it->remaining_turns > 0) {
            --(it->remaining_turns);
            if(it->remaining_turns == 0) {
                it = m_entry_lut[m_initiative_model.GetUName(m_currentIndex)]->annotations.erase(it);
                continue;
            }
        }

        ++it;
    }
}

void MainWindow::on_pbRemove_clicked()
{
    QString uname = getCurrentUName();
    if(uname != QString("")) {
        int index = ui->lvInitiativeList->currentIndex().row();
        m_initiative_model.Remove(index);

        if(index >= m_initiative_model.rowCount()) {
            ui->lvInitiativeList->setCurrentIndex(m_initiative_model.index(m_initiative_model.rowCount()-1, 0));
        }

        if(m_initiative_model.rowCount() == 0) {
            //empty model
            m_currentIndex = -1;
        } else if(index <= m_currentIndex) {
            --m_currentIndex;
        }


        //add the removed char to the dead
        ui->lwDead->addItem(uname);
    }
}

QString MainWindow::DescribeEntry(const QString& uname) const {
    using namespace std;

    if(!m_entry_lut.count(uname)) {
        return QString("");
    }

    BaseEntry_PtrType baseentry = m_entry_lut.at(uname);
    CustomNPC_PtrType cnpc;
    DescriptionEntry_PtrType mon_npc;

    if(mon_npc = dynamic_pointer_cast<DescriptionEntry>(baseentry)) {
        //NPC or Monster
        std::string str = mon_npc->description.toStdString();
//        std::cerr << str << std::endl << std::endl;
        boost::regex r("<b>hp </b>[[:digit:]]+.\\((.*?)\\)");
        str = boost::regex_replace(str, r, "<b>hp </b>" + std::to_string(mon_npc->remaining_hp)+"/"+std::to_string(mon_npc->total_hp)+"(\\1)");
//        std::cerr << str << std::endl;
        return QString::fromStdString(formatDescription(str));
    } else if(cnpc = dynamic_pointer_cast<CustomNPC>(baseentry)) {
        //custom NPC
        return QString(R"+(
                       <div class="heading">
                       %1
                       </div>
                       <h5>
                       <p><b>hp: </b>%2/%3</p>
                       </h5>
                       )+").arg(uname).arg(cnpc->remaining_hp).arg(cnpc->total_hp);
    } else {
        //PC
        return QString(R"+(
                       <div class="heading">
                       %1
                       </div>)+").arg(uname);
    }
    return QString("");
}

void MainWindow::lvInitiativeList_currentChanged(const QModelIndex& cur, const QModelIndex& prev) {
    int index = cur.row();

//    std::string str = m_initiative.GetUName(index).toStdString();

    Display(m_initiative_model.GetUName(index));
}

void MainWindow::Display(const QString &uname) {
    DisplayDescription(uname);
    DisplayAnnotations(uname);

    m_initiative_model.Refresh();
    m_annotations_model.Refresh();
}

void MainWindow::DisplayAnnotations(const QString& uname) {
    if(m_entry_lut.count(uname)) {
        m_annotations_model.SetAnnotations(&m_entry_lut.at(uname)->annotations);
    }
}

void MainWindow::Display() {
    Display(ui->lvInitiativeList->currentIndex().row());
}

void MainWindow::Display(int index) {
    Display(m_initiative_model.GetUName(index));
}

void MainWindow::DisplayDescription(const QString &uname) {
    QString html = QString(R"+(<!DOCTYPE html>
      <html xmlns="http://www.w3.org/1999/xhtml" lang="en">
          <head>
              <title>Skills</title>
              <link href="PF.css" rel="stylesheet" type="text/css"/>
              <meta http-equiv="Content-Type" content="text/html; charset=utf-8"/>
              <meta name="viewport" content="width=device-width, minimum-scale=1, maximum-scale=1"/>
          </head>
          <body>
          %1
          </body>
      </html>)+").arg(DescribeEntry(uname));
    ui->tbDescription->setHtml(html);
}

void MainWindow::on_actionAdd_PC_triggered()
{
    //std::cerr << "Add triggered" << std::endl;
    PCInsert win(this);
    //win.setModal(true);
    //releaseKeyboard();
    if(win.exec() != QDialog::Rejected) {

        QString name = win.GetName();
        int init = win.GetInitiative();

        name = generateUName(name);
        PC_PtrType new_pc = make_shared<PC>();
        //set the name and the initiative
        new_pc->name = name;
        new_pc->initiative = init;

        //add it to the table
        AddEntry(name, new_pc);
//        m_entry_lut[name] = new_pc;

        //add an initiative entry
//        m_initiative.AddInitiativeEntry(name);

        //std::cerr << name << " " << init << std::endl;
    }
    win.hide();
    //win.close();
}

void MainWindow::on_lvInitiativeList_clicked(const QModelIndex &index)
{
    lvInitiativeList_currentChanged(index, index);
}

void MainWindow::ShowCharMenu(const QPoint& p) {
    m_charMenu.exec(ui->lvInitiativeList->mapToGlobal(p));
}

void MainWindow::ShowAnnotationMenu(const QPoint& p) {
    m_annotationMenu.exec(ui->lvAnnotations->mapToGlobal(p));
}

void MainWindow::MenuSetup() {
    //CHAR MENU
    //HP manipulation
    QMenu* pf_menu = new QMenu("HP", this);
    for(int ii = -120; ii <= +30; ++ii) {
        if(ii == 0) continue;

        QAction* a = pf_menu->addAction(QString((ii > 0?"+%1":"%1")).arg(ii));
        a->setData(ii);
        connect(a, SIGNAL(triggered()), SLOT(change_pf()));
    }
    m_charMenu.addMenu(pf_menu);
    //annotations
    QMenu* ann_menu = new QMenu("Annotations", this);
    {
        //custom annotation
        QAction* a = ann_menu->addAction("custom annotation");
        connect(a, SIGNAL(triggered()), SLOT(insertCustomAnnotation()));
    }
    m_charMenu.addMenu(ann_menu);

    //ANNOTATION MENU
    QAction* delete_ann = m_annotationMenu.addAction("delete");
    connect(delete_ann, SIGNAL(triggered()), SLOT(removeAnnotation()));
}

void MainWindow::removeAnnotation() {
    BaseEntry_PtrType ce = getCurrentEntry();

    if(!ce) {return;}

    QItemSelectionModel* ann_selected = ui->lvAnnotations->selectionModel();
    int row = ann_selected->currentIndex().row();
    if(row >= 0 && row < ce->annotations.size()) {
        //delete the annotation
        m_annotations_model.eraseAnnotation(row);
    }
}

BaseEntry_PtrType MainWindow::getCurrentEntry() {
    if(m_entry_lut.count(getCurrentUName())) {
        return m_entry_lut[getCurrentUName()];
    } else {
        return NULL;
    }
}

void MainWindow::insertCustomAnnotation() {
    AnnotationInsert aiwin(this);

    if(aiwin.exec() != QDialog::Rejected) {
        QString uname = getCurrentUName();

        Annotation ann;
        ann.text = aiwin.getDescription();
        ann.remaining_turns = aiwin.getDuration();
        ann.ico = ":/res/ico/default_annotation";

        m_entry_lut.at(uname)->annotations.push_back(ann);

        Display();
    }

//    std::cerr << "insertCustomAnnotation()" << std::endl;
}

void MainWindow::change_pf() {
    const QAction * a = qobject_cast<const QAction *>(this->sender());
    int val = a->data().value<int>();
    ChangePF(val);

    //std::cerr << "change_pf called with value: " << val << std::endl;
}

void MainWindow::ChangePF(int val) {
    int index = ui->lvInitiativeList->currentIndex().row();
    QString uname = m_initiative_model.GetUName(index);

    HPEntry_PtrType hpp = dynamic_pointer_cast<HPEntry>(m_entry_lut.at(uname));

    if(hpp) {
        hpp->remaining_hp += val;
        lvInitiativeList_currentChanged(m_initiative_model.index(index), m_initiative_model.index(index));
    }
}

void MainWindow::on_lwDead_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Display(current->text());
}

void MainWindow::on_lwDead_clicked(const QModelIndex &index)
{
    Display(ui->lwDead->item(index.row())->text());
}

void MainWindow::on_actionAdd_NPC_triggered() {
    if(m_enable_npc) {
        ///add npc
        MonsterInsert mi_win(m_npc_db, this);
        if(mi_win.exec() != QDialog::Rejected && mi_win.isSelected()) {
            //get the record
            txtDatabase::RecordType r = mi_win.getSelectedMonster();
            int rep = mi_win.getQty();
            for(int ii = 0; ii < rep; ++ii) {
                NPC_PtrType m = std::make_shared<Monster>();
                //set m field
                m->name = generateUName(QString::fromStdString(r[m_npc_labels.at("Name")]));
                std::string descr = r[m_npc_labels.at("FullText")];
                //the initiative
                m->initiative = d20(eng)+std::stoi(r[m_npc_labels.at("Init")]);
                //pf
                m->total_hp = generateDice(r[m_npc_labels.at("HD")]);
                m->remaining_hp = m->total_hp;
                //description
                m->description = QString::fromStdString(descr);

                AddEntry(m->name, m);
            }
        }
    }
}

void MainWindow::on_lvInitiativeList_doubleClicked(const QModelIndex &index)
{
    changeCurrentIndex(index.row());
//    m_currentIndex = ;
//    m_initiative_model.Refresh();
}

int MainWindow::generateDice(const string &str) {
    std::string f = "PF = " + preprocessDice(str);
    int PF;
    if(luaL_loadstring(m_L, f.c_str()) || lua_pcall(m_L, 0, 0, 0)) {
        std::cerr << "Error while executing PF script" << std::endl;
        std::cerr << lua_tostring(m_L, -1) << std::endl;
        PF = std::numeric_limits<int>::min();
    } else {
        //get the global value of PF
        lua_getglobal(m_L, "PF");
        if(lua_isnumber(m_L, -1)) {
            PF = lua_tointeger(m_L, -1);
        } else {
            PF = 0;
        }

        //erase the global value
        lua_pushnil(m_L);
        lua_setglobal(m_L, "PF");
    }

    return PF;
}

void MainWindow::on_actionAdd_Custom_NPC_triggered()
{
    //std::cerr << "Add triggered" << std::endl;
    if(m_npcinsert_win.exec() != QDialog::Rejected) {

        QString name = m_npcinsert_win.GetUName();
        name = generateUName(name);

        int init = m_npcinsert_win.GetIni();
        if(m_npcinsert_win.GetRD20()) {
            init += d20(eng);
        }

        CustomNPC_PtrType cnpc = make_shared<CustomNPC>();
        //set the name and the initiative
        cnpc->name = name;
        cnpc->initiative = init;
        cnpc->total_hp = generateDice(m_npcinsert_win.GetPF());
        cnpc->remaining_hp = cnpc->total_hp;

        AddEntry(name, cnpc);
    }

    m_npcinsert_win.hide();
}

QString MainWindow::getCurrentUName() const {
    return m_initiative_model.GetUName(ui->lvInitiativeList->currentIndex().row());
}

void MainWindow::timeoutTick() {
    m_spareTime -= 100;

    if(m_spareTime <= 0) {
        m_spareTime = 0;
        m_timeout.stop();

        ui->lcdTimeout->setStyleSheet("background-color:red;");
    }

    ui->lcdTimeout->display(QString("%1.%2").arg(floor(m_spareTime/1000)).arg((m_spareTime%1000)/100));
}


void MainWindow::on_actionAdd_Monster_triggered() {
    if(m_enable_monster) {
        ///add monster
        MonsterInsert mi_win(m_monster_db, this);
        if(mi_win.exec() != QDialog::Rejected && mi_win.isSelected()) {
            //get the record
            txtDatabase::RecordType r = mi_win.getSelectedMonster();
            int rep = mi_win.getQty();
            for(int ii = 0; ii < rep; ++ii) {
                Monster_PtrType m = make_shared<Monster>();
                //set m field
                m->name = generateUName(QString::fromStdString(r[m_monster_labels.at("Name")]));
                std::string descr = r[m_monster_labels.at("FullText")];
                //the initiative
                m->initiative = d20(eng)+extractInit(descr);
                //pf
                m->total_hp = generateDice(r[m_monster_labels.at("HD")]);
                m->remaining_hp = m->total_hp;
                //description
                m->description = QString::fromStdString(descr);

                AddEntry(m->name, m);
            }
        }
    }
}

void MainWindow::on_actionRoll_DIce_triggered()
{
    if(m_diceroll_win.exec() != QDialog::Rejected) {
        int result = generateDice(m_diceroll_win.getDiceText().toStdString());

        logText(QString("%1 --> %2\n").arg(m_diceroll_win.getDiceText()).arg(result));
    }
}

void MainWindow::logText(QString str) {
    ui->tbLog->setPlainText(ui->tbLog->toPlainText() + str);
}
