#ifndef TABLECACHE_H
#define TABLECACHE_H

#include <QSqlDatabase>

#include "DbScopeCached.h"

class TableCache {
public:
    TableCache(DbScopeCached &dbs, const QString &table_name)
            : m_dbs(dbs), m_table_name(table_name) {

    }

    virtual void invalidate() = 0;

    inline const QString &get_table_name() const {
        return m_table_name;
    }

    inline QSqlDatabase &get_db() const {
        return m_dbs.get_db();
    }

protected:
    DbScopeCached &m_dbs;
    const QString m_table_name;

};

#endif
