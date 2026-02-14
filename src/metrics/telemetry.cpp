// ============================================================================
// HFT Exchange Simulator — Telemetry Implementation
// ============================================================================

#include "telemetry.h"

using namespace HFTToolset;

Telemetry::Telemetry(const Clock& clock) : clock_(clock) {}

Telemetry::~Telemetry() = default;

// ── Latency Recording ──────────────────────────────────────────────────────
void Telemetry::record_e2e_latency(std::int64_t ns)         { e2e_latency_.record(ns); }
void Telemetry::record_matching_latency(std::int64_t ns)     { matching_latency_.record(ns); }
void Telemetry::record_md_latency(std::int64_t ns)           { md_latency_.record(ns); }
void Telemetry::record_fix_inbound_latency(std::int64_t ns)  { fix_in_latency_.record(ns); }
void Telemetry::record_fix_outbound_latency(std::int64_t ns) { fix_out_latency_.record(ns); }

void Telemetry::record_fill(Quantity qty, Price price) {
    total_fill_qty_.fetch_add(static_cast<std::uint64_t>(qty), std::memory_order_relaxed);
    total_fill_notional_.fetch_add(
        static_cast<std::uint64_t>(qty * price), std::memory_order_relaxed);
}

void Telemetry::record_queue_position(std::uint32_t pos) {
    queue_pos_hist_.record(static_cast<std::int64_t>(pos));
}

void Telemetry::record_fill_probability(double prob) {
    fill_prob_sum_ += prob;
    fill_prob_count_++;
}

void Telemetry::record_slippage(double ticks) {
    slippage_sum_ += ticks;
    slippage_count_++;
}

// ── Timer Integration ──────────────────────────────────────────────────────
void Telemetry::start_timer(const std::string& name) {
    std::lock_guard<std::mutex> lock(timer_mutex_);
    active_timers_[name] = {std::chrono::steady_clock::now()};
}

void Telemetry::end_timer(const std::string& name) {
    auto end = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(timer_mutex_);
    auto it = active_timers_.find(name);
    if (it != active_timers_.end()) {
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end - it->second.start).count();
        // Route to appropriate histogram based on name
        if (name.find("e2e") != std::string::npos) {
            e2e_latency_.record(ns);
        } else if (name.find("match") != std::string::npos) {
            matching_latency_.record(ns);
        } else if (name.find("md") != std::string::npos) {
            md_latency_.record(ns);
        }
        active_timers_.erase(it);
    }
}

// ── Export ──────────────────────────────────────────────────────────────────
nlohmann::json Telemetry::stats_to_json(const LatencyStats& s) {
    return {
        {"samples", s.samples},
        {"min_ns", s.min_ns},
        {"max_ns", s.max_ns},
        {"mean_ns", s.mean_ns},
        {"median_ns", s.median_ns},
        {"p50_ns", s.p50_ns},
        {"p90_ns", s.p90_ns},
        {"p95_ns", s.p95_ns},
        {"p99_ns", s.p99_ns},
        {"p999_ns", s.p999_ns}
    };
}

nlohmann::json Telemetry::to_json() const {
    nlohmann::json j;

    j["counters"] = {
        {"orders", orders_.load(std::memory_order_relaxed)},
        {"trades", trades_.load(std::memory_order_relaxed)},
        {"cancels", cancels_.load(std::memory_order_relaxed)},
        {"rejects", rejects_.load(std::memory_order_relaxed)},
        {"fill_quantity", total_fill_qty_.load(std::memory_order_relaxed)},
        {"fill_notional", total_fill_notional_.load(std::memory_order_relaxed)}
    };

    j["latency"] = {
        {"end_to_end", stats_to_json(e2e_latency_.stats())},
        {"matching_engine", stats_to_json(matching_latency_.stats())},
        {"market_data", stats_to_json(md_latency_.stats())},
        {"fix_inbound", stats_to_json(fix_in_latency_.stats())},
        {"fix_outbound", stats_to_json(fix_out_latency_.stats())}
    };

    j["queue"] = {
        {"position_stats", stats_to_json(queue_pos_hist_.stats())},
        {"avg_fill_probability", fill_prob_count_ > 0 ? fill_prob_sum_ / static_cast<double>(fill_prob_count_) : 0.0},
        {"avg_slippage_ticks", slippage_count_ > 0 ? slippage_sum_ / static_cast<double>(slippage_count_) : 0.0}
    };

    return j;
}

void Telemetry::export_json(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (file.is_open()) {
        file << to_json().dump(2) << std::endl;
    }
}

