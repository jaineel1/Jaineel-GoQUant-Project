#ifndef LATENCY_TRACKER_H
#define LATENCY_TRACKER_H

#include <chrono>
#include <map>
#include <vector>
#include <mutex>
#include <string>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <iomanip>

using namespace std;

class LatencyTracker {
public:
    // Enum to categorize different latency types
    enum LatencyType {
        ORDER_PLACEMENT,
        MARKET_DATA_PROCESSING,
        WEBSOCKET_MESSAGE_PROPAGATION,
        TRADING_LOOP_END_TO_END
    };

    // Nested struct to store detailed latency information
    struct LatencyMetric {
        chrono::high_resolution_clock::time_point start_time;
        chrono::high_resolution_clock::time_point end_time;
        chrono::nanoseconds duration{0};
        bool completed{false};
    };

    // Start measuring latency for a specific type
    void start_measurement(LatencyType type, const string& unique_id = "");

    // Stop measuring latency and record the result
    void stop_measurement(LatencyType type, const string& unique_id = "");

    // Generate a comprehensive latency report
    string generate_report();

    // Export raw metrics for external analysis
    map<LatencyType, vector<LatencyMetric>> get_raw_metrics();

    // Clear all collected metrics
    void reset();

private:
    // Thread-safe collections for storing latency metrics
    mutex metrics_mutex;
    map<LatencyType, vector<LatencyMetric>> latency_metrics;
    map<string, LatencyMetric> active_measurements;
};

// Global singleton accessor
LatencyTracker& getLatencyTracker();

#endif // LATENCY_TRACKER_H