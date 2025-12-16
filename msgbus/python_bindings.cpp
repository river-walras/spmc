#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include "market_data.hpp"
#include "market_data_hub.hpp"

namespace py = pybind11;
using namespace marketdata;

// Python callback wrapper
// 这个wrapper负责处理GIL(Global Interpreter Lock)
class PyCallbackWrapper {
public:
    PyCallbackWrapper(py::object callback) : callback_(callback) {}

    void operator()(DataType data_type, const void* data_ptr) {
        // 获取 GIL (因为要调用Python代码)
        py::gil_scoped_acquire acquire;

        try {
            switch (data_type) {
                case DataType::KLINE: {
                    const Kline* kline = static_cast<const Kline*>(data_ptr);
                    // 创建Python对象并传递给回调
                    py::dict result;
                    result["timestamp"] = kline->timestamp;
                    result["open"] = kline->open;
                    result["high"] = kline->high;
                    result["low"] = kline->low;
                    result["close"] = kline->close;
                    result["volume"] = kline->volume;
                    result["symbol"] = std::string(kline->symbol, strnlen(kline->symbol, sizeof(kline->symbol)));
                    callback_("kline", result);
                    break;
                }
                case DataType::TRADE: {
                    const Trade* trade = static_cast<const Trade*>(data_ptr);
                    py::dict result;
                    result["timestamp"] = trade->timestamp;
                    result["price"] = trade->price;
                    result["quantity"] = trade->quantity;
                    result["symbol"] = std::string(trade->symbol, strnlen(trade->symbol, sizeof(trade->symbol)));
                    result["is_buyer_maker"] = trade->is_buyer_maker;
                    callback_("trade", result);
                    break;
                }
                case DataType::BOOK_L1: {
                    const BookL1* book = static_cast<const BookL1*>(data_ptr);
                    py::dict result;
                    result["timestamp"] = book->timestamp;
                    result["bid_price"] = book->bid_price;
                    result["bid_quantity"] = book->bid_quantity;
                    result["ask_price"] = book->ask_price;
                    result["ask_quantity"] = book->ask_quantity;
                    result["symbol"] = std::string(book->symbol, strnlen(book->symbol, sizeof(book->symbol)));
                    callback_("book_l1", result);
                    break;
                }
            }
        } catch (const std::exception& e) {
            // 捕获异常避免C++线程崩溃
            py::print("Error in callback:", e.what());
        }
    }

private:
    py::object callback_;
};

