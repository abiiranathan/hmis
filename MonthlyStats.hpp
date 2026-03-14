#ifndef MONTHLYSTATS_H
#define MONTHLYSTATS_H

#include <QHash>
#include <QString>

// Typed replacement for QMap<QString, QMap<QString, QMap<QString, int>>>
// Usage: stats.get("Malaria", "5 - 9 years").male
struct CategoryCount {
    int male = 0;
    int female = 0;

    [[nodiscard]] int total() const { return male + female; }

    void increment(const QString& sex) {
        if (sex == "Male") {
            male++;
        } else if (sex == "Female") {
            female++;
        }
    }
};

struct MonthlyStats {
    // key1 = diagnosis name or attendance status ("YES"/"NO")
    // key2 = age category string
    QHash<QString, QHash<QString, CategoryCount>> data;

    void increment(const QString& key1, const QString& ageCategory, const QString& sex) {
        data[key1][ageCategory].increment(sex);
    }

    [[nodiscard]] CategoryCount get(const QString& key1, const QString& ageCategory) const {
        auto it = data.find(key1);
        if (it == data.end()) {
            return {};
        }
        auto it2 = it->find(ageCategory);
        if (it2 == it->end()) {
            return {};
        }
        return *it2;
    }

    bool contains(const QString& key1) const { return data.contains(key1); }

    QList<QString> keys() const { return data.keys(); }

    void clear() { data.clear(); }
};

#endif  // MONTHLYSTATS_H
