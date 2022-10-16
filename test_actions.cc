/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

enum class ActionType {
    A_Unknown = 0,
    A_Type1,
    A_Type2,
    A_Type3,
    A_Nr,
};

QJsonObject read_actions(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << __func__ << "Can't read" << path;
        return QJsonObject();
    }

    QJsonDocument jdoc = QJsonDocument::fromJson(file.readAll());
    if (jdoc.isNull()) {
        qWarning() << __func__ << "Can't parse" << path;
        return QJsonObject();
    }

    if (!jdoc.isObject()) {
        qWarning() << __func__ << "Json doc is not an object" << path;
        return QJsonObject();
    }

    return jdoc.object();
}

ActionType toActionType(int a)
{
    if (a < static_cast<int>(ActionType::A_Unknown)
        || a >= static_cast<int>(ActionType::A_Nr)) {
        return ActionType::A_Unknown;
    }

    return static_cast<ActionType>(a);
}

void test_read_actions(const QString &path)
{
    QJsonObject jobj = read_actions(path);
    QJsonValue jval = jobj.value(QStringLiteral("actions"));
    if (jval == QJsonValue::Undefined) {
        qDebug() << __func__ << "Cannot find actions in" << path;
        return;
    }

    if (!jval.isArray()) {
        qDebug() << __func__ << "Actions field is not an array in" << path;
        return;
    }

    QJsonArray jarr = jval.toArray();

    for (auto i = jarr.constBegin(); i != jarr.constEnd(); ++i) {
        qDebug() << __func__ << static_cast<int>(toActionType(i->toInt()));
    }
}

int main()
{
    test_read_actions(QStringLiteral("/tmp/actions.json"));

    qDebug() << "Good";

    return 0;
}
