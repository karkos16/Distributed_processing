#include <mpi.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <queue>

#include <unistd.h>

#include "tourist.h"
#include "communication.h"
#include "constants.h"

using namespace std;

int touristsNumber;


void printQueue(std::priority_queue<Message> queue, int id) {
    cout << id << " Queue: ";
    while (!queue.empty()) {
        Message msg = queue.top();
        cout << msg.touristId << " ";
        queue.pop();
    }
    cout << endl;
}

void imitateTourist(Tourist &tourist) {

    while (true)
    {
        if (!tourist.inTripQueue && !tourist.sendRequest && tourist.receivedReleaseGroup == groupSize) {
            tourist.joinTripQueue(touristsNumber);
        } else{
            // Odebranie wiadomosci i ustawienie aktualnych timestampow

            int flag;
            MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
            Message msg = {0, 0, 0};
            if (flag) {
                receiveMessage(msg);
                tourist.timestamp = incrementTimestamp(tourist.timestamp, msg.timestamp);
                tourist.currentTimestamps[msg.touristId] = msg.timestamp;
                tourist.currentTimestamps[tourist.pid] = tourist.timestamp;
            }
            
            // Obsługa wiadomosci
            switch (msg.type) {
                case REQ_TRIP:
                    // PROCES DOSTAL WIADOMOSC ZE INNY PROCES UBIEGA SIE O MIESJCE W GRUPIE
                    tourist.timestamp++;
                    sendMessage(msg.touristId, ACK_TRIP, tourist.timestamp, tourist.pid);
                    tourist.tripQueue.push(msg);
                    break;
                case REQ_GUIDE:
                    // PROCES DOSTAŁ WIADOMOSC ZE INNY PROCES UBIEGA SIE O PRZEWODNIKA
                    tourist.timestamp++;
                    sendMessage(msg.touristId, ACK_GUIDE, tourist.timestamp, tourist.pid);
                    tourist.guideQueue.push(msg);
                    // // JESLI PROCES JESZCZE NIE MA GRUPY TO POWINIEN JA "UFORMOWAC", 
                    // // BO O PRZEWODNIKA MOZE UBIEGAC SIE PROCES Z JEGO GRUPY
                    // if (tourist.group == -1 && !tourist.isLeader && !tourist.inTrip) {
                    //     // cout << "Forming group " << tourist.pid << endl;
                    //     tourist.formGroup();
                    // }
                    // cout << msg.touristId << " rezerwuje przewodnika" << endl;
                    // cout << "Guide queue: ";
                    // printQueue(tourist.guideQueue, tourist.pid);
                    // cout << "Trip queue: ";
                    // printQueue(tourist.tripQueue, tourist.pid);
                    break;
                case ACK_TRIP:
                    // PROCES DOSTAJE POTWIERDZNIE ZE MOZE UBIEGAC SIE O MIESJCE W GRUPIE
                    if (tourist.acks < touristsNumber - 1) {
                        tourist.acks++;
                    } else {
                        tourist.inTripQueue = true;
                        tourist.sendRequest = false;
                        tourist.acks = 1;

                        int position = tourist.findTouristIndexInTripQueue(tourist.pid);
                        // cout << "Tourist " << tourist.pid << " is " << position << " in queue" << endl;
                        if ((position + 1)% groupSize == 0 && position != -1 && !tourist.isThereLowerTimestamp(tourist.requestTripTimestamp())) {
                            // cout << tourist.pid <<": I'm the leader of the group " << tourist.group << ": ";
                            tourist.isLeader = true;
                            tourist.joinGuideQueue(touristsNumber);
                        }
                    }
                    break;
                case ACK_GUIDE:
                    // PROCES DOSTAJE POTWIERDZENIE ZE MOZE UBIEGAC SIE O PRZEWODNIKA
                    if (tourist.guideAcks < touristsNumber - 1) {
                        tourist.guideAcks++;
                        
                    } else {
                        tourist.inGuideQueue = true;
                        tourist.sendRequest = false;
                        tourist.guideAcks = 1;
                        // printQueue(tourist.guideQueue);

                        int position = tourist.findTouristIndexInGuideQueue();
                        // MOGL STWORZYC GRUPE JESLI PRZED WSZYSTKIMI ACK JAKIS INNY PROCES UBIEGAL SIE O PRZEWODNIKA
                        // if (tourist.group == -1) {
                        //     tourist.formGroup();
                        // }

                    
                        // printQueue(tourist.groupMembers);
                        if (position < numberOfGuides && position != -1 && !tourist.isThereLowerTimestamp(tourist.requestGuideTimestamp())) {
                            tourist.formGroup();
                            // cout << tourist.pid << " reserved the guide. Starting trip " << position << endl;
                            // cout << "Guide queue: ";
                            // printQueue(tourist.guideQueue, tourist.pid);
                            // cout << "Trip queue: ";
                            // printQueue(tourist.tripQueue, tourist.pid);
                            // PRZEWODNICY SA DOSTEPNI WIEC MOZNA WYSTARTOWAC WYCIECZKE
                            tourist.startTrip();
                        }

                    }
                    break;
                case START_TRIP:
                    // PROCES DOSTAJE WIADOMOSC ZE WYCIECZKA SIE ROZPOCZELA
                    tourist.formGroup(msg.touristId);
                    tourist.inTrip = true;
                    // cout << "Tourist " << tourist.pid << " started the trip" << endl;
                    break;
                case RELEASE_GROUP:
                    // PROCES DOSTAJE WIADOMOSC ZE INNY PROCES Z JEGO GRUPY KONCZY WYCIECZKE
                    tourist.receivedReleaseGroup++;
                    
                    if (tourist.isLeader && tourist.receivedReleaseGroup == groupSize) {
                       tourist.broadcastMessage(RELEASE_GUIDE);
                       tourist.isLeader = false;
                       tourist.guideQueue = tourist.removeTouristFromQueue(tourist.guideQueue, tourist.pid);
                       tourist.inGuideQueue = false;
                       tourist.groupMembers = priority_queue<Message>();
                        tourist.tripQueue = tourist.removeTouristFromQueue(tourist.tripQueue, tourist.pid);
                    } else if (tourist.receivedReleaseGroup == groupSize) {
                        tourist.group = -1;
                        tourist.inTrip = false;
                        tourist.inTripQueue = false;
                        tourist.tripCounter = 0;
                        tourist.groupMembers = priority_queue<Message>();
                        tourist.tripQueue = tourist.removeTouristFromQueue(tourist.tripQueue, tourist.pid);
                    }     
                    break;
                case RELEASE_TRIP:
                    // PROCES DOSTAJE WIADOMOSC ZE INNY PROCES KONCZY WYCIECZKE
                    tourist.tripQueue = tourist.removeTouristFromQueue(tourist.tripQueue, msg.touristId);
                    // NUMER GRUPY MOGL ULEC ZMIANIE BO KOLEJKA IS PRZESUWA
                    // if (tourist.group != -1) {
                    //     tourist.updateGroupNumber();
                    // }
                    break;
                case RELEASE_GUIDE:
                    // PROCES DOSTAJE WIADOMOSC ZE INNY PROCES ZWALNIA PRZEWODNIKA
                    tourist.guideQueue = tourist.removeTouristFromQueue(tourist.guideQueue, msg.touristId);
                    if (tourist.isLeader) {
                        int position = tourist.findTouristIndexInGuideQueue();
                        if (position < numberOfGuides && position != -1 && !tourist.isThereLowerTimestamp(tourist.requestGuideTimestamp())) {
                            // cout << tourist.pid << " reserved the guide. Starting trip" << endl;
                            // printQueue(tourist.guideQueue, tourist.pid);
                            // printQueue(tourist.tripQueue, tourist.pid);
                            // PRZEWODNICY SA DOSTEPNI WIEC MOZNA WYSTARTOWAC WYCIECZKE
                            tourist.formGroup();
                            tourist.startTrip();
                        }
                    }
                    break;
                default:
                    break;
            }

            if (tourist.inTrip && !tourist.isBeaten) {
                tourist.tripCounter++;
                tourist.timestamp++;
                // if (tourist.isLeader) {
                //     cout << "Tourist " << tourist.pid << " is the leader of the group " << tourist.group << " and is at step: " << tourist.tripCounter << endl;
                // }
                // cout << "Tourist " << tourist.pid << " is on the trip at " << tourist.tripCounter << endl;
                if (tourist.tripCounter == tripTime) {
                    if (tourist.isLeader) {
                        printQueue(tourist.tripQueue, tourist.pid);
                        // printQueue(tourist.groupMembers, tourist.pid);
                        // printQueue(tourist.guideQueue, tourist.pid);
                    }
                    // printQueue(tourist.groupMembers, tourist.pid);
                    tourist.releaseGroup();
                } else {
                    if (tourist.isTouristBeaten()) {
                        // cout << "Tourist " << tourist.pid << " is beaten" << endl;
                    }
                }
                // sleep(1);
            } else if (tourist.isBeaten)  {
                if (tourist.inTrip) {
                    tourist.tripCounter++;
                }
                tourist.hospitalCounter++;
                
                if(tourist.hospitalCounter == hospitalizationTime) {
                    tourist.isBeaten = false;
                    tourist.hospitalCounter = 0;
                }
                
                if(tourist.tripCounter == tripTime) {
                    if (tourist.isLeader) {
                        printQueue(tourist.tripQueue, tourist.pid);
                        // printQueue(tourist.groupMembers, tourist.pid);
                        // printQueue(tourist.guideQueue, tourist.pid);
                    }
                    // printQueue(tourist.groupMembers, tourist.pid);
                    tourist.releaseGroup();
                }
                // sleep(1);
            }
        
        }
    }
}

