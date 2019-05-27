
#include "DbScopeCached.h"
#include "TableCache.h"
#include "CarCache.h"

#include <QDebug>

QSqlQuery qexec(DbScopeCached &dbs, const char *qstr) {
    QSqlQuery q(dbs.get_db());

    if (! q.prepare(qstr)) {
        qFatal("Query prep error: %s", q.lastError().databaseText().toUtf8().constData());
    }

    if (! q.exec()) {
        qFatal("Query exec error: %s", q.lastError().databaseText().toUtf8().constData());
    }

    return q;
}


int main() {
    DbScopeCached dbs0("cars.db");

    qexec(dbs0, "drop table if exists cars");
    qexec(dbs0,
        "create table cars ("
             "id integer primary key autoincrement"
            ",license_plate text not null unique"
            ",owner text not null"
        ")"
    );

    DbScopeCached dbs1("cars.db");
    dbs1.register_cache<CarCache>("cache1");

    DbScopeCached dbs2("cars.db");
    dbs2.register_cache<CarCache>("cache2");

    qDebug() << "create table cars ok";

    qexec(dbs0, "insert into cars (license_plate, owner) values ('lp1234', 'burak');");

    return 0;
}
