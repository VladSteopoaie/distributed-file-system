#include "server_api.hpp"

template <typename ConnectionHandler>
GenericServer<ConnectionHandler>::GenericServer(int thread_count)
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

template <typename ConnectionHandler>
template <typename... Args>
void GenericServer<ConnectionHandler>::run(uint16_t port, Args&& ...args)
{
    try {
        auto conn_handler = std::make_shared<ConnectionHandler>(context, std::forward<Args>(args)...);
        tcp::endpoint endpoint(tcp::v4(), port);
        acceptor.open(endpoint.protocol());
        acceptor.set_option(tcp::acceptor::reuse_address(true));
        acceptor.bind(endpoint);
        acceptor.listen();

        SPDLOG_INFO("Server running on {}:{}", endpoint.address().to_string(), endpoint.port());

        acceptor.async_accept(conn_handler->socket(), 
            [conn_handler](std::error_code ec) {
                handle_connection(conn_handler, ec);
            });

        for (int i = 0; i < thread_count; i ++)
        {
            thread_pool.emplace_back([this]() { context.run(); });
        }

    }
    catch (std::exception& e)
    {
        throw std::runtime_error(e.what());
    }
}

template <typename ConnectionHandler>
void GenericServer<ConnectionHandler>::handle_connection(shared_handler_t handler, std::error_code const& error)
{
    if (error) { return; }

    handler->start();

    auto new_handler = std::make_shared<ConnectionHandler>(context);

    acceptor.async_accept(new_handler->get_socket(), 
            [new_handler](std::error_code ec) {
                handle_connection(new_handler, ec);
            });
}