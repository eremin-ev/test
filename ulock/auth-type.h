/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#ifndef AUTH_TYPE_H
#define AUTH_TYPE_H

#include <QObject>
#include <QAbstractListModel>

class AuthType;
class AuthTypeModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum AuthTypeRoles {
        NameRole = Qt::UserRole + 1,
        TypeRole,
    };

    explicit AuthTypeModel(QObject *parent = nullptr);
    ~AuthTypeModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    QVector<AuthType> m_types;
};

#endif  // AUTH_TYPE_H
