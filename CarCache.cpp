#include "CarCache.h"


CarCache::CarCache(DbScope &dbs, const char *cache_name)
    : TableCache(dbs, "cars"), m_cache(10000), m_cache_name(cache_name) {

}

QString CarCache::get_owner(QString license_plate) {
    QString *retval = m_cache.object(license_plate);
    if (retval) {
        return *retval;
    }

    QSqlQuery q(m_dbs.get_db());
    Q_ASSERT(q.prepare("select owner from cars where license_plate = :license_plate"));
    q.bindValue(":license_plate", license_plate);

    if (! q.exec()) {
        qCritical() << q.lastError().databaseText();
        qCritical() << q.lastError().driverText();
        return QString();
    }

    if (! q.next()) {
        qDebug() << "License plate" << license_plate << "not found";
        return QString();
    }

    retval = new QString(q.value(0).toString());
    m_cache.insert(license_plate, retval, retval->size());
    return *retval;
}

void CarCache::invalidate() {
    qDebug() << m_cache_name << "invalidate()";
    m_cache.clear();
}