PYBIND11_MODULE(_core, m) {
    m.doc() = "msgbus C++ core module - High performance SPMC market data distribution";

    // 绑定 DataType 枚举
    py::enum_<DataType>(m, "DataType")
        .value("KLINE", DataType::KLINE)
        .value("TRADE", DataType::TRADE)
        .value("BOOK_L1", DataType::BOOK_L1)
        .export_values();

    // 绑定 Kline 结构体
    py::class_<Kline>(m, "Kline")
        .def(py::init<>())
        .def_readwrite("timestamp", &Kline::timestamp)
        .def_readwrite("open", &Kline::open)
        .def_readwrite("high", &Kline::high)
        .def_readwrite("low", &Kline::low)
        .def_readwrite("close", &Kline::close)
        .def_readwrite("volume", &Kline::volume)
        .def_property("symbol",
            [](const Kline& k) { return std::string(k.symbol); },
            [](Kline& k, const std::string& s) {
                strncpy(k.symbol, s.c_str(), sizeof(k.symbol) - 1);
                k.symbol[sizeof(k.symbol) - 1] = '\0';
            });

    // 绑定 Trade 结构体
    py::class_<Trade>(m, "Trade")
        .def(py::init<>())
        .def_readwrite("timestamp", &Trade::timestamp)
        .def_readwrite("price", &Trade::price)
        .def_readwrite("quantity", &Trade::quantity)
        .def_readwrite("is_buyer_maker", &Trade::is_buyer_maker)
        .def_property("symbol",
            [](const Trade& t) { return std::string(t.symbol); },
            [](Trade& t, const std::string& s) {
                strncpy(t.symbol, s.c_str(), sizeof(t.symbol) - 1);
                t.symbol[sizeof(t.symbol) - 1] = '\0';
            });

    // 绑定 BookL1 结构体
    py::class_<BookL1>(m, "BookL1")
        .def(py::init<>())
        .def_readwrite("timestamp", &BookL1::timestamp)
        .def_readwrite("bid_price", &BookL1::bid_price)
        .def_readwrite("bid_quantity", &BookL1::bid_quantity)
        .def_readwrite("ask_price", &BookL1::ask_price)
        .def_readwrite("ask_quantity", &BookL1::ask_quantity)
        .def_property("symbol",
            [](const BookL1& b) { return std::string(b.symbol); },
            [](BookL1& b, const std::string& s) {
                strncpy(b.symbol, s.c_str(), sizeof(b.symbol) - 1);
                b.symbol[sizeof(b.symbol) - 1] = '\0';
            });

    // 绑定 MarketDataHub
    py::class_<MarketDataHub>(m, "MarketDataHub")
        .def(py::init<>())
        .def("add_kline", [](MarketDataHub& hub, const Kline& kline, bool release_gil) {
            if (release_gil) {
                py::gil_scoped_release release;
                hub.add(MarketData(kline));
            } else {
                hub.add(MarketData(kline));
            }
        }, py::arg("kline"), py::arg("release_gil") = false,
           "Add Kline data to the hub\n"
           "Note: `release_gil=True` can reduce GIL blocking for consumer callbacks, but adds overhead per call.")
        .def("add_trade", [](MarketDataHub& hub, const Trade& trade, bool release_gil) {
            if (release_gil) {
                py::gil_scoped_release release;
                hub.add(MarketData(trade));
            } else {
                hub.add(MarketData(trade));
            }
        }, py::arg("trade"), py::arg("release_gil") = false,
           "Add Trade data to the hub\n"
           "Note: `release_gil=True` can reduce GIL blocking for consumer callbacks, but adds overhead per call.")
        .def("add_book_l1", [](MarketDataHub& hub, const BookL1& book, bool release_gil) {
            if (release_gil) {
                py::gil_scoped_release release;
                hub.add(MarketData(book));
            } else {
                hub.add(MarketData(book));
            }
        }, py::arg("book"), py::arg("release_gil") = false,
           "Add BookL1 data to the hub\n"
           "Note: `release_gil=True` can reduce GIL blocking for consumer callbacks, but adds overhead per call.")
        .def("add_klines", [](MarketDataHub& hub, const std::vector<Kline>& klines) {
            py::gil_scoped_release release;
            for (const auto& kline : klines) {
                hub.add(MarketData(kline));
            }
        }, py::arg("klines"),
           "Add a batch of Kline messages (releases the GIL once for the whole batch).")
        .def("add_trades", [](MarketDataHub& hub, const std::vector<Trade>& trades) {
            py::gil_scoped_release release;
            for (const auto& trade : trades) {
                hub.add(MarketData(trade));
            }
        }, py::arg("trades"),
           "Add a batch of Trade messages (releases the GIL once for the whole batch).")
        .def("add_books_l1", [](MarketDataHub& hub, const std::vector<BookL1>& books) {
            py::gil_scoped_release release;
            for (const auto& book : books) {
                hub.add(MarketData(book));
            }
        }, py::arg("books"),
           "Add a batch of BookL1 messages (releases the GIL once for the whole batch).")
        .def("subscribe", [](MarketDataHub& hub, DataType data_type, py::object callback) {
            // 创建 C++ callback wrapper
            auto wrapper = std::make_shared<PyCallbackWrapper>(callback);
            PyCallback cpp_callback = [wrapper](DataType dt, const void* ptr) {
                (*wrapper)(dt, ptr);
            };

            // 释放 GIL 让 C++ 线程可以运行
            py::gil_scoped_release release;
            return hub.subscribe(data_type, std::move(cpp_callback));
        }, py::arg("data_type"), py::arg("callback"),
           "Subscribe to market data with a callback function\n"
           "Callback signature: callback(data_type: str, data: dict)")
        .def("unsubscribe", [](MarketDataHub& hub, int subscriber_id) {
            py::gil_scoped_release release;
            hub.unsubscribe(subscriber_id);
        }, "Unsubscribe from market data")
        .def("stop_all", [](MarketDataHub& hub) {
            py::gil_scoped_release release;
            hub.stop_all();
        }, "Stop all subscriptions")
        .def("subscriber_count", &MarketDataHub::subscriber_count,
             "Get current subscriber count");

    // 绑定 MockCppProducer
    py::class_<MockCppProducer>(m, "MockCppProducer",
        "C++ mock producer for performance testing - generates data in pure C++ without GIL")
        .def(py::init<MarketDataHub*>(),
             py::arg("hub"),
             "Create a mock C++ producer")
        .def("start", [](MockCppProducer& producer, uint64_t num_messages, int message_type) {
            // 释放 GIL，让 C++ 线程自由运行
            py::gil_scoped_release release;
            producer.start(num_messages, message_type);
        }, py::arg("num_messages"), py::arg("message_type") = 0,
           "Start producing messages in C++ thread\n"
           "Args:\n"
           "  num_messages: Number of messages to produce\n"
           "  message_type: 0=Trade (default), 1=Kline, 2=BookL1")
        .def("stop", [](MockCppProducer& producer) {
            py::gil_scoped_release release;
            producer.stop();
        }, "Stop the producer thread")
        .def("wait", [](MockCppProducer& producer) {
            py::gil_scoped_release release;
            producer.wait();
        }, "Wait for producer to finish")
        .def("messages_produced", &MockCppProducer::messages_produced,
             "Get the number of messages produced");
}
