#pragma once
#include <vector>
#include <queue>
#include <map>
#include <random>

#include "communication.h"
#include "constants.h"


using namespace std;

struct Tourist {
    int pid;
    int timestamp;
    int group;
    bool inTripQueue;
    bool inGuideQueue;
    bool inHospital;
    bool isBeaten;
    bool sendRequest;
    bool isLeader;
    bool inTrip;
    int acks;
    int acksGuide;
    int tripCounter;
    int startTripAcks;
    int receivedReleaseGroup;
    int hospitalCounter;


    std::priority_queue<Message> groupMembers;

    // Mapa do przechowywania aktualnych znacznikow czasowych innych procesow
    std::map<int, int> currentTimestamps; 

    // Aktualna kolejka do wycieczki
    std::priority_queue<Message> tripQueue;

    // Aktualna kolejka do przewodnikae
    std::priority_queue<Message> guideQueue;

    bool operator<(const Tourist& other) const {
        if (timestamp == other.timestamp) {
            return pid > other.pid;
        }
        return timestamp > other.timestamp;
    }

    void broadcastMessage(int type) {
        timestamp++;

        for (int to = 0; to < MAX_TOURISTS; ++to) {
            if (to != pid) {
                sendMessage(to, type, timestamp, pid);
            }
        }
    }

    void broadcastMessageInGroup(int type) {
        timestamp++;
        priority_queue<Message> tempQueue = groupMembers;
        while (!tempQueue.empty()) {
            Message msg = tempQueue.top();
            if (msg.touristId != pid) {
                sendMessage(msg.touristId, type, timestamp, pid);
            }
            tempQueue.pop();
        }
    }

    void joinTripQueue(int numberOfTourists) {
        broadcastMessage(REQ_TRIP);
        tripQueue.push({timestamp, pid, REQ_TRIP});
        sendRequest = true;
        receivedReleaseGroup = 1;
    }  

    void joinGuideQueue(int numberOfTourists) {
        broadcastMessage(REQ_GUIDE);
        guideQueue.push({timestamp, pid, REQ_GUIDE});

        sendRequest = true;
    }

    priority_queue<Message> removeTouristFromQueue(priority_queue<Message> queue, int touristID) {
        priority_queue<Message> tempQueue = queue;
        priority_queue<Message> newQueue = priority_queue<Message>();
        while (!tempQueue.empty()) {
            Message msg = tempQueue.top();
            if (msg.touristId != touristID) {
                newQueue.push(msg);
            }
            tempQueue.pop();
        }

        
        return newQueue;
    }

    void updateGroupNumber() {
        group = findTouristIndexInTripQueue() / groupSize;
    }

    void releaseGroup() {
        broadcastMessage(RELEASE_TRIP);
        broadcastMessageInGroup(RELEASE_GROUP);

        groupMembers = priority_queue<Message>();
        tripQueue = removeTouristFromQueue(tripQueue, pid);
        group = -1;
        
        tripCounter = 0;


        if (isLeader) {
            broadcastMessage(RELEASE_GUIDE);
            isLeader = false;
            guideQueue = removeTouristFromQueue(guideQueue, pid);
            inGuideQueue = false;
        }
    }

    int findTouristIndexInTripQueue() {
        std::priority_queue<Message> tempQueue = tripQueue; 
        int index = 0;
        while (!tempQueue.empty()) {
            Message msg = tempQueue.top();
            if (msg.touristId == pid) {
                return index;
            }
            tempQueue.pop();
            index++;
        }
        return -1; 
    } 

    int findTouristIndexInTripQueue(int id) {
        std::priority_queue<Message> tempQueue = tripQueue; 
        int index = 0;
        while (!tempQueue.empty()) {
            Message msg = tempQueue.top();
            if (msg.touristId == id) {
                return index;
            }
            tempQueue.pop();
            index++;
        }
        return -1; 
    }

    int findTouristIndexInGuideQueue() {
        std::priority_queue<Message> tempQueue = guideQueue;
        int index = 0;
        while (!tempQueue.empty()) {
            Message msg = tempQueue.top();
            if (msg.touristId == pid) {
                return index;
            }
            tempQueue.pop();
            index++;
        }
        return -1; 
    }

    void formGroup() {
        int groupNumber = findTouristIndexInTripQueue() / groupSize;
        std::priority_queue<Message> tempQueue = tripQueue; 
        while (!tempQueue.empty()) {
            Message msg = tempQueue.top();
            if ((findTouristIndexInTripQueue(msg.touristId) / groupSize) == groupNumber) {
                groupMembers.push(msg);
            }
            tempQueue.pop();
        }
        if (groupMembers.size() != groupSize) {
            while (!groupMembers.empty()) {
                groupMembers.pop();
            }
        } else {
            group = groupNumber;
        }
    }
    
    void startTrip() {
        timestamp++;
        inTrip = true;
        priority_queue<Message> tempQueue = groupMembers;
        
        while (!tempQueue.empty()) {
            Message msg = tempQueue.top();
            if (msg.touristId != pid) {
                sendMessage(msg.touristId, START_TRIP, timestamp, pid);
            }
            tempQueue.pop();
        }
        
    }

    bool isTouristBeaten() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);
        isBeaten = dis(gen) < probabilityOfBeaten;
        return isBeaten;
    }
    
};

