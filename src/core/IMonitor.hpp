#pragma once
#include <string>
#include <map>
#include <variant>

namespace lsaa {

    // Types de métriques simples
    using MetricValue = std::variant<long long, double, std::string>;
    using MetricsMap = std::map<std::string, MetricValue>;

    class IMonitor {
    public:
        virtual ~IMonitor() = default;

        // Initialisation (ouverture handles, etc.)
        virtual bool initialize() = 0;

        // Collecte des données (tick)
        virtual bool collect() = 0;

        // Récupération des résultats
        virtual MetricsMap getMetrics() const = 0;

        // Nom unique du moniteur
        virtual std::string getName() const = 0;
    };

}
