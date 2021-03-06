#ifndef CACHEDDBSCOPE_H
#define CACHEDDBSCOPE_H

#include <mutex>
#include <vector>
#include <memory>
#include <unordered_set>

#include <QSet>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>

class TableCache;
class DbScopeCached;
using CacheStore = std::unordered_set<TableCache *>;
using ConnectionStore = std::unordered_set<DbScopeCached *>;
class DbScopeCached {
public:
    explicit DbScopeCached(const QString &dbname);
    ~DbScopeCached();

    bool register_table_change_callback(TableCache *);

    bool is_ready() const {
        return m_is_ready;
    }

    QSqlDatabase &get_db() {
        return *m_db;
    }

    template <typename T>
    T *register_cache(const char *cache_name) {
        m_own_caches.emplace_back(std::make_unique<T>(*this, cache_name));
        auto retiter = std::prev(m_own_caches.cend());
        auto retval = retiter->get();
        s_table_caches.insert(retval);

        if (! register_table_change_callback(retval)) {
            m_own_caches.erase(retiter);
            s_table_caches.erase(retval);
            return nullptr;
        }

        for (auto conn: s_connections) {
            conn->register_table_change_callback(retval);
        }

        return static_cast<T*>(retval);
    }

    static std::recursive_mutex &get_mutex();

private:
    static std::recursive_mutex s_mutex;
    static CacheStore s_table_caches;
    static ConnectionStore s_connections;
    std::vector<std::unique_ptr<TableCache>> m_own_caches;
    QSet<QString> m_triggered_tables;

    bool m_is_ready = false;
    std::unique_ptr<QSqlDatabase> m_db;

};

#endif
