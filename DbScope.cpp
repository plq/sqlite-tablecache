
#include "DbScope.h"

#include <inttypes.h>

#include <QDebug>
#include <QVariant>

#include <QSqlError>
#include <QSqlQuery>
#include <QSqlDriver>
#include <QSqlDatabase>

#include <sqlite3.h>

#include "TableCache.h"

Q_DECLARE_OPAQUE_POINTER(sqlite3 *)

Q_DECLARE_METATYPE(sqlite3 *)

CacheStore DbScope::s_table_caches;
std::recursive_mutex DbScope::s_mutex;
ConnectionStore DbScope::s_connections;

static void table_change_callback(sqlite3_context *ctx, int, sqlite3_value **) {
    auto ptr = sqlite3_user_data(ctx);
    auto caches = reinterpret_cast<CacheStore *>(ptr);

    {
        std::lock_guard<std::recursive_mutex> guard(DbScope::get_mutex());
        for (auto &cache: *caches) {
            cache->invalidate();
        }
    }
}

DbScope::DbScope(const QString &dbname) {
    auto conn_name = QString::number(reinterpret_cast<uintptr_t>(this));

    m_db = std::make_unique<QSqlDatabase>(QSqlDatabase::addDatabase("QSQLITE", conn_name));
    m_db->setDatabaseName(dbname);
    m_db->open();
    m_is_ready = m_db->isOpen();

    if (! m_is_ready) {
        qFatal("Unable to open db: %s", m_db->lastError().databaseText().toUtf8().constData());
    }

    {
        std::lock_guard<std::recursive_mutex> guard(s_mutex);
        s_connections.insert(this);
    }

    qDebug() << "conn:" << conn_name << "ready";
}

bool DbScope::register_table_change_callback(TableCache *cache) {
    const auto &table_name = cache->get_table_name();
    auto func_name = QStringLiteral("%1_notify_change").arg(table_name);
    if (m_triggered_tables.contains(func_name)) {
        return true;
    }

    auto &db = *m_db;

    auto h = db.driver()->handle();
    if (! h.isValid()) {
        qCritical() << "invalid db handle";
        return 0;
    }


    sqlite3* sqlite_handle = h.value<sqlite3 *>();
    auto func_name_bytes = func_name.toUtf8();
    const auto num_args = 0;
    const auto func_props = SQLITE_UTF8;
    const auto user_data = static_cast<void *>(&s_table_caches);

    auto ret = sqlite3_create_function(
                /* sqlite3 *db */                sqlite_handle,
                /* const char *zFunctionName, */ func_name_bytes.constData(),
                /* int nArg */                   num_args,
                /* int eTextRep */               func_props,
                /* void *pApp */                 user_data,
                /* void (*xFunc)(...) */         &table_change_callback,
                /* void (*xStep)(...) */         NULL,
                /* void (*xFinal)(...) */        NULL);

    if (ret != 0) {
        qCritical() << "Error registering table change callback:" << ret;
        return 0;
    }

    const auto queries = {
        QStringLiteral(
            "CREATE TEMP TRIGGER %2_i AFTER INSERT ON %1 FOR EACH ROW "
            "BEGIN "
                "select %2(); "
            "END"
        ),
        QStringLiteral(
            "CREATE TEMP TRIGGER %2_u AFTER UPDATE ON %1 FOR EACH ROW "
            "BEGIN "
                "select %2(); "
            "END"
        ),
        QStringLiteral(
            "CREATE TEMP TRIGGER %2_d AFTER DELETE ON %1 FOR EACH ROW "
            "BEGIN "
                "select %2(); "
            "END"
        ),
    };


    for (const auto &qstr_template: queries) {
        QSqlQuery q(db);
        auto qstr = qstr_template.arg(table_name).arg(func_name);

        if(! q.prepare(qstr)) {
            qFatal("Unable to prepare trigger."
                       "\n\tconn: %s"
                       "\n\tqstr: %s"
                       "\n\tqErr: %s",
                       db.connectionName().toUtf8().constData(),
                       qstr.toUtf8().constData(),
                       q.lastError().databaseText().toUtf8().constData());
        }

        if (! q.exec()) {
            qCritical() << q.lastError().databaseText();
            qCritical() << q.lastError().driverText();
            qFatal("Unable to register trigger");
            return 0;
        }
        else {
            qDebug() << "conn:" << db.connectionName() << "qstr:" << qstr;
        }
    }

    m_triggered_tables.insert(func_name);

    return reinterpret_cast<quintptr>(sqlite_handle);
}

std::recursive_mutex &DbScope::get_mutex() {
    return s_mutex;
}
