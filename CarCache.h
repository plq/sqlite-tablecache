#ifndef CARCACHE_H
#define CARCACHE_H

#include "TableCache.h"

#include <QDebug>
#include <QCache>
#include <QSqlQuery>
#include <QSqlError>

class CarCache : public TableCache {
public:
    CarCache(DbScopeCached &dbs, const char *cache_name);

    QString get_owner(QString license_plate);
    virtual void invalidate() override;

private:
    QCache<QString, QString> m_cache;
    const char *m_cache_name; // just to make debugging easier

};

#endif // CARCACHE_H
