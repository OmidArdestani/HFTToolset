#pragma once
// ============================================================================
// HFTToolset — L1 Feed
// Computes and publishes top-of-book statistics: best bid/ask,
// spread, mid-price, microprice, VWAP, and rolling spread.
// ============================================================================

#include "common/types.h"
#include "orderbook/l3_order_book.h"
#include <functional>
#include <deque>

namespace HFTToolset {

/// L1 Feed — computes and publishes top-of-book statistics.
class L1Feed {
public:
    using L1Callback = std::function<void(const HFTToolset::TopOfBook&)>;

    explicit L1Feed(const L3OrderBook& book);

    /// Update and return current L1 data.
    [[nodiscard]] HFTToolset::TopOfBook update(HFTToolset::Timestamp ts);

    /// Get last known L1 data (no recomputation).
    [[nodiscard]] const HFTToolset::TopOfBook& last() const { return last_tob_; }

    /// Register callback for L1 updates.
    void on_update(L1Callback cb) { callback_ = std::move(cb); }

    // ── VWAP and statistics ────────────────────────────────────────────
    [[nodiscard]] double vwap() const;
    [[nodiscard]] double rolling_spread(std::size_t window) const;

    void record_trade(HFTToolset::Price price, HFTToolset::Quantity qty);
private:
    const L3OrderBook& book_;
    HFTToolset::TopOfBook last_tob_;
    L1Callback callback_;

    // Rolling trade data for VWAP
    struct TradeRecord {
        HFTToolset::Price    price;
        HFTToolset::Quantity quantity;
    };
    std::deque<TradeRecord> recent_trades_;
    static constexpr std::size_t MAX_TRADE_HISTORY = 1000;

    // Rolling spread data
    std::deque<HFTToolset::Price> spread_history_;
    static constexpr std::size_t MAX_SPREAD_HISTORY = 500;
};

} // namespace HFTToolset
