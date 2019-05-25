
#ifndef DBSCOPE_H
#define DBSCOPE_H

#include <vector>
#include <memory>
#include <unordered_set>

#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>

class TableCache;
using CacheStore = std::unordered_set<TableCache *>;
class DbScope {
public:
    explicit DbScope(const QString &dbname);

    bool is_ready() const {
        return m_is_ready;
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

        return static_cast<T*>(retval);
    }

    bool register_table_change_callback(TableCache *);

    ~DbScope() {
        m_db->close();
        for (const auto &cache: m_own_caches) {
            s_table_caches.erase(cache.get());
        }
    }

    QSqlDatabase &get_db() {
        return *m_db;
    }

private:
    static CacheStore s_table_caches;
    std::vector<std::unique_ptr<TableCache>> m_own_caches;

    bool m_is_ready = false;
    std::unique_ptr<QSqlDatabase> m_db;

};

#endif
