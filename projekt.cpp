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

void printVector(vector<int> vec, int id) {
    cout << id << ": ";
    for (int i = 0; i < vec.size(); i++) {
        cout << vec[i] << " ";
    }
    cout << endl;
}

void imitateTourist(Tourist &tourist) {

    while (true)
    {
        if (!tourist.inTripQueue && !tourist.sendRequest && !tourist.waitingForReleaseGroup) {
            tourist.reinitializeGroupMember();
            tourist.joinTripQueue(touristsNumber);

        } else{
            // Odebranie wiadomosci i ustawienie aktualnych timestampow

            int flag;
            MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
            Message msg = {0, 0, 0, vector<int>({0,0,0,0,0,0,0})};
            if (flag) {
                receivePackedMessage(msg);
                
                tourist.timestamp = incrementTimestamp(tourist.timestamp, msg.timestamp);
                tourist.currentTimestamps[msg.touristId] = msg.timestamp;
                tourist.currentTimestamps[tourist.pid] = tourist.timestamp;
            }

            // Obsługa wiadomosci
            switch (msg.type) {
                case REQ_TRIP:
                    // PROCES DOSTAL WIADOMOSC ZE INNY PROCES UBIEGA SIE O MIESJCE W GRUPIE
                    tourist.timestamp++;
                    sendPackedMessage(msg.touristId, ACK_TRIP, tourist.timestamp, tourist.pid, tourist.groupMembersToVector());
                    tourist.tripQueue.push(msg);
                    break;
                case REQ_GUIDE:
                    // PROCES DOSTAŁ WIADOMOSC ZE INNY PROCES UBIEGA SIE O PRZEWODNIKA
                    tourist.timestamp++;
                    sendPackedMessage(msg.touristId, ACK_GUIDE, tourist.timestamp, tourist.pid, tourist.groupMembersToVector());
                    tourist.guideQueue.push(msg);

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
                        if ((position + 1)% groupSize == 0 && position != -1 && !tourist.isThereLowerTimestamp(tourist.requestTripTimestamp())) {
                            // cout << tourist.pid <<": I'm the leader of the group " << tourist.group << endl;
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

                        int position = tourist.findTouristIndexInGuideQueue();
                    
                        if (position < numberOfGuides && position != -1 && !tourist.isThereLowerTimestamp(tourist.requestGuideTimestamp())) {
                            tourist.formGroup();
                           

                            tourist.startTrip();
                            cout << "Tourist " << tourist.pid << " started the trip with guide: " << position << endl;
                            // TODO: WYSYŁA WIADOMOŚĆ DO SWOJEJ GRUPY
                        }

                    }
                    break;
                case START_TRIP:
                    // PROCES DOSTAJE WIADOMOSC ZE WYCIECZKA SIE ROZPOCZELA
                    tourist.formGroup(msg.groupMembers);
                    tourist.inTrip = true;
                    tourist.leaderID = msg.touristId;
  
                    break;
                case RELEASE_GROUP:
                    // PROCES DOSTAJE WIADOMOSC ZE INNY PROCES Z JEGO GRUPY KONCZY WYCIECZKE
                    tourist.receivedReleaseGroup++;
 
                    if (tourist.isLeader && tourist.receivedReleaseGroup == groupSize) {
                        // cout << "Tourist " << tourist.pid << " released the group" << endl;
                        tourist.broadcastMessage(RELEASE_GUIDE);
                        tourist.broadcastMessage(RELEASE_TRIP);
                        tourist.isLeader = false;
                        tourist.guideQueue = tourist.removeTouristFromQueue(tourist.guideQueue, tourist.pid);
                        tourist.inGuideQueue = false;
                        tourist.inTrip = false;
                        tourist.removeTouristsFromQueue(tourist.groupMembersToVector());
                        
                    } 
                    break;
                case RELEASE_TRIP:
                    // PROCES DOSTAJE WIADOMOSC ZE INNY PROCES KONCZY WYCIECZKE
                    tourist.removeTouristsFromQueue(msg.groupMembers);

                    
                    tourist.broadcastMessage(REMOVED_FROM_GROUP, msg.groupMembers);
                    if(find(msg.groupMembers.begin(), msg.groupMembers.end(), tourist.pid) != msg.groupMembers.end()) {
                        sendPackedMessage(tourist.pid, REMOVED_FROM_GROUP, tourist.timestamp, tourist.pid, tourist.groupMembersToVector()); // bo BROADCAST NIE WYSYLA DO SIEBIE
                    }                    
                    break;
                case REMOVED_FROM_GROUP:
                    // PROCES DOSTAJE WIADOMOSC ZE INNY PROCES GO USUNAL ZE SWOEJ KOLEJKI
                    tourist.removedCounter++;
                    // cout << "Tourist " << tourist.pid << " received removed from group from " << msg.touristId << " " << tourist.removedCounter << endl;
                    if (tourist.removedCounter == touristsNumber - 1) {
                        tourist.removedCounter = 0;
                        tourist.inTrip = false;
                        tourist.inTripQueue = false;
                        tourist.isLeader = false;
                        tourist.receivedReleaseGroup = groupSize;
                        tourist.waitingForReleaseGroup = false;
                    }
                case RELEASE_GUIDE:
                    // PROCES DOSTAJE WIADOMOSC ZE INNY PROCES ZWALNIA PRZEWODNIKA
                    tourist.guideQueue = tourist.removeTouristFromQueue(tourist.guideQueue, msg.touristId);
                    if (tourist.isLeader && !tourist.inTrip) {
                        int position = tourist.findTouristIndexInGuideQueue();
                        if (position < numberOfGuides && position != -1 && !tourist.isThereLowerTimestamp(tourist.requestGuideTimestamp())) {
                            // cout << tourist.pid << " reserved the guide. Starting trip" << endl;
                            // PRZEWODNICY SA DOSTEPNI WIEC MOZNA WYSTARTOWAC WYCIECZKE
                            tourist.formGroup();
                            tourist.startTrip();
                        }
                    }
                    break;
                default:
                    break;
            }

            if (tourist.inTrip && !tourist.isBeaten && !tourist.waitingForReleaseGroup) {
                tourist.tripCounter++;
                tourist.timestamp++;

                if (tourist.tripCounter == tripTime) {
                    if (tourist.isLeader) {
                        printQueue(tourist.tripQueue, tourist.pid);
                    //     printQueue(tourist.groupMembers, tourist.pid);

                    }
                    // printQueue(tourist.groupMembers, tourist.pid);
                    // WYSYLAJA DO LIDERA ZE SKONCZYLI WYCIECZKE
                    tourist.releaseGroup();
                } else {
                    if (tourist.isTouristBeaten()) {
                        cout << "Tourist " << tourist.pid << " is beaten" << endl;
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
                
                if(tourist.tripCounter == tripTime && !tourist.waitingForReleaseGroup) {
                    if (tourist.isLeader) {
                        printQueue(tourist.tripQueue, tourist.pid);
                        // printQueue(tourist.groupMembers, tourist.pid);
                    }
                    // printQueue(tourist.groupMembers, tourist.pid);
                    tourist.releaseGroup();
                }
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
            cout << "Size: " << groupSize<< " Number of guides: " << numberOfGuides << " Max tourists: " << MAX_TOURISTS << endl;
        }
    } else {
        Tourist tourist;
        tourist.pid = rank;
        tourist.timestamp = 0;
        tourist.inTripQueue = false;
        tourist.inGuideQueue = false;
        tourist.isBeaten = false;
        tourist.sendRequest = false;
        tourist.acks = 1; // 1 żeby nie wysyłał REQ do siebie
        tourist.isLeader = false;
        tourist.inTrip = false;
        tourist.tripCounter = 0;
        tourist.receivedReleaseGroup = 0; // groupSize zeby poczatkowy warunek byl spelniony
        tourist.hospitalCounter = 0;
        tourist.guideAcks = 1;
        tourist.waitingForReleaseGroup = false;
        tourist.removedCounter = 0;
        for (int i = 0; i < touristsNumber; i++) {
            tourist.currentTimestamps[i] = 0;
        }
        for (int i = 0; i < groupSize; i++) {
            tourist.groupMembers.push(Message({0, 0, 0, vector<int>({0,0,0,0,0,0,0})}));
        }
        imitateTourist(tourist);
    }
    

    MPI_Finalize();
}
