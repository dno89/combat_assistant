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
///other forms
#include <pcinsert.h>
#include <npcinsert.h>

///TEMPORARY HARD CONF
static const char* template_folder = "templates/";
static const char* base_script = "base.lua";
static const char* npc_table_script = "npc_table.lua";

//global
static std::default_random_engine eng(time(NULL));
static std::uniform_int_distribution<short> d20(1, 20);

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_initiative(m_entry_lut), m_npcinsert_win(this)
{
    ui->setupUi(this);

    ///LUA SETUP
    //setup the lua state
    m_L = luaL_newstate();
    //open the standard libraries
    luaL_openlibs(m_L);
    //open the base script
    if(luaL_loadfile(m_L, base_script) || lua_pcall(m_L, 0, 0, 0)) {
        std::cerr << "Error while loading/executing the base script: " << base_script << std::endl;
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
    //set the correct name
    if(m_name_count.count(name) == 0) {
        m_name_count[name] = 1;
    } else {
        ++m_name_count[name];
    }
//    QString().ar
//    std::ostrstream uname;
//    uname << name << " #" << m_name_count[name] << '\0';

    return QString("%1#%2").arg(name).arg(m_name_count[name]);
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

    ostrstream out;

    BaseEntry_PtrType baseentry = m_entry_lut.at(uname);
    CustomNPC_PtrType cnpc;
    Monster_PtrType monster;
    NPC_PtrType npc;

    if(cnpc = dynamic_pointer_cast<CustomNPC_PtrType>(baseentry)) {
    } else if(monster = dynamic_pointer_cast<Monster_PtrType>(baseentry)) {
    } else if(npc = dynamic_pointer_cast<Monster_PtrType>(baseentry)) {
    }

//    if(m_template_table.count(uname)) {
//        const template_npc& n = m_template_table.at(uname);

//        const char* title;
//        out << "===========[ " << uname << " ]===========" << endl;
//        out << "Tag: " << n.TAG << "; Liv: " << n.LIV << "; DV: " << "d" << n.DV << endl;
//        out << "INIZ: " << showpos << n.INIZ << noshowpos << " (" << n.INIZ_val << ")" << endl;
//        //out << string(strlen(title), '=');
//        title = "===========[ DIFESA ]===========";
//        out << title << endl;
//        out << "CA:\t" << n.CA << " (contatto: " << n.CA_cont << ", sprovvista: " << n.CA_sprovv << ")" << endl;
//        out << "PF:\t" << n.remaining_PF << " / " << n.PF << endl;
//        out << "TEM: " << showpos << n.TEM << "; RIF: " << n.RIF << "; VOL: " << n.VOL << endl;
//        if(!n.DIF.empty()) {
//            out << "Abilita' di difesa: " << endl;
//            for(auto d : n.DIF) {
//                out << "\t> " << d << endl;
//            }
//        }
//        title = "===========[ OFFESA ]===========";
//        out << title << endl;
//        out << "VEL: " << n.VEL << "m ("<< std::floor(n.VEL/1.5) << " quadretti)" << endl;
//        out << "MISCHIA: " << n.A_MIS.NAME << ": " << n.MIS.TPC << " (" << n.MIS.DAN << ", " << n.A_MIS.CRI <<")" << endl;
//        out << "DISTANZA: " << n.A_DIS.NAME << ": " << n.DIS.TPC << " (" << n.DIS.DAN << ", " << n.A_DIS.CRI << ")" << endl;

//        title = "===========[ STATISTICHE ]===========";
//        out << title << endl;
//        out << noshowpos << "FOR:" << n.FOR << "; DES: " << n.DES << "; COS: " << n.COS << "; INT: " << n.INT << "; SAG: " << n.SAG << "; CAR: " << n.CAR << ";" << endl;
//        out << "BAB: " << n.BAB << "; BMC: " << n.BMC << "; DMC: " << n.DMC << endl;
//        title = "===========[ TALENTI ]===========";
//        out << title << endl;
//        for(auto t : n.TAL) {
//            out << " > " << t << endl;
//        }
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
//        title = "===========[ OGGETTI ]===========";
//        out << title << endl;
//        out << noshowpos << "mp:\t" << n.MON.MP << endl;
//        out << "mo:\t" << n.MON.MO << endl;
//        out << "ma:\t" << n.MON.MA << endl;
//        out << "mr:\t" << n.MON.MR << endl;
//        for(auto t : n.OGG) {
//            out << " - " << t << endl;
//        }

//        out << '\0';
//    } else {
//        const npc& n = m_npc_table.at(uname);

//        const char* title = "===========[ GENERIC NPC ]===========";
//        out << title << endl;
//        out << "PF:\t" << n.remaining_PF << " / " << n.PF << endl;
//        out << "INIZ:\t" << noshowpos << " (" << n.INIZ_val << ")" << endl;
//        out << '\0';
//    }

    return QString::fromStdString(out.str());
}

void MainWindow::lvCharacters_currentChanged(const QModelIndex& cur, const QModelIndex& prev) {
    int index = cur.row();

//    std::string str = m_initiative.GetUName(index).toStdString();

    DisplayDescription(m_initiative.GetUName(index));
}

void MainWindow::DisplayDescription(const QString &uname) {
    ui->tbNPC->setText(DescribeEntry(uname));

//    if(m_template_table.count(uname) || m_npc_table.count(uname)) {
//        ui->tbNPC->setEnabled(true);

//        //NPC description
//        std::string str = DescribeNPC(uname);

//        //set the description
//        ui->tbNPC->setText(QString(str.c_str()));
//    } else {
//        ui->tbNPC->setEnabled(false);
//    }
}

void MainWindow::on_actionAdd_PC_triggered()
{
    //std::cerr << "Add triggered" << std::endl;
    PCInsert win(this);
    //win.setModal(true);
    //releaseKeyboard();
    if(win.exec() != QDialog::Rejected) {

        std::string name = win.GetName();
        int init = win.GetInitiative();

        name = generateUName(name);
        character new_pc;
        //set the name and the initiative
        new_pc.NAME = name;
        new_pc.INIZ_val = init;

        //add it to the table
        m_pc_table[name] = new_pc;

        //add an initiative entry
        m_initiative.AddInitiativeEntry(name);

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
    std::string uname = m_initiative.GetUName(index);

    if(m_template_table.count(uname)) {
        m_template_table[uname].remaining_PF += val;

        lvCharacters_currentChanged(m_initiative.index(index), m_initiative.index(index));
    } else if(m_npc_table.count(uname)) {
        m_npc_table[uname].remaining_PF += val;
        lvCharacters_currentChanged(m_initiative.index(index), m_initiative.index(index));
    }

    //std::cerr << "change_pf called with value: " << val << std::endl;
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
    //std::cerr << "Add triggered" << std::endl;
    //releaseKeyboard();
    if(m_npcinsert_win.exec() != QDialog::Rejected) {

        std::string name = m_npcinsert_win.GetUName();
        name = generateUName(name);

        int init = m_npcinsert_win.GetIni();
        if(m_npcinsert_win.GetRD20()) {
            init += d20(eng);
        }

        std::string pf = m_npcinsert_win.GetPF();
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


        npc new_npc;
        //set the name and the initiative
        new_npc.NAME = name;
        new_npc.INIZ_val = init;
        new_npc.PF = PF;
        new_npc.remaining_PF = PF;

        //add it to the table
        m_npc_table[name] = new_npc;

        //add an initiative entry
        m_initiative.AddInitiativeEntry(name);

        //std::cerr << name << " " << init << std::endl;
    }
    m_npcinsert_win.hide();
    //m_npcinsert_win.close();

    //grabKeyboard();
}

void MainWindow::on_lvCharacters_doubleClicked(const QModelIndex &index)
{
    m_currentIndex = index.row();
}