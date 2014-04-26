#ifndef INITIATIVELISTMODEL_H
#define INITIATIVELISTMODEL_H

#include <QListView>

///////////////////////////////////////////////////////
/// \brief The InitiativeListModel class
///////////////////////////////////////////////////////
class InitiativeListModel : public QAbstractListModel {
public:
    InitiativeListModel(std::map<QString, BaseEntry_PtrType>& entry_lut, const int& current_index) :
        m_entry_lut(entry_lut), m_current_index(current_index)
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
                if(index.row() == m_current_index) {
                    name = ":/res/ico/current";
                } else {
                    name = m_entry_lut.at(name)->ico;
                }
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
    void Refresh() {
        QModelIndex first = this->createIndex(0, 0, nullptr), last = this->createIndex(m_entry_lut.size()-1, 0, nullptr);
        emit dataChanged(first, last);
    }


private:
    //actual data
    std::vector<QString> m_initiative_list;
    //the lookup tables
    std::map<QString, BaseEntry_PtrType>& m_entry_lut;
    //the current index
    const int& m_current_index;

    //functions
    void Sort() {
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

#endif // INITIATIVELISTMODEL_H
