#pragma once
#include <mpi.h>
#include <iostream>


#include "constants.h"


using namespace std;

struct Message {
    int timestamp;
    int touristId;
    int type;  
    vector<int> groupMembers;

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


void sendPackedMessage(int to, int type, int timestamp, int pid, const vector<int>& groupMembers) {
    int position = 0;
    int size = 0;

    // OBLICZENIE ROZMIARU WIADOMOSCI
    MPI_Pack_size(3, MPI_INT, MPI_COMM_WORLD, &size); // TIMESTAMP, PID, TYPE
    int vectorSize = groupMembers.size();
    size += sizeof(int); // ROZMIAR WEKTORA
    size += vectorSize * sizeof(int); // ELEMENTY WEKTORA

    char* buffer = new char[size];

    // PAKOWANIE WIADOMOSCI
    MPI_Pack(&timestamp, 1, MPI_INT, buffer, size, &position, MPI_COMM_WORLD);
    MPI_Pack(&pid, 1, MPI_INT, buffer, size, &position, MPI_COMM_WORLD);
    MPI_Pack(&type, 1, MPI_INT, buffer, size, &position, MPI_COMM_WORLD);
    MPI_Pack(&vectorSize, 1, MPI_INT, buffer, size, &position, MPI_COMM_WORLD);
    MPI_Pack(groupMembers.data(), vectorSize, MPI_INT, buffer, size, &position, MPI_COMM_WORLD);

    // WYSYLANIE WIADOMOSCI
    MPI_Send(buffer, position, MPI_PACKED, to, 0, MPI_COMM_WORLD);

    // CZYSZCZENIE
    delete[] buffer;
}

void receivePackedMessage(Message &msg) {
    MPI_Status status;
    int size;
    int position = 0;

    // POBIERZ ROZMIAR WIADOMOSCI, NIE BEDZIE BLOKUJACE BO WCZESNIEJ W PROJEKT.CPP JEST ASYNCHRONICZNE MPI_PROBE
    MPI_Probe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, MPI_PACKED, &size);

    // ALOKACJA
    char* buffer = new char[size];

    // ODEBRANIE WIADOMOSCI
    MPI_Recv(buffer, size, MPI_PACKED, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

    // ODPAKOWANIE WIADOMOSCI
    MPI_Unpack(buffer, size, &position, &msg.timestamp, 1, MPI_INT, MPI_COMM_WORLD);
    MPI_Unpack(buffer, size, &position, &msg.touristId, 1, MPI_INT, MPI_COMM_WORLD);
    MPI_Unpack(buffer, size, &position, &msg.type, 1, MPI_INT, MPI_COMM_WORLD);
    
    int vectorSize;
    MPI_Unpack(buffer, size, &position, &vectorSize, 1, MPI_INT, MPI_COMM_WORLD);
    if (vectorSize != groupSize) {
        cout << "ERROR: groupMembers size is not "<< groupSize << " it is: "<< vectorSize << ". Received from: " << msg.touristId << " with type " << msg.type << endl;
    }
    msg.groupMembers.resize(vectorSize);
    MPI_Unpack(buffer, size, &position, msg.groupMembers.data(), vectorSize, MPI_INT, MPI_COMM_WORLD);

    // CZYSZCZENIE
    delete[] buffer;
}