int main(int argc, char **argv) {

    if (numberOfGuides < 1 || groupSize < 1) {
        cout << "Invalid input" << endl;
        return 1;
    }

    MPI_Init(&argc, &argv);

    
    int rank;
    
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &touristsNumber);

    if (touristsNumber <= numberOfGuides || MAX_TOURISTS < touristsNumber) {
        if (rank == 0) {
            cout << "Invalid number of processes" << endl;
            cout << "Size: " << touristsNumber << " Number of guides: " << touristsNumber << " Max tourists: " << MAX_TOURISTS << endl;
        }
    } else {
        Tourist tourist;
        tourist.pid = rank;
        tourist.timestamp = 0;
        tourist.group = -1;
        tourist.inTripQueue = false;
        tourist.inGuideQueue = false;
        tourist.inHospital = false;
        tourist.isBeaten = false;
        tourist.sendRequest = false;
        tourist.acks = 1; // 1 żeby nie wysyłał REQ do siebie
        tourist.isLeader = false;
        tourist.inTrip = false;
        tourist.tripCounter = 0;
        tourist.receivedReleaseGroup = groupSize; // groupSize zeby poczatkowy warunek byl spelniony
        tourist.hospitalCounter = 0;
        tourist.guideAcks = 1;
        for (int i = 0; i < touristsNumber; i++) {
            tourist.currentTimestamps[i] = 0;
        }
        imitateTourist(tourist);
    }
    

    MPI_Finalize();
}
