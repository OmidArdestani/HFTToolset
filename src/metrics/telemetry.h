#pragma once
// ============================================================================
// HFT Exchange Simulator — Telemetry & Metrics
// Comprehensive metrics collection with JSON/CSV export and console dashboard.
// Uses HFTToolset's ScopeTimer for precise timing.
// ============================================================================

#include <atomic>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <ScopeTimer.hpp>
#include <string>
#include <unordered_map>

#include "clock.h"
#include "latency_histogram.h"
#include "types.h"

namespace HFTToolset
{

/// Telemetry system — collects, aggregates, and exports simulator metrics.
class Telemetry
{
public:
    explicit Telemetry( const Clock& clock );
    ~Telemetry();

    // ── Latency Recording ──────────────────────────────────────────────
    void record_e2e_latency( std::int64_t ns );           // FIX → trade → FIX
    void record_matching_latency( std::int64_t ns );      // matching engine only
    void record_md_latency( std::int64_t ns );            // market data dissemination
    void record_fix_inbound_latency( std::int64_t ns );   // FIX inbound processing
    void record_fix_outbound_latency( std::int64_t ns );  // FIX outbound processing

    // ── Counter Recording ──────────────────────────────────────────────
    void record_order() { orders_.fetch_add( 1, std::memory_order_relaxed ); }

    void record_trade() { trades_.fetch_add( 1, std::memory_order_relaxed ); }

    void record_cancel() { cancels_.fetch_add( 1, std::memory_order_relaxed ); }

    void record_reject() { rejects_.fetch_add( 1, std::memory_order_relaxed ); }

    void record_fill( Quantity qty, Price price );

    // ── Queue / Fill Metrics ───────────────────────────────────────────
    void record_queue_position( std::uint32_t pos );
    void record_fill_probability( double prob );
    void record_slippage( double ticks );

    // ── Export ──────────────────────────────────────────────────────────
    /// Export all metrics as JSON.
    [[nodiscard]] nlohmann::json to_json() const;

    /// Write metrics to JSON file.
    void export_json( const std::string& filepath ) const;

    /// Write metrics to CSV file.
    void export_csv( const std::string& filepath ) const;

    /// Print real-time console dashboard.
    void print_dashboard() const;

    /// Print latency summary.
    void print_latency_summary() const;

    // ── Control ────────────────────────────────────────────────────────
    void reset();

    void set_export_interval_ms( uint32_t ms ) { export_interval_ms_ = ms; }

    // ── ScopeTimer integration ─────────────────────────────────────────
    /// Start a named timer.
    void start_timer( const std::string& name );
    /// End a named timer and record the latency.
    void end_timer( const std::string& name );

private:
    const Clock& clock_;

    // Latency histograms
    LatencyHistogram<10000, 100> e2e_latency_;
    LatencyHistogram<10000, 100> matching_latency_;
    LatencyHistogram<10000, 100> md_latency_;
    LatencyHistogram<10000, 100> fix_in_latency_;
    LatencyHistogram<10000, 100> fix_out_latency_;

    // Counters
    std::atomic<std::uint64_t> orders_{ 0 };
    std::atomic<std::uint64_t> trades_{ 0 };
    std::atomic<std::uint64_t> cancels_{ 0 };
    std::atomic<std::uint64_t> rejects_{ 0 };
    std::atomic<std::uint64_t> total_fill_qty_{ 0 };
    std::atomic<std::uint64_t> total_fill_notional_{ 0 };

    // Queue/fill stats
    LatencyHistogram<1000, 1> queue_pos_hist_;  // queue position histogram
    double fill_prob_sum_          = 0.0;
    std::uint64_t fill_prob_count_ = 0;
    double slippage_sum_           = 0.0;
    std::uint64_t slippage_count_  = 0;

    // Timer tracking
    struct TimerEntry
    {
        std::chrono::steady_clock::time_point start;
    };

    std::unordered_map<std::string, TimerEntry> active_timers_;
    mutable std::mutex timer_mutex_;

    uint32_t export_interval_ms_ = 1000;

    // Helper: format latency stats as JSON
    static nlohmann::json stats_to_json( const LatencyStats& stats );
};

}  // namespace HFTToolset
