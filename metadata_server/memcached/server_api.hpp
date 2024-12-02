#ifndef SERVER_API_HPP
#define SERVER_API_HPP

#define ASIO_STANALONE // non-boost version
#define ASIO_NO_DEPRECATED // no need for deprecated stuff
#define ASIO_HAS_STD_COROUTINE // c++20 coroutines needed
#include <asio.hpp>
#include <spdlog/spdlog.h>

using asio::ip::tcp;

template <typename ConnectionHandler>
class GenericServer {
    using shared_handler_t = std::shared_ptr<ConnectionHandler>;
private:
    int thread_count;
    std::vector<std::thread> thread_pool;
    asio::io_context context;
    tcp::acceptor acceptor;
    asio::signal_set signals;

    template <typename... Args>
    void handle_connection(shared_handler_t handler, std::error_code const& error, Args&& ...args)
    {
        if (error) { return; }

        handler->start();

        auto new_handler = std::make_shared<ConnectionHandler>(context, std::forward<Args>(args)...);

        acceptor.async_accept(new_handler->get_socket(), 
                [new_handler, this, &args...](std::error_code ec) {
                    handle_connection(new_handler, ec, std::forward<Args>(args)...);
                });
    }

public:
    GenericServer(int thread_count)
        : thread_count(thread_count)
        , acceptor(context)
        , signals(context)
    {
        try {
            signals.add(SIGINT);
            signals.add(SIGTERM);
            signals.async_wait([&](auto, auto){ 
                SPDLOG_WARN("Server: Interupt detected. Exiting ...");
                context.stop(); 
            });
        }
        catch (std::exception& e)
        {
            throw std::runtime_error(e.what());
        }
    }

    template <typename... Args>
    void run(uint16_t port, Args&& ...args)
    {

        try {
            auto conn_handler = std::make_shared<ConnectionHandler>(context, std::forward<Args>(args)...);
            tcp::endpoint endpoint(tcp::v4(), port);
            acceptor.open(endpoint.protocol());
            acceptor.set_option(tcp::acceptor::reuse_address(true));
            acceptor.bind(endpoint);
            acceptor.listen();

            SPDLOG_INFO("Server running on {}:{}", endpoint.address().to_string(), endpoint.port());

            acceptor.async_accept(conn_handler->get_socket(), 
                [conn_handler, this, &args...](std::error_code ec) {
                    handle_connection(conn_handler, ec, std::forward<Args>(args)...);
                });

            for (int i = 0; i < thread_count; i ++)
            {
                thread_pool.emplace_back([&]() { context.run(); });
            }
        
            context.run();

            for (auto& t : thread_pool)
            {
                if (t.joinable())
                    t.join();
            }
        }
        catch (std::exception& e)
        {
            throw std::runtime_error(e.what());
        }
    }

};

#endif