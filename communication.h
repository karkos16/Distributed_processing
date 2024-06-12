#pragma once
#include <mpi.h>
#include <iostream>


#include "constants.h"


using namespace std;

struct Message {
    int timestamp;
    int touristId;
    int type;  

    bool operator<(const Message& other) const {
        if (timestamp == other.timestamp) {
            return touristId > other.touristId;
        } else {
            return timestamp > other.timestamp;
        }
    } 
};

int incrementTimestamp(int currentLamport, int timestamp) {
  return max(currentLamport, timestamp) + 1;
}

void sendMessage(int to, int type, int timestamp, int pid) {
    Message msg = {timestamp, pid, type};
    MPI_Send(&msg, 3, MPI_INT, to, 0, MPI_COMM_WORLD);
}

void receiveMessage(Message &msg) {
    MPI_Status status;
    MPI_Recv(&msg, 3, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
}

