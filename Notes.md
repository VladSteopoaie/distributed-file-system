# Some notes

## Cache server
### Questions and things to consider
- which arhchitecture is best? (co-routines, multi-threaded, or hybrid)
- should I use a single io_context approach, an io_context for each thread, or hybrid?
- what would be the architecture of the server?

Useful links:
- [Boost asio tutorial](https://www.boost.org/doc/libs/1_86_0/doc/html/boost_asio/tutorial.html)
- [Boost asio documentation](https://think-async.com/Asio/boost_asio_1_30_2/doc/html/boost_asio.html)