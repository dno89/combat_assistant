#ifndef STRUCTS_H
#define STRUCTS_H

////include
//Qt
#include <QString>
//std
#include <vector>
#include <memory>


/**
 * @brief Representation of an annotation
 */
struct Annotation {
    Annotation(QString _text = "", int _remaining_turns = 0) : text(_text), remaining_turns(_remaining_turns)
        {}
    QString text;   //description
    int remaining_turns; //the remaining duration, in turns
    QString ico;    //the icon
};

/**
 * @brief The basic initiative entry
 */
struct BaseEntry {
    //////////////////////////////////////////////
    /// DATA
    //////////////////////////////////////////////
    int initiative; //the initiative value
    QString name;   //the displayed name
    std::vector<Annotation> annotations;    //the annotations
    QString ico;    //the icon

    //////////////////////////////////////////////
    /// FUNCTIONS
    //////////////////////////////////////////////
    void UpdateAnnotations() {
        for(auto it = annotations.begin(); it != annotations.end();) {
            if((*it).remaining_turns > 0) {
                --(*it).remaining_turns;
                if((*it).remaining_turns == 0) {
                    it = annotations.erase(it);
                    continue;
                }
            }
            ++it;
        }
    }

    virtual ~BaseEntry() {};
};

/**
 * @brief Entry with hp information
 */
struct HPEntry : public BaseEntry {
    int total_hp; //the total amount of HP
    int remaining_hp; //the remaining HP

    ~HPEntry() {}
};

/**
 * @brief A complete monster entry
 */
struct MonsterEntry : public HPEntry {
///TODO
    ~MonsterEntry() {}
};

/**
 * @brief A complete npc entry
 */
struct NPCEntry : public HPEntry {
///TODO
    ~NPCEntry() {}
};

////////////////////////////
/// TYPEDEF
////////////////////////////
typedef BaseEntry PC;
typedef HPEntry CustomNPC;
typedef MonsterEntry Monster;
typedef NPCEntry NPC;

//pointers
typedef std::shared_ptr<PC> PC_PtrType;
typedef std::shared_ptr<BaseEntry> BaseEntry_PtrType;
typedef std::shared_ptr<CustomNPC> CustomNPC_PtrType;
typedef std::shared_ptr<HPEntry> HPEntry_PtrType;
typedef std::shared_ptr<Monster> Monster_PtrType;
typedef std::shared_ptr<NPC> NPC_PtrType;



#endif // STRUCTS_H







































