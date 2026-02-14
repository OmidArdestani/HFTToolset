#pragma once
// ============================================================================
// HFTToolset — Market Data Engine
// Manages market data dissemination (L1/L2/L3) with configurable throttling.
// Uses HPRingBuffer for lock-free cross-thread message passing.
// ============================================================================

#include <functional>
#include <HPRingBuffer.hpp>
#include <memory>
#include <vector>

#include "common/clock.h"
#include "common/types.h"
#include "market/matching_engine.h"

namespace hft_sim
{

/// Market data update types.
enum class MDUpdateType : std::uint8_t
{
    SnapshotFull,        // Full refresh
    IncrementalRefresh,  // Delta update
    TradeReport          // Trade event
};

/// Market data message envelope.
struct MarketDataMessage
{
    MDUpdateType type = MDUpdateType::SnapshotFull;
    void inject_replace( const HFTToolset::ReplaceRequest& replace );
    HFTToolset::Symbol symbol;
    HFTToolset::Timestamp timestamp = 0;

    union
    {
        HFTToolset::TopOfBook tob;
        HFTToolset::DepthSnapshot depth;
        HFTToolset::Trade trade;
    };

    MarketDataMessage() : tob{} {}
};

/// Market Data Engine — aggregates and publishes market data.
class MarketDataEngine
{
public:
    using L1Handler    = std::function<void( const HFTToolset::TopOfBook& )>;
    using L2Handler    = std::function<void( const HFTToolset::DepthSnapshot& )>;
    using TradeHandler = std::function<void( const HFTToolset::Trade& )>;

    explicit MarketDataEngine( HFTToolset::Clock& clock );

    // ── Configure ──────────────────────────────────────────────────────
    void set_throttle_interval_ns( std::uint64_t ns ) { throttle_ns_ = ns; }

    void set_l2_depth( std::size_t depth ) { l2_depth_ = depth; }

    // ── Feed handling ──────────────────────────────────────────────────
    /// Called when the matching engine produces a TOB update.
    void on_tob_update( const HFTToolset::TopOfBook& tob );

    /// Called when the matching engine produces a depth update.
    void on_depth_update( const HFTToolset::DepthSnapshot& depth );

    /// Called when a trade occurs.
    void on_trade( const HFTToolset::Trade& trade );

    /// Process pending market data (called from MD publisher thread).
    void process_pending();

    // ── Subscribe ──────────────────────────────────────────────────────
    void subscribe_l1( L1Handler handler ) { l1_handlers_.push_back( std::move( handler ) ); }

    void subscribe_l2( L2Handler handler ) { l2_handlers_.push_back( std::move( handler ) ); }

    void subscribe_trades( TradeHandler handler ) { trade_handlers_.push_back( std::move( handler ) ); }

    // ── Stats ──────────────────────────────────────────────────────────
    [[nodiscard]] std::uint64_t messages_published() const { return messages_published_; }

    [[nodiscard]] std::uint64_t messages_throttled() const { return messages_throttled_; }

private:
    HFTToolset::Clock& clock_;

    // Throttling
    std::uint64_t throttle_ns_ = 0;  // 0 = no throttling
    std::size_t l2_depth_      = 10;

    // Per-symbol last publish timestamp for throttling
    std::unordered_map<HFTToolset::Symbol, HFTToolset::Timestamp, HFTToolset::SymbolHash> last_publish_time_;

    // Lock-free queue for cross-thread MD messages (heap-allocated, ~12 MB)
    std::unique_ptr<HPRingBuffer<MarketDataMessage, 16384>> md_queue_;

    // Subscriber callbacks
    std::vector<L1Handler> l1_handlers_;
    std::vector<L2Handler> l2_handlers_;
    std::vector<TradeHandler> trade_handlers_;

    // Statistics
    std::uint64_t messages_published_ = 0;
    std::uint64_t messages_throttled_ = 0;

    bool should_throttle( const HFTToolset::Symbol& symbol, HFTToolset::Timestamp now );
};

}  // namespace hft_sim
