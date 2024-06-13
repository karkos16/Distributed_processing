#pragma once
#include <vector>
#include <queue>
#include <map>
#include <random>
#include <limits.h>

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
    int guideAcks;
    int tripCounter;
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

    bool isThereLowerTimestamp(int timestamp) {
        for (auto const& x : currentTimestamps) {
            if (x.second < timestamp) {
                return true;
            } else if (x.second == timestamp && x.first < pid) {
                return true;
            }
        }
        return false;
    }

    int requestTripTimestamp() {
        priority_queue<Message> tempQueue = tripQueue;
        while (!tempQueue.empty()) {
            Message msg = tempQueue.top();
            if (msg.touristId == pid) {
                return msg.timestamp;
            }
            tempQueue.pop();
        }
        return INT_MAX;
    }

    int requestGuideTimestamp() {
        priority_queue<Message> tempQueue = guideQueue;
        while (!tempQueue.empty()) {
            Message msg = tempQueue.top();
            if (msg.touristId == pid) {
                return msg.timestamp;
            }
            tempQueue.pop();
        }
        return INT_MAX;
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
        inTrip = false;
        broadcastMessage(RELEASE_TRIP);
        broadcastMessageInGroup(RELEASE_GROUP);
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
        priority_queue<Message> tempQueue = tripQueue;
        if (isLeader) {
            int index = findTouristIndexInTripQueue();
            while (!tempQueue.empty()) {
                Message msg = tempQueue.top();
                if ((index - findTouristIndexInTripQueue(msg.touristId)) < groupSize && (index - findTouristIndexInTripQueue(msg.touristId)) >= 0) {
                    groupMembers.push(msg);
                }
                tempQueue.pop();
            
            }
        }
    }

    void formGroup(int leaderID) {
        int leaderIndex = findTouristIndexInTripQueue(leaderID);
        priority_queue<Message> tempQueue = tripQueue;
        while (!tempQueue.empty()) {
                Message msg = tempQueue.top();
                if ((leaderIndex - findTouristIndexInTripQueue(msg.touristId)) < groupSize && (leaderIndex - findTouristIndexInTripQueue(msg.touristId)) >= 0) {
                    groupMembers.push(msg);
                }
                tempQueue.pop();
            
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