void Telemetry::export_csv(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) return;

    file << "metric,value\n";
    file << "orders," << orders_.load() << "\n";
    file << "trades," << trades_.load() << "\n";
    file << "cancels," << cancels_.load() << "\n";
    file << "rejects," << rejects_.load() << "\n";

    auto e2e = e2e_latency_.stats();
    file << "e2e_p50_ns," << e2e.p50_ns << "\n";
    file << "e2e_p99_ns," << e2e.p99_ns << "\n";
    file << "e2e_p999_ns," << e2e.p999_ns << "\n";

    auto me = matching_latency_.stats();
    file << "matching_p50_ns," << me.p50_ns << "\n";
    file << "matching_p99_ns," << me.p99_ns << "\n";
    file << "matching_p999_ns," << me.p999_ns << "\n";

    auto md = md_latency_.stats();
    file << "md_p50_ns," << md.p50_ns << "\n";
    file << "md_p99_ns," << md.p99_ns << "\n";
    file << "md_p999_ns," << md.p999_ns << "\n";
}

void Telemetry::print_dashboard() const {
    auto e2e = e2e_latency_.stats();
    auto me  = matching_latency_.stats();
    auto md  = md_latency_.stats();

    std::cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║          HFT Exchange Simulator — Dashboard              ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════╣\n";

    std::cout << "║ THROUGHPUT                                               ║\n";
    std::cout << "║   Orders: " << std::setw(12) << orders_.load()
              << "   Trades: " << std::setw(12) << trades_.load()
              << "        ║\n";
    std::cout << "║   Cancels:" << std::setw(12) << cancels_.load()
              << "   Rejects:" << std::setw(12) << rejects_.load()
              << "        ║\n";

    std::cout << "╠══════════════════════════════════════════════════════════╣\n";
    std::cout << "║ LATENCY (ns)           p50        p99       p999        ║\n";
    std::cout << "║   End-to-end:   " << std::setw(10) << e2e.p50_ns
              << std::setw(11) << e2e.p99_ns
              << std::setw(11) << e2e.p999_ns << "        ║\n";
    std::cout << "║   Matching:     " << std::setw(10) << me.p50_ns
              << std::setw(11) << me.p99_ns
              << std::setw(11) << me.p999_ns << "        ║\n";
    std::cout << "║   Market Data:  " << std::setw(10) << md.p50_ns
              << std::setw(11) << md.p99_ns
              << std::setw(11) << md.p999_ns << "        ║\n";

    std::cout << "╠══════════════════════════════════════════════════════════╣\n";
    std::cout << "║ FILL METRICS                                            ║\n";
    std::cout << "║   Volume:     " << std::setw(14) << total_fill_qty_.load()
              << "                          ║\n";
    if (fill_prob_count_ > 0) {
        std::cout << "║   Avg Fill P: " << std::setw(14)
                  << std::fixed << std::setprecision(4)
                  << fill_prob_sum_ / static_cast<double>(fill_prob_count_)
                  << "                          ║\n";
    }
    if (slippage_count_ > 0) {
        std::cout << "║   Avg Slip:   " << std::setw(14)
                  << std::fixed << std::setprecision(4)
                  << slippage_sum_ / static_cast<double>(slippage_count_)
                  << " ticks                    ║\n";
    }

    std::cout << "╚══════════════════════════════════════════════════════════╝\n";
}

void Telemetry::print_latency_summary() const {
    auto e2e = e2e_latency_.stats();
    auto me  = matching_latency_.stats();

    std::cout << "  E2E   — p50: " << e2e.p50_ns << " ns, p99: " << e2e.p99_ns
              << " ns, p999: " << e2e.p999_ns << " ns (" << e2e.samples << " samples)\n";
    std::cout << "  Match — p50: " << me.p50_ns << " ns, p99: " << me.p99_ns
              << " ns, p999: " << me.p999_ns << " ns (" << me.samples << " samples)\n";
}

void Telemetry::reset() {
    e2e_latency_.reset();
    matching_latency_.reset();
    md_latency_.reset();
    fix_in_latency_.reset();
    fix_out_latency_.reset();
    queue_pos_hist_.reset();

    orders_.store(0);
    trades_.store(0);
    cancels_.store(0);
    rejects_.store(0);
    total_fill_qty_.store(0);
    total_fill_notional_.store(0);

    fill_prob_sum_   = 0.0;
    fill_prob_count_ = 0;
    slippage_sum_    = 0.0;
    slippage_count_  = 0;
}
