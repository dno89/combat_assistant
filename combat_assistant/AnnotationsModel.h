#ifndef ANNOTATIONS_MODEL_H
#define ANNOTATIONS_MODEL_H

#include <QListView>

///////////////////////////////////////////////////////
/// \brief The AnnotationsModel class
///////////////////////////////////////////////////////
class AnnotationsModel : public QAbstractListModel {
public:
    AnnotationsModel()
    {}

    //virtual functions
    int rowCount(const QModelIndex &parent = QModelIndex()) const {
        if(m_annotations) {
            return m_annotations->size();
        } else {
            return 0;
        }
    }
    int columnCount(const QModelIndex &parent = QModelIndex()) const {
        if(m_annotations) {
            return 1;
        } else {
            return 0;
        }
    }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const {
        assert(index.column() == 0);

        if(!m_annotations) {
            return QVariant();
        }

        if(role == Qt::DisplayRole) {
            int i = index.row();
            QString description = QString("%1 - [%2]").arg((*m_annotations)[i].text).arg((*m_annotations)[i].remaining_turns == 0?"permanent":QString("%1 rounds").arg((*m_annotations)[i].remaining_turns));
            return description;
        } else if(role == Qt::DecorationRole) {
            return QIcon((*m_annotations)[index.row()].ico);
        }

        return QVariant();
    }
    void Refresh() {
        if(m_annotations) {
            QModelIndex first = this->createIndex(0, 0, nullptr), last = this->createIndex(m_annotations->size()-1, 0, nullptr);
            emit dataChanged(first, last);
        }
    }
    void SetAnnotations(std::vector<Annotation>* model) {
        int old_size;
        if(m_annotations) {
            old_size = m_annotations->size();
        } else {
            old_size = 0;
        }
        beginRemoveRows(QModelIndex(), 0, old_size);
        m_annotations = model;
        endRemoveRows();
        Refresh();
    }
    void eraseAnnotation(unsigned int index) {
        if(index >= m_annotations->size()) {
            throw std::runtime_error("AnnotationsModel::eraseAnnotation ERROR: index out of bound!");
        }

        beginRemoveRows(QModelIndex(), index, index);
        m_annotations->erase(m_annotations->begin()+index);
        endRemoveRows();

        Refresh();
    }

private:
    ////data
    std::vector<Annotation>* m_annotations = nullptr;
};


#endif  //ANNOTATIONS_MODEL_H
