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

    if(npc = dynamic_pointer_cast<NPC>(baseentry)) {
        //NPC
    } else if(monster = dynamic_pointer_cast<Monster>(baseentry)) {
        //Monster
    } else if(cnpc = dynamic_pointer_cast<CustomNPC>(baseentry)) {
        //custom NPC
    } else {
        //PC
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
//    ui->tbDescription->setHtml(R"+(<!DOCTYPE html>
//                               <html xmlns="http://www.w3.org/1999/xhtml" lang="en">
//                                   <head>
//                                       <title>Skills</title>
//                                       <link href="PF.css" rel="stylesheet" type="text/css" />
//                                       <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
//                                       <meta name="viewport" content="width=device-width, minimum-scale=1, maximum-scale=1" />
//                                   </head>
//                                   <body>
//                                   <h1 id="aasimar" class="monster-header">Aasimar</h1>
//                                               <p class="flavor-text"><i>This supernaturally beautiful woman looks human, yet emanates a strange sense of calm and benevolence.</i></p>

//                                               <p class="stat-block-title"><b>Aasimar <span class="stat-block-cr">CR 1/2</span></b></p>
//                                               <p class="stat-block-1"><b>XP 200</b></p>
//                                               <p class="stat-block-1">Aasimar cleric 1</p>
//                                               <p class="stat-block-1">NG Medium <a href="creatureTypes.html#_outsider" >outsider</a> (<a href="creatureTypes.html#_native-subtype" >native</a>)</p>
//                                               <p class="stat-block-1"><b>Init</b> +0; <b>Senses</b> darkvision 60 ft.; <a href="../skills/perception.html#_perception" >Perception</a> +5</p>
//                                               <p class="stat-block-breaker"><b>Defense</b></p>
//                                               <p class="stat-block-1"><b>AC</b> 15, touch 10, flat-footed 15 (+5 armor)</p>
//                                               <p class="stat-block-1"><b>hp</b> 11 (1d8+3)</p>
//                                               <p class="stat-block-1"><b>Fort</b> +4, <b>Ref</b> +0, <b>Will</b> +5</p>
//                                               <p class="stat-block-1"><b>Resist </b>acid 5, cold 5, electricity 5</p>
//                                               <p class="stat-block-breaker"><b>Offense</b></p>
//                                               <p class="stat-block-1"><b>Speed</b> 30 ft. (20 ft. in armor)</p>
//                                               <p class="stat-block-1"><b>Melee</b> heavy mace &ndash;1 (1d8&ndash;1)</p>
//                                               <p class="stat-block-1"><b>Ranged</b> light crossbow +0 (1d8/19&ndash;20)</p>
//                                               <p class="stat-block-1"><b>Special Attacks</b> channel positive energy (5/day, 1d6, DC 12); rebuke death (1d4+1, 6/day); touch of good (6/day)</p>
//                                               <p class="stat-block-1"><b><a href="universalMonsterRules.html#_spell-like-abilities" >Spell-Like Abilities</a></b> (CL 1st)</p>
//                                               <p class="stat-block-2">1/day&mdash;<i><a href="../spells/daylight.html#_daylight" >daylight</a></i></p>
//                                               <p class="stat-block-1"><b>Spells Prepared</b> (CL 1st)</p>
//                                               <p class="stat-block-2">1st&mdash;<i><a href="../spells/bless.html#_bless" >bless</a></i>, <i><a href="../spells/command.html#_command" >command</a></i> (DC 14), <i><a href="../spells/protectionFromEvil.html#_protection-from-evil" >protection from evil</a></i><sup>D</sup></p>
//                                               <p class="stat-block-2">0 (at will)&mdash;<i><a href="../spells/detectMagic.html#_detect-magic" >detect magic</a>, <a href="../spells/guidance.html#_guidance" >guidance</a>, <a href="../spells/stabilize.html#_stabilize" >stabilize</a></i></p>
//                                               <p class="stat-block-2"><b>D</b> domain spell; <b>Domains</b> Good, Healing</p>
//                                               <p class="stat-block-breaker"><b>Statistics</b></p>
//                                               <p class="stat-block-1"><b>Str</b> 8, <b>Dex</b> 10, <b>Con</b> 14, <b>Int</b> 13, <b>Wis</b> 17, <b>Cha</b> 14</p>
//                                               <p class="stat-block-1"><b>Base Atk</b> +0; <b>CMB</b> &ndash;1; <b>CMD</b> 9</p>
//                                               <p class="stat-block-1"><b>Feats</b> <a href="../feats.html#_turn-undead" >Turn Undead</a></p>
//                                               <p class="stat-block-1"><b>Skills</b> <a href="../skills/diplomacy.html#_diplomacy" >Diplomacy</a> +8, <a href="../skills/heal.html#_heal" >Heal</a> +7, <a href="../skills/knowledge.html#_knowledge" >Knowledge</a> (religion) +5; <b>Racial Modifiers</b> +2 <a href="../skills/diplomacy.html#_diplomacy" >Diplomacy</a>, +2 <a href="../skills/perception.html#_perception" >Perception</a></p>
//                                               <p class="stat-block-1"><b>Languages</b> Celestial, Common, Draconic</p>
//                                               <p class="stat-block-breaker"><b>Ecology</b></p>
//                                               <p class="stat-block-1"><b>Environment</b> any land</p>
//                                               <p class="stat-block-1"><b>Organization</b> solitary, pair, or team (3&ndash;6)</p>
//                                               <p class="stat-block-1"><b>Treasure</b> NPC gear (scale mail, heavy mace, light crossbow with 10 bolts, other treasure)</p>
//                                               <p>Aasimars are humans with a significant amount of celestial or other good outsider blood in their ancestry. Aasimars are not always good, but it is a natural tendency for them, and they gravitate to good faiths or organizations associated with celestials. Aasimar heritage can hide for generations, only to appear suddenly in the child of two apparently human parents. Most societies interpret aasimar births as good omens. Aasimars look mostly human except for some minor physical trait that reveals their unusual heritage. Typical aasimar features are hair that shines like metal, unusual eye or skin color, or even glowing golden halos. </p>
//                                               <h2>Aasimar Characters</h2>
//                                               <p>Aasimars are defined by class levels&mdash;they do not possess racial Hit Dice. Aasimars have the following racial traits.</p>
//                                               <p><b>+2 Charisma, +2 Wisdom</b>: Aasimars are insightful, confident, and personable.</p>
//                                               <p><b>Normal Speed</b>: Aasimars have a base speed of 30 feet.</p>
//                                               <p><b>Darkvision</b>: Aasimars can see in the dark up to 60 feet.</p>
//                                               <p><b>Skilled</b>: Aasimars have a +2 racial bonus on <a href="../skills/diplomacy.html#_diplomacy" >Diplomacy</a> and <a href="../skills/perception.html#_perception" >Perception</a> checks.</p>
//                                               <p><b>Spell-Like Ability</b>: Aasimars can use <i><a href="../spells/daylight.html#_daylight" >daylight</a></i> once per day as a spell-like ability (caster level equals the aasimar's class level).</p>
//                                               <p><b>Celestial Resistance</b>: Aasimars have acid resistance 5, cold resistance 5, and electricity resistance 5.</p>
//                                               <p><b>Languages</b>: Aasimars begin play speaking Common and Celestial. Aasimars with high Intelligence scores can choose any of the following bonus languages: Draconic, Dwarven, Elven, Gnome, Halfling, and Sylvan.</p>
//                                           </div>
//                                       </div>
//                                   </body>
//)+");
//    ui->tbDescription->setHtml(R"+(<!DOCTYPE html>
//                               <html xmlns="http://www.w3.org/1999/xhtml" lang="en">
//                                   <head>
//                                       <title>Skills</title>
//                                       <link href="PF.css" rel="stylesheet" type="text/css" />
//                                       <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
//                                       <meta name="viewport" content="width=device-width, minimum-scale=1, maximum-scale=1" />
//                                   </head>
//                                   <body>
//                                   <div><h2>Aasimar</h2><h3><i>This supernaturally beautiful woman looks human, yet emanates a strange sense of ca     lm and benevolence.</i></h3><br></br></div><div class="heading"><p class="alignleft">Aasimar</p><p class="alignright">CR 1/2</p><div style="clear: both;"></div></div><div><h5><b>XP </b>200</h5><h5>Aasimar cleric 1</h5><h5>NG Medium ou     tsider (native) </h5><h5><b>Init </b>+0; <b>Senses </b>darkvision 60 ft.; Perception +5</h5></div><hr/><div><h5><b>DEFENSE</b></h5></div><hr/><div><h5><b>AC </b>15, touch 10, flat-footed 15 (+5 armor)</h5><h5><b>hp </b>11 (1d8+3)</h5>     <h5><b>Fort </b>+4, <b>Ref </b>+0, <b>Will </b>+5</h5><h5><b>Resist </b>acid 5, cold 5, electricity 5</h5></div><hr/><div><h5><b>OFFENSE</b></h5></div><hr/><div><h5><b>Spd </b>30 ft. (20 ft. in armor)</h5><h5><b>Melee </b>heavy mace -     1 (1d8-1)</h5><h5><b>Ranged </b>light crossbow +0 (1d8/19-20)</h5><h5><b>Special Attacks </b>channel positive energy (5/day, 1d6, DC 12); rebuke death (1d4+1, 6/day); touch of good (6/day)</h5><h5><b>Spell-Like Abilities</b> (CL 1st)      </br>1/day&mdash;<i>daylight</i></h5></h5><h5><b>Spells Prepared</b> (CL 1st)</br>1st&mdash;<i>bless</i>, <i>command</i> (DC 14), <i>protection from evil</i><sup>D</sup></br>0 (at will)&mdash;<i>detect magic</i>,<i> guidance</i>, <i>s     tabilize</i></h5></h5><h5><b>D</b> domain spell; <b>Domains </b>Good, Healing</h5></div><hr/><div><h5><b>STATISTICS</b></h5></div><hr/><div><h5><b>Str</b> 8, <b>Dex</b> 10, <b>Con</b> 14, <b>Int</b> 13, <b>Wis</b> 17, <b>Cha</b> 14</h     5><h5><b>Base Atk </b>+0; <b>CMB </b>-1; <b>CMD </b>9</h5><h5><b>Feats </b>Turn Undead</h5><h5><b>Skills </b>Diplomacy +8, Heal +7, Knowledge (religion) +5; <b>Racial Modifiers </b>+2 Diplomacy, +2 Perception</h5><h5><b>Languages </b>     Celestial, Common, Draconic</h5></div><hr/><div><h5><b>ECOLOGY</b></h5></div><hr/><div><h5><b>Environment </b>Environment any land</h5><h5><b>Organization </b>solitary, pair, or team (3-6)</h5><h5><b>Treasure </b>NPC gear (scale mail,      heavy mace, light crossbow with 10 bolts, other treasure)</h5></div><br></br><div><h4><p>Aasimars are humans with a signif icant amount of celestial or other good outsider blood in their ancestry.</p><p>Aasimars are not always good,      but it is a natural tendency for them, and they gravitate to good faiths or organizations associated with celestials. Aasimar heritage can hide for generations, only to appear suddenly in the child of two apparently human parents. Mos     t societies interpret aasimar births as good omens. Aasimars look mostly human except for some minor physical trait that reveals their unusual heritage. Typical aasimar features are hair that shines like metal, unusual eye or skin col     or, or even glowing golden halos.</p><p><b>Aasimar Characters</b></br> Aasimars are defined by class levels-they do not possess racial Hit Dice. Aasimars have the following racial traits.</p><p><b>+2 Charisma, +2 Wisdom:</b> Aasimars      are insightful, confident, and personable.</p><p><b>Normal Speed:</b> Aasimars have a base speed of 30 feet.</p><p><b>Darkvision:</b> Aasimars can see in the dark up to 60 feet.</p><p><b>Skilled:</b> Aasimars have a +2 racial bonus on      Diplomacy and Perception checks.</p><p><b>Spell- Like Ability:</b> Aasimars can use daylight once per day as a spell-like ability (caster level equals the aasimar's class level).</p><p><b>Celestial Resistance:</b> Aasimars have acid      resistance 5, cold resistance 5, and electricity resistance 5.</p><p><b>Languages:</b> Aasimars begin play speaking Common and Celestial. Aasimars with high Intelligence scores can choose any of the following bonus languages: Draconic     , Dwarven, Elven, Gnome, Half ling, and Sylvan.</p></h4></div>
//                                   </body>
//)+");
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
)+").arg(R"+(<link href="PF.css" rel="stylesheet" type="text/css"/><div><h2>Aasimar</h2><h3><i>This supernaturally beautiful woman looks human, yet emanates a strange sense of ca     lm and benevolence.</i></h3><br></br></div><div class="heading"><p class="alignleft">Aasimar</p><p class="alignright">CR 1/2</p><div style="clear: both;"></div></div><div><h5><b>XP </b>200</h5><h5>Aasimar cleric 1</h5><h5>NG Medium ou     tsider (native) </h5><h5><b>Init </b>+0; <b>Senses </b>darkvision 60 ft.; Perception +5</h5></div><hr/><div><h5><b>DEFENSE</b></h5></div><hr/><div><h5><b>AC </b>15, touch 10, flat-footed 15 (+5 armor)</h5><h5><b>hp </b>11 (1d8+3)</h5>     <h5><b>Fort </b>+4, <b>Ref </b>+0, <b>Will </b>+5</h5><h5><b>Resist </b>acid 5, cold 5, electricity 5</h5></div><hr/><div><h5><b>OFFENSE</b></h5></div><hr/><div><h5><b>Spd </b>30 ft. (20 ft. in armor)</h5><h5><b>Melee </b>heavy mace -     1 (1d8-1)</h5><h5><b>Ranged </b>light crossbow +0 (1d8/19-20)</h5><h5><b>Special Attacks </b>channel positive energy (5/day, 1d6, DC 12); rebuke death (1d4+1, 6/day); touch of good (6/day)</h5><h5><b>Spell-Like Abilities</b> (CL 1st)      </br>1/day&mdash;<i>daylight</i></h5></h5><h5><b>Spells Prepared</b> (CL 1st)</br>1st&mdash;<i>bless</i>, <i>command</i> (DC 14), <i>protection from evil</i><sup>D</sup></br>0 (at will)&mdash;<i>detect magic</i>,<i> guidance</i>, <i>s     tabilize</i></h5></h5><h5><b>D</b> domain spell; <b>Domains </b>Good, Healing</h5></div><hr/><div><h5><b>STATISTICS</b></h5></div><hr/><div><h5><b>Str</b> 8, <b>Dex</b> 10, <b>Con</b> 14, <b>Int</b> 13, <b>Wis</b> 17, <b>Cha</b> 14</h     5><h5><b>Base Atk </b>+0; <b>CMB </b>-1; <b>CMD </b>9</h5><h5><b>Feats </b>Turn Undead</h5><h5><b>Skills </b>Diplomacy +8, Heal +7, Knowledge (religion) +5; <b>Racial Modifiers </b>+2 Diplomacy, +2 Perception</h5><h5><b>Languages </b>     Celestial, Common, Draconic</h5></div><hr/><div><h5><b>ECOLOGY</b></h5></div><hr/><div><h5><b>Environment </b>Environment any land</h5><h5><b>Organization </b>solitary, pair, or team (3-6)</h5><h5><b>Treasure </b>NPC gear (scale mail,      heavy mace, light crossbow with 10 bolts, other treasure)</h5></div><br></br><div><h4><p>Aasimars are humans with a signif icant amount of celestial or other good outsider blood in their ancestry.</p><p>Aasimars are not always good,      but it is a natural tendency for them, and they gravitate to good faiths or organizations associated with celestials. Aasimar heritage can hide for generations, only to appear suddenly in the child of two apparently human parents. Mos     t societies interpret aasimar births as good omens. Aasimars look mostly human except for some minor physical trait that reveals their unusual heritage. Typical aasimar features are hair that shines like metal, unusual eye or skin col     or, or even glowing golden halos.</p><p><b>Aasimar Characters</b></br> Aasimars are defined by class levels-they do not possess racial Hit Dice. Aasimars have the following racial traits.</p><p><b>+2 Charisma, +2 Wisdom:</b> Aasimars      are insightful, confident, and personable.</p><p><b>Normal Speed:</b> Aasimars have a base speed of 30 feet.</p><p><b>Darkvision:</b> Aasimars can see in the dark up to 60 feet.</p><p><b>Skilled:</b> Aasimars have a +2 racial bonus on      Diplomacy and Perception checks.</p><p><b>Spell- Like Ability:</b> Aasimars can use daylight once per day as a spell-like ability (caster level equals the aasimar's class level).</p><p><b>Celestial Resistance:</b> Aasimars have acid      resistance 5, cold resistance 5, and electricity resistance 5.</p><p><b>Languages:</b> Aasimars begin play speaking Common and Celestial. Aasimars with high Intelligence scores can choose any of the following bonus languages: Draconic     , Dwarven, Elven, Gnome, Half ling, and Sylvan.</p></h4></div>)+");
//    std::cout << html.toStdString() << std::endl;
    ui->tbDescription->setHtml(html);

//    ui->tbNPC->setText(DescribeEntry(uname));

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

        QString name = win.GetName();
        int init = win.GetInitiative();

        name = generateUName(name);
        PC_PtrType new_pc = make_shared<PC>();
        //set the name and the initiative
        new_pc->name = name;
        new_pc->initiative = init;

        //add it to the table
        m_entry_lut[name] = new_pc;

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

        CustomNPC_PtrType cnpc = make_shared<CustomNPC>();
        //set the name and the initiative
        cnpc->name = name;
        cnpc->initiative = init;
        cnpc->total_hp = PF;
        cnpc->remaining_hp = PF;

        //add it to the table
        m_entry_lut[name] = static_pointer_cast<BaseEntry>(cnpc);

        //add an initiative entry
        m_initiative.AddInitiativeEntry(name);

        //std::cerr << name << " " << init << std::endl;
    }

    m_npcinsert_win.hide();
    //m_npcinsert_win.close();
    //grabKeyboard();
}
