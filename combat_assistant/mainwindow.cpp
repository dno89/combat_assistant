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
///other forms
#include <pcinsert.h>
#include <npcinsert.h>
///boost
#include <boost/regex.hpp>

//HARD CONF
static const char* conf_file = "conf.lua";

using namespace std;

//global
static std::default_random_engine eng(time(NULL));
static std::uniform_int_distribution<short> d20(1, 20);

//global functions
std::string processHP(const std::string& str) {
        boost::regex r("([[:digit:]]+)d([[:digit:]]+)");
        return regex_replace(str, r, "d(\\2, \\1)");
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_initiative(m_entry_lut), m_npcinsert_win(this)
{
    ui->setupUi(this);

    //setup the splitter
    ui->mainSplitter->setStretchFactor(0, 30);
    ui->mainSplitter->setStretchFactor(1, 70);

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
    ui->lvCharacters->setModel(&m_initiative);

    //grab the keyboard
    //grabKeyboard();

    //current item selection slot
    connect(ui->lvCharacters->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(lvCharacters_currentChanged(const QModelIndex&,const QModelIndex&)));


    //charmenu setup
    MenuSetup();
    //custom menu
    ui->lvCharacters->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->lvCharacters, SIGNAL(customContextMenuRequested(const QPoint&)), SLOT(ShowMenu(const QPoint&)));

    m_currentIndex = 0;
}

void MainWindow::readConf() {
    if(luaL_loadfile(m_L, conf_file) || lua_pcall(m_L, 0, 0, 0)) {
        std::cerr << "Error while loading/executing the configuration script: " << conf_file << std::endl;
        std::cerr << lua_tostring(m_L, -1) << std::endl;
        exit(1);
    }

    //read the base script location
    lua_getglobal(m_L, "base_script");
    assert(lua_isstring(m_L, -1));
    m_base_script = lua_tostring(m_L, -1);
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
    int index = ui->lvCharacters->currentIndex().row();
    if(m_initiative.Swap(index, index-1)) {
        ui->lvCharacters->setCurrentIndex(m_initiative.index(index-1, 0));

        //copy the initiative value

    }
}

void MainWindow::on_pbUp_2_clicked()
{
    int index = ui->lvCharacters->currentIndex().row();
    if(m_initiative.Swap(index, index+1)) {
        ui->lvCharacters->setCurrentIndex(m_initiative.index(index+1, 0));
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    //std::cerr << "Keyboard event" << std::endl;
    //std::cerr << event->key() << std::endl;
    if(event->key() == Qt::Key_Return) {
//        std::cerr << "space pressed" << std::endl;
        //int index = ui->lvCharacters->currentIndex().row();

//        std::cerr << "index: " << index << ", rowCount: " << m_initiative.rowCount() << std::endl;

        if(m_currentIndex+1 < m_initiative.rowCount()) {
            ++m_currentIndex;
        } else if(m_currentIndex >= 0 && m_currentIndex == m_initiative.rowCount()-1) {
            m_currentIndex = 0;
        }
        ui->lvCharacters->setCurrentIndex(m_initiative.index(m_currentIndex, 0));
    } else if(event->key() == Qt::Key_Backspace) {
        //std::cerr << "enter pressed" << std::endl;
        ui->lvCharacters->setCurrentIndex(m_initiative.index(m_currentIndex, 0));
    } else if(event->key() == Qt::Key_Plus) {
        ChangePF(1);
    } else if(event->key() == Qt::Key_Minus) {
        ChangePF(-1);
    }
}

void MainWindow::on_pbRemove_clicked()
{
    QString uname = m_initiative.GetUName(ui->lvCharacters->currentIndex().row());
    int index = ui->lvCharacters->currentIndex().row();
    m_initiative.Remove(index);

    if(index < m_currentIndex) {
        --m_currentIndex;
    }

    //add the removed char to the dead
    if(uname != QString("")) {
        ui->lwDead->addItem(uname);
    }
}

//std::string MainWindow::DescribeNPC(const std::string& uname) const {
//    using namespace std;

//    assert(m_template_table.count(uname) || m_npc_table.count(uname));

//    ostrstream out;

//    if(m_template_table.count(uname)) {
//        const template_npc& n = m_template_table.at(uname);

//        const char* title = "===========[ LIVELLO / TAGLIA / DV ]===========";
//        out << title << endl;
//        out << "Tag:\t" << n.TAG << endl;
//        out << "Liv:\t" << n.LIV << endl;
//        out << "DV:\t" << "d" << n.DV << endl;
//        //out << string(strlen(title), '=');
//        title = "===========[ PF / INIZIATIVA / CA ]===========";
//        out << title << endl;
//        out << "PF:\t" << n.remaining_PF << " / " << n.PF << endl;
//        out << "INIZ:\t" << showpos << n.INIZ << noshowpos << " (" << n.INIZ_val << ")" << endl;
//        out << "CA:\t" << n.CA << endl;
//        title = "===========[ CARATTERISTICHE ]===========";
//        out << title << endl;
//        out << noshowpos << "FOR:\t" << n.FOR << "\t| " << showpos << n.FOR_mod << endl;
//        out << noshowpos << "DES:\t" << n.DES << "\t| " << showpos << n.DES_mod << endl;
//        out << noshowpos << "COS:\t" << n.COS << "\t| " << showpos << n.COS_mod << endl;
//        out << noshowpos << "INT:\t" << n.INT << "\t| " << showpos << n.INT_mod << endl;
//        out << noshowpos << "SAG:\t" << n.SAG << "\t| " << showpos << n.SAG_mod << endl;
//        out << noshowpos << "CAR:\t" << n.CAR << "\t| " << showpos << n.CAR_mod << endl << noshowpos;
//        title = "===========[ TIRI SALVEZZA ]===========";
//        out << title << endl;
//        out << "TEM:\t" << showpos << n.TEM << endl;
//        out << "RIF:\t" << n.RIF << endl;
//        out << "VOL:\t" << n.VOL << endl;
//        title = "===========[ COMBATTIMENTO ]===========";
//        out << title << endl;
//        out << "BAB:\t" << n.BAB << endl;
//        out << "BMC:\t" << n.BMC << endl;
//        out << "DMC:\t" << n.DMC << endl;
//        title = "===========[ MISCHIA ]===========";
//        out << title << endl;
//        out << n.A_MIS.NAME << ":\t" << n.MIS.TPC << " (" << n.MIS.DAN << ", " << n.A_MIS.CRI <<")" << endl;
//        title = "===========[ DISTANZA ]===========";
//        out << title << endl;
//        out << n.A_DIS.NAME << ":\t" << n.DIS.TPC << " (" << n.DIS.DAN << ", " << n.A_DIS.CRI << ")" << endl;
//        title = "===========[ ARMATURA / SCUDO ]===========";
//        out << title << endl;
//        out << n.ARM.NAME << ":\t" << showpos << n.ARM.BONUS << " CA, " << n.ARM.PEN << " pen, ";
//        if(n.ARM.DES_MAX > 0) {
//            out << n.ARM.DES_MAX << " DES max";
//        } out << endl;
//        out << n.SCU.NAME << ":\t" << showpos << n.SCU.BONUS << " CA, " << n.SCU.PEN << " pen, ";
//        if(n.SCU.DES_MAX > 0) {
//            out << n.SCU.DES_MAX << " DES max";
//        } out << endl;
//        title = "===========[ TALENTI ]===========";
//        out << title << endl;
//        for(auto t : n.TAL) {
//            out << " > " << t << endl;
//        }
//        title = "===========[ MONETE ]===========";
//        out << title << endl;
//        out << noshowpos << "mp:\t" << n.MON.MP << endl;
//        out << "mo:\t" << n.MON.MO << endl;
//        out << "ma:\t" << n.MON.MA << endl;
//        out << "mr:\t" << n.MON.MR << endl;
//        title = "===========[ OGGETTI ]===========";
//        out << title << endl;
//        for(auto t : n.OGG) {
//            out << " - " << t << endl;
//        }
//        //out << string(strlen(title), '=');

//        out << '\0';
//    } else {
//        const npc& n = m_npc_table.at(uname);

//        const char* title = "===========[ GENERIC NPC ]===========";
//        out << title << endl;
//        out << "PF:\t" << n.remaining_PF << " / " << n.PF << endl;
//        out << "INIZ:\t" << noshowpos << " (" << n.INIZ_val << ")" << endl;
//        out << '\0';
//    }

//    return out.str();
//}

QString MainWindow::DescribeEntry(const QString& uname) const {
    using namespace std;

    assert(m_entry_lut.count(uname));

//    ostrstream out;

    BaseEntry_PtrType baseentry = m_entry_lut.at(uname);
    CustomNPC_PtrType cnpc;
    Monster_PtrType monster;
    NPC_PtrType npc;

    if(npc = dynamic_pointer_cast<NPC>(baseentry)) {
        //NPC
    } else if(monster = dynamic_pointer_cast<Monster>(baseentry)) {
        //Monster
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
//    return QString::fromStdString(out.str());
    return QString("");
}

void MainWindow::lvCharacters_currentChanged(const QModelIndex& cur, const QModelIndex& prev) {
    int index = cur.row();

//    std::string str = m_initiative.GetUName(index).toStdString();

    DisplayDescription(m_initiative.GetUName(index));
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

//    std::cout << html.toStdString() << std::endl;
//    ui->tbNPC->setText(DescribeEntry(uname));
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

    //grabKeyboard();
}

void MainWindow::on_lvCharacters_clicked(const QModelIndex &index)
{
    lvCharacters_currentChanged(index, index);
}

void MainWindow::ShowMenu(const QPoint& p) {
    m_charMenu.exec(ui->lvCharacters->mapToGlobal(p));
}

void MainWindow::MenuSetup() {
    QMenu* pf_menu = new QMenu("PF", this);

    for(int ii = 40; ii >= -40; --ii) {
        if(ii == 0) continue;

        QAction* a = pf_menu->addAction(QString((ii > 0?"+%1":"%1")).arg(ii));
        a->setData(ii);
        connect(a, SIGNAL(triggered()), SLOT(change_pf()));
    }

    m_charMenu.addMenu(pf_menu);

}

void MainWindow::change_pf() {
    const QAction * a = qobject_cast<const QAction *>(this->sender());
    int val = a->data().value<int>();
    ChangePF(val);

    //std::cerr << "change_pf called with value: " << val << std::endl;
}

void MainWindow::ChangePF(int val) {
    int index = ui->lvCharacters->currentIndex().row();
    QString uname = m_initiative.GetUName(index);

    HPEntry_PtrType hpp = dynamic_pointer_cast<HPEntry>(m_entry_lut[uname]);

    if(hpp) {
        hpp->remaining_hp += val;
        lvCharacters_currentChanged(m_initiative.index(index), m_initiative.index(index));
    }
}

void MainWindow::on_lwDead_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    DisplayDescription(current->text().toLocal8Bit().data());
}

void MainWindow::on_lwDead_clicked(const QModelIndex &index)
{
    DisplayDescription(ui->lwDead->item(index.row())->text().toLocal8Bit().data());
}

void MainWindow::on_actionAdd_NPC_triggered() {

}

void MainWindow::on_lvCharacters_doubleClicked(const QModelIndex &index)
{
    m_currentIndex = index.row();
}

void MainWindow::on_actionAdd_Custom_NPC_triggered()
{
    //std::cerr << "Add triggered" << std::endl;
    //releaseKeyboard();
    if(m_npcinsert_win.exec() != QDialog::Rejected) {

        QString name = m_npcinsert_win.GetUName();
        name = generateUName(name);

        int init = m_npcinsert_win.GetIni();
        if(m_npcinsert_win.GetRD20()) {
            init += d20(eng);
        }

        std::string pf = processHP(m_npcinsert_win.GetPF());
        int PF;
        pf = "NPC_PF = " + pf;
        if(luaL_loadstring(m_L, pf.c_str()) || lua_pcall(m_L, 0, 0, 0)) {
            std::cerr << "Error while executing pf script" << std::endl;
            std::cerr << lua_tostring(m_L, -1) << std::endl;
            PF = 0;
        } else {
            //get the global value of PF
            lua_getglobal(m_L, "NPC_PF");
            if(lua_isnumber(m_L, -1)) {
                PF = lua_tointeger(m_L, -1);
            } else {
                PF = 0;
            }

            //erase the global value
            lua_pushnil(m_L);
            lua_setglobal(m_L, "NPC_PF");
        }

        CustomNPC_PtrType cnpc = make_shared<CustomNPC>();
        //set the name and the initiative
        cnpc->name = name;
        cnpc->initiative = init;
        cnpc->total_hp = PF;
        cnpc->remaining_hp = PF;

        AddEntry(name, cnpc);
        //add it to the table
//        m_entry_lut[name] = static_pointer_cast<BaseEntry>(cnpc);
        //add an initiative entry
//        m_initiative.AddInitiativeEntry(name);

        //std::cerr << name << " " << init << std::endl;
    }

    m_npcinsert_win.hide();
    //m_npcinsert_win.close();
    //grabKeyboard();
}
