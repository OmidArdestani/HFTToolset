#pragma once

#include "types.h"
#include <vector>
#include <functional>

#define MAKE_TOP_OF_BOOK_HANDLER(fn) \
    std::bind(&fn, this, std::placeholders::_1)

#define MAKE_TRADE_HANDLER(fn) \
    std::bind(&fn, this, std::placeholders::_1)

#define MAKE_DEPTH_SNAPSHOT_HANDLER(fn) \
    std::bind(&fn, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)

namespace MarketMicroStructure {

class MarketDataPublisher {
public:
    using TopOfBookHandler     = std::function<void(const TopOfBook&)>;
    using TradeHandler         = std::function<void(const Trade&)>;
    using DepthSnapshotHandler = std::function<void(const SymbolId&, const std::vector<BookLevel>& bids, const std::vector<BookLevel>& asks)>;

    void onTopOfBook(TopOfBookHandler cb);
    void onTrade(TradeHandler cb);
    void onDepthSnapshot(DepthSnapshotHandler cb);
    void publishTopOfBook(const TopOfBook& tob) const;
    void publishTrade(const Trade& t) const;
    void publishDepth(const SymbolId& sym, const std::vector<BookLevel>& bids, const std::vector<BookLevel>& asks) const;

private:
    TopOfBookHandler     tob_handler_;
    TradeHandler         trade_handler_;
    DepthSnapshotHandler depth_handler_;
};

} // namespace MarketMicroStructure
