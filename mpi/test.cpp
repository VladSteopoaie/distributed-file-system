// combined.cpp
#include <mpi.h>
#include <iostream>
#include <string>
#include <array>
#include <memory>
#include <cstdio>

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    
    // Remove trailing newline if present
    if (!result.empty() && result[result.size()-1] == '\n') {
        result.erase(result.size()-1);
    }
    
    return result;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        // Rank 0 behavior (original first.cpp)
        std::cout << "I am rank " << rank << std::endl;
        std::string message = exec("hostname");
        int message_length = message.length() + 1;
        MPI_Send(&message_length, 1, MPI_INT, 1, 11, MPI_COMM_WORLD);
        MPI_Send(message.c_str(), message_length, MPI_CHAR, 1, 12, MPI_COMM_WORLD);
        
        int response_length;
        MPI_Recv(&response_length, 1, MPI_INT, 1, 13, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        char* response = new char[response_length];
        MPI_Recv(response, response_length, MPI_CHAR, 1, 14, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        std::cout << "Rank 0 received: " << response << std::endl;
        delete[] response;
    }
    else if (rank == 1) {
        // Rank 1 behavior (original second.cpp)
        std::cout << "I am rank " << rank << std::endl;
        int message_length;
        MPI_Recv(&message_length, 1, MPI_INT, 0, 11, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        char* message = new char[message_length];
        MPI_Recv(message, message_length, MPI_CHAR, 0, 12, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        std::cout << "Rank 1 received: " << message << std::endl;
        delete[] message;
        
        std::string response = exec("hostname");
        int response_length = response.length() + 1;
        MPI_Send(&response_length, 1, MPI_INT, 0, 13, MPI_COMM_WORLD);
        MPI_Send(response.c_str(), response_length, MPI_CHAR, 0, 14, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}