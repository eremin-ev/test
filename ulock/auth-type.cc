/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <QDebug>

#include "ulock.h"
#include "auth-type.h"

class AuthType {
public:
    QString authName;
    QString authType;
};

AuthTypeModel::AuthTypeModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_types({
        { QStringLiteral("Login"), QStringLiteral("Login") },
        { QStringLiteral("Authenticate"), QStringLiteral("Authenticate") }
      })
{
    qDebug() << __PRETTY_FUNCTION__;
}

//
// Use default if debug output is not required
//
//  AuthTypeModel::~AuthTypeModel() = default;
//
AuthTypeModel::~AuthTypeModel()
{
    qDebug() << __PRETTY_FUNCTION__;
}

int AuthTypeModel::rowCount(const QModelIndex &) const
{
    return m_types.size();
}

QVariant AuthTypeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() > m_types.size())
        return {};

    const AuthType &item = m_types[index.row()];

    switch (role) {
    case NameRole:
        return item.authName;
    case TypeRole:
        return item.authType;
    }

    return {};
}

QHash<int, QByteArray> AuthTypeModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {TypeRole, "type"},
    };
}
