#include "../lib/cache_client.hpp"

std::string read_input(std::string message)
{
    std::cout << message << std::endl;
    std::cout << "> ";
    std::string input;
    std::getline(std::cin, input);
    return input;
}

void clear_pointer(CacheAPI::CacheClient*& p)
{
    if (p != NULL)
        delete p;
    p = NULL;
}

void print_menu()
{
    std::cout << "(1) Provide a configuration string for the memcached server." << std::endl;
    std::cout << "(2) Connect to a cache server." << std::endl;
    std::cout << "(3) Set an object." << std::endl;
    std::cout << "(4) Get an object." << std::endl;
    std::cout << "(5) Reset connection." << std::endl;
    std::cout << "(6) End connection." << std::endl;
    std::cout << "(m) Print this menu." << std::endl;
    std::cout << "(q) Quit." << std::endl;

}

int main()
{
    spdlog::set_level(spdlog::level::trace); // Set global log level to debug
    spdlog::set_pattern("(%s:%#) [%^%l%$] %v");
    CacheAPI::CacheClient *client = NULL;
    std::string address, port, option, mem_conf_string;
    bool connected = false;

    print_menu();

    while (true) {
        try {
            while (true) {
                option = read_input("Choose an option");
                
                if (option == "q") {
                    std::cout << "Exiting..." << std::endl;
                    clear_pointer(client);
                    exit(0);
                } else if (option == "m") { 
                    print_menu();
                    continue;
                } else if (option == "1") {
                    mem_conf_string = read_input("Provide the configuration string");

                    try {
                        Utils::prepare_conf_string(mem_conf_string);
                    }
                    catch(...)
                    {
                        std::cout << "Invalid configuration string!" << std::endl;
                    }
                } else if (option == "2" || option == "5") {
                    if (option == "5") {
                        if (!connected)
                        {
                            std::cout << "No connection to reset." << std::endl;
                            continue;
                        }
                        std::cout << "Resetting connection." << std::endl;
                    }

                    clear_pointer(client);
                    connected = false;
                    client = new CacheAPI::CacheClient(mem_conf_string);

                    if (mem_conf_string.length() > 0) {
                        std::cout << "Using configuration string: " << mem_conf_string << "." << std::endl;
                    }
                    else {
                        std::cout << "Connecting withou a configuration string." << std::endl;
                    }

                    if (option == "2") {
                        address = read_input("Provide the server's IP address");
                        port = read_input("Provide the server's port");
                    }

                    client->connect(address, port);
                    connected = true;
                    std::cout << "Connected successfully." << std::endl;
                } else if (option == "3") {
                    if (!connected)
                    {
                        std::cout << "Client not connected." << std::endl;
                        continue;
                    }

                    std::string key = read_input("Provide the key");
                    std::string value = read_input("Provide the value");

                    client->set(key, value);
                    std::cout << "Stored." << std::endl;
                } else if (option == "4") {
                    if (!connected)
                    {
                        std::cout << "Client not connected." << std::endl;
                        continue;
                    }

                    std::string key = read_input("Provide the key");
                    std::string value = client->get(key);
                    if (value.length() <= 0)
                        std::cout << "No value with key " << key << std::endl;
                    else 
                        std::cout << value << std::endl;
                } else if (option == "6") {
                    if (!connected)
                    {
                        std::cout << "No connection to end." << std::endl;
                        continue;
                    }

                    clear_pointer(client);
                    connected = false;
                    std::cout << "Connection ended." << std::endl;
                } else {
                    std::cout << "Invalid option, try again." << std::endl;
                    continue;
                }
            }
        }
        catch (std::exception& e)
        {
            SPDLOG_ERROR(e.what());
            clear_pointer(client);
            mem_conf_string = "";
        }
    }

    return 0;
}