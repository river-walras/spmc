#pragma once

#include "spmc.hpp"
#include "market_data.hpp"
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace marketdata {

// Python callback 函数类型定义
// 参数: DataType (数据类型), void* (数据指针指向 Kline/Trade/BookL1)
using PyCallback = std::function<void(DataType, const void*)>;

// 订阅者信息
struct Subscriber {
    int id;                          // 订阅者ID
    DataType data_type;              // 订阅的数据类型
    PyCallback callback;             // Python回调函数
    std::unique_ptr<std::thread> thread;  // 后台线程
    bool running;                    // 线程运行状态

    // 根据数据类型创建对应的 Reader
    struct ReaderHolder {
        SPMCQueue<MarketData, 512>::Reader reader;
    };
    std::unique_ptr<ReaderHolder> reader_holder;

    Subscriber(int id, DataType type, PyCallback cb)
        : id(id), data_type(type), callback(std::move(cb)),
          running(false), reader_holder(std::make_unique<ReaderHolder>()) {}
};

/**
 * MarketDataHub - 市场数据分发中心
 *
 * 职责:
 * 1. 维护一个 SPMCQueue<MarketData>
 * 2. 提供 add() 接口给 Python 生产者调用
 * 3. 管理订阅者和后台线程
 * 4. 根据订阅类型过滤数据并调用 Python callback
 */
class MarketDataHub {
public:
    MarketDataHub() : next_subscriber_id_(0) {}

    ~MarketDataHub() {
        // 停止所有订阅者线程
        stop_all();
    }

    // 禁止拷贝和移动
    MarketDataHub(const MarketDataHub&) = delete;
    MarketDataHub& operator=(const MarketDataHub&) = delete;

    /**
     * 添加市场数据 (Python 生产者调用)
     * @param data 市场数据 (Kline/Trade/BookL1)
     */
    void add(const MarketData& data) {
        queue_.write(data);
    }

    /**
     * 订阅市场数据 (Python 消费者调用)
     * @param data_type 订阅的数据类型
     * @param callback Python 回调函数
     * @return 订阅ID (用于后续取消订阅)
     */
    int subscribe(DataType data_type, PyCallback callback) {
        std::lock_guard<std::mutex> lock(mutex_);

        int sub_id = next_subscriber_id_++;
        auto subscriber = std::make_unique<Subscriber>(sub_id, data_type, std::move(callback));

        // 为该订阅者创建 Reader
        subscriber->reader_holder->reader = queue_.getReader();
        subscriber->running = true;

        // 创建后台线程
        subscriber->thread = std::make_unique<std::thread>(
            &MarketDataHub::consumer_thread, this, sub_id
        );

        subscribers_[sub_id] = std::move(subscriber);

        return sub_id;
    }

    /**
     * 取消订阅
     * @param subscriber_id 订阅ID
     */
    void unsubscribe(int subscriber_id) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = subscribers_.find(subscriber_id);
        if (it != subscribers_.end()) {
            // 停止线程
            it->second->running = false;
            if (it->second->thread && it->second->thread->joinable()) {
                it->second->thread->join();
            }
            subscribers_.erase(it);
        }
    }

    /**
     * 停止所有订阅
     */
    void stop_all() {
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto& [id, subscriber] : subscribers_) {
            subscriber->running = false;
            if (subscriber->thread && subscriber->thread->joinable()) {
                subscriber->thread->join();
            }
        }
        subscribers_.clear();
    }

    /**
     * 获取当前订阅数量
     */
    size_t subscriber_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return subscribers_.size();
    }

private:
    /**
     * 消费者线程函数
     * @param subscriber_id 订阅者ID
     */
    void consumer_thread(int subscriber_id) {
        // 需要在循环外获取 subscriber 指针,避免每次循环都加锁
        Subscriber* subscriber = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = subscribers_.find(subscriber_id);
            if (it == subscribers_.end()) {
                return;
            }
            subscriber = it->second.get();
        }

        auto& reader = subscriber->reader_holder->reader;
        DataType target_type = subscriber->data_type;

        while (subscriber->running) {
            MarketData* data_ptr = reader.read();
            if (!data_ptr) {
                // 没有数据,短暂休眠避免CPU空转
                std::this_thread::sleep_for(std::chrono::microseconds(1));
                continue;
            }

            // 根据订阅类型过滤数据
            DataType current_type = static_cast<DataType>(data_ptr->index());
            if (current_type != target_type) {
                continue;  // 不是订阅的类型,跳过
            }

            // 拷贝数据以避免数据竞争
            // 生产者可能会覆写队列中的数据,所以需要拷贝一份
            MarketData data_copy = *data_ptr;

            // 调用 Python callback
            // 使用 std::visit 来安全地获取数据指针
            std::visit([&](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                subscriber->callback(current_type, &arg);
            }, data_copy);
        }
    }

    SPMCQueue<MarketData, 512> queue_;  // 市场数据队列
    std::unordered_map<int, std::unique_ptr<Subscriber>> subscribers_;  // 订阅者映射
    mutable std::mutex mutex_;  // 保护 subscribers_
    int next_subscriber_id_;    // 下一个订阅者ID
};

/**
 * MockCppProducer - C++ 模拟数据生产者
 *
 * 在纯 C++ 层生成数据，用于测试无 GIL 情况下的性能
 */
class MockCppProducer {
public:
    MockCppProducer(MarketDataHub* hub)
        : hub_(hub), running_(false), thread_(nullptr) {}

    ~MockCppProducer() {
        stop();
    }

    /**
     * 启动生产者线程
     * @param num_messages 要生成的消息数量
     * @param message_type 消息类型 (0=Trade, 1=Kline, 2=BookL1)
     */
    void start(uint64_t num_messages, int message_type = 0) {
        if (running_) return;

        running_ = true;
        num_messages_ = num_messages;
        message_type_ = message_type;

        thread_ = std::make_unique<std::thread>(&MockCppProducer::producer_thread, this);
    }

    /**
     * 停止生产者线程
     */
    void stop() {
        running_ = false;
        if (thread_ && thread_->joinable()) {
            thread_->join();
        }
    }

    /**
     * 等待生产完成
     */
    void wait() {
        if (thread_ && thread_->joinable()) {
            thread_->join();
        }
    }

    /**
     * 获取已生产的消息数量
     */
    uint64_t messages_produced() const {
        return messages_produced_;
    }

private:
    void producer_thread() {
        messages_produced_ = 0;

        for (uint64_t i = 0; i < num_messages_ && running_; ++i) {
            if (message_type_ == 0) {
                // 生成 Trade
                Trade trade;
                trade.timestamp = i;
                trade.price = 50000.0 + (i % 100);
                trade.quantity = 1.0;
                trade.is_buyer_maker = (i % 2 == 0);
                strncpy(trade.symbol, "BTCUSDT", sizeof(trade.symbol) - 1);
                trade.symbol[sizeof(trade.symbol) - 1] = '\0';

                hub_->add(MarketData(trade));
            } else if (message_type_ == 1) {
                // 生成 Kline
                Kline kline;
                kline.timestamp = i;
                kline.open = 50000.0;
                kline.high = 50100.0;
                kline.low = 49900.0;
                kline.close = 50000.0 + (i % 100);
                kline.volume = 100.0;
                strncpy(kline.symbol, "BTCUSDT", sizeof(kline.symbol) - 1);
                kline.symbol[sizeof(kline.symbol) - 1] = '\0';

                hub_->add(MarketData(kline));
            } else {
                // 生成 BookL1
                BookL1 book;
                book.timestamp = i;
                book.bid_price = 50000.0;
                book.bid_quantity = 10.0;
                book.ask_price = 50001.0;
                book.ask_quantity = 10.0;
                strncpy(book.symbol, "BTCUSDT", sizeof(book.symbol) - 1);
                book.symbol[sizeof(book.symbol) - 1] = '\0';

                hub_->add(MarketData(book));
            }

            ++messages_produced_;
        }
    }

    MarketDataHub* hub_;
    bool running_;
    std::unique_ptr<std::thread> thread_;
    uint64_t num_messages_;
    int message_type_;
    std::atomic<uint64_t> messages_produced_{0};
};

} // namespace marketdata
