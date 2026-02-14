#pragma once
// ============================================================================
// HFTToolset — L3 Order Book
// Full order-level book with queue position tracking, iceberg support,
// FOK/IOC/GTC/Day time-in-force, and cancel-replace. Provides callbacks
// for trade, top-of-book, and depth events.
// ============================================================================

#include "types.h"
#include "constants.h"
#include <map>
#include <list>
#include <unordered_map>
#include <optional>
#include <vector>
#include <functional>


namespace HFTToolset {
/// Individual order entry in the L3 book.
struct alignas(64) L3Order {
    HFTToolset::OrderId     id            = HFTToolset::INVALID_ORDER_ID;
    HFTToolset::TraderId    trader_id     = 0;
    HFTToolset::Side        side          = HFTToolset::Side::Buy;
    HFTToolset::Price       price         = 0;
    HFTToolset::Quantity    visible_qty   = 0;    // displayed quantity
    HFTToolset::Quantity    hidden_qty    = 0;    // iceberg hidden portion
    HFTToolset::Quantity    filled_qty    = 0;
    HFTToolset::Timestamp   entry_time    = 0;
    std::uint32_t queue_pos   = 0;    // position in price-level queue
    bool        is_iceberg    = false;
    char        cl_ord_id[20] = {};
};

/// Aggregate info at a single price level.
struct PriceLevelInfo {
    HFTToolset::Price       price         = 0;
    HFTToolset::Quantity    total_visible = 0;
    HFTToolset::Quantity    total_hidden  = 0;
    std::uint32_t order_count = 0;
};

/// Result of a match operation.
struct MatchResult {
    std::vector<HFTToolset::Trade> trades;
    HFTToolset::Quantity           remaining_qty = 0;
    bool               fully_filled  = false;
};

/// L3 Order Book — price-time priority with full order-level granularity.
class L3OrderBook {
public:
    // Callback types
    using TradeCallback       = std::function<void(const HFTToolset::Trade&)>;
    using TopOfBookCallback   = std::function<void(const HFTToolset::TopOfBook&)>;
    using DepthCallback       = std::function<void(const HFTToolset::Symbol&,
                                    const std::vector<HFTToolset::BookLevel>& bids,
                                    const std::vector<HFTToolset::BookLevel>& asks)>;
    explicit L3OrderBook(const HFTToolset::Symbol& symbol);

    // ── Order operations ───────────────────────────────────────────────
    /// Submit a new order. Returns execution report.
    HFTToolset::ExecutionReport add_order(const HFTToolset::Order& order, HFTToolset::Timestamp ts);

    /// Cancel an existing order.
    HFTToolset::ExecutionReport cancel_order(HFTToolset::OrderId id, HFTToolset::Timestamp ts);

    /// Modify an existing order (cancel-replace).
    HFTToolset::ExecutionReport replace_order(const HFTToolset::ReplaceRequest& req, HFTToolset::Timestamp ts);

    // ── Query ──────────────────────────────────────────────────────────
    [[nodiscard]] const HFTToolset::Symbol& symbol() const { return symbol_; }

    [[nodiscard]] std::optional<HFTToolset::BookLevel> best_bid() const;
    [[nodiscard]] std::optional<HFTToolset::BookLevel> best_ask() const;
    [[nodiscard]] HFTToolset::TopOfBook top_of_book(HFTToolset::Timestamp ts) const;

    [[nodiscard]] std::vector<HFTToolset::BookLevel> bid_depth(std::size_t levels) const;
    [[nodiscard]] std::vector<HFTToolset::BookLevel> ask_depth(std::size_t levels) const;

    /// Get L3 orders at a specific price level.
    [[nodiscard]] std::vector<L3Order> orders_at_price(HFTToolset::Side side, HFTToolset::Price price) const;

    /// Queue position of a specific order.
    [[nodiscard]] std::uint32_t queue_position(HFTToolset::OrderId id) const;
    /// Total quantity ahead of an order in its price level.
    [[nodiscard]] HFTToolset::Quantity quantity_ahead(HFTToolset::OrderId id) const;

    [[nodiscard]] std::size_t total_bid_orders() const { return bid_order_count_; }
    [[nodiscard]] std::size_t total_ask_orders() const { return ask_order_count_; }

    // ── Callbacks ──────────────────────────────────────────────────────
    void on_trade(TradeCallback cb)           { trade_cb_ = std::move(cb); }
    void on_top_of_book(TopOfBookCallback cb) { tob_cb_ = std::move(cb); }
    void on_depth(DepthCallback cb)           { depth_cb_ = std::move(cb); }

    // ── Statistics ─────────────────────────────────────────────────────
    [[nodiscard]] std::uint64_t total_trades()   const { return trade_count_; }
    [[nodiscard]] HFTToolset::Quantity      total_volume()    const { return total_volume_; }
    [[nodiscard]] HFTToolset::Price         last_trade_price() const { return last_trade_price_; }

private:
    HFTToolset::Symbol symbol_;
    // ── Book data structures ───────────────────────────────────────────
    using OrderQueue    = std::list<L3Order>;
    using BidPriceLevels = std::map<HFTToolset::Price, OrderQueue, std::greater<HFTToolset::Price>>;
    using AskPriceLevels = std::map<HFTToolset::Price, OrderQueue, std::less<HFTToolset::Price>>;

    BidPriceLevels bids_;
    AskPriceLevels asks_;

    // O(1) order lookup: OrderId -> location in book
    struct OrderLocation {
        HFTToolset::Side                   side;
        HFTToolset::Price                  price;
        OrderQueue::iterator   it;
    };
    std::unordered_map<HFTToolset::OrderId, OrderLocation> order_index_;
    // ── Matching logic ─────────────────────────────────────────────────
    MatchResult match_incoming(const HFTToolset::Order& order, HFTToolset::Timestamp ts);

    template <typename PriceLevels>
    void match_against(PriceLevels& levels, const HFTToolset::Order& incoming,
                       HFTToolset::Quantity& remaining, std::vector<HFTToolset::Trade>& trades,
                       HFTToolset::Timestamp ts);

    void add_resting_order(const HFTToolset::Order& order, HFTToolset::Quantity remaining, HFTToolset::Timestamp ts);
    void update_queue_positions(HFTToolset::Side side, HFTToolset::Price price);
    void publish_tob(HFTToolset::Timestamp ts);
    // ── Counters ───────────────────────────────────────────────────────
    std::uint64_t trade_count_    = 0;
    std::uint64_t next_trade_id_  = 1;
    HFTToolset::Quantity      total_volume_   = 0;
    HFTToolset::Price         last_trade_price_ = 0;
    std::size_t   bid_order_count_ = 0;
    std::size_t   ask_order_count_ = 0;

    // ── Callbacks ──────────────────────────────────────────────────────
    TradeCallback     trade_cb_;
    TopOfBookCallback tob_cb_;
    DepthCallback     depth_cb_;
};

} // namespace HFTToolset
