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
    bool inTripQueue;
    bool inGuideQueue;
    bool isBeaten;
    bool sendRequest;
    bool isLeader;
    bool inTrip;
    int acks;
    int guideAcks;
    int tripCounter;
    int receivedReleaseGroup;
    int hospitalCounter;
    int leaderID;
    bool waitingForReleaseGroup;
    int removedCounter;


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
                // sendMessage(to, type, timestamp, pid, groupMembersToVector());
                sendPackedMessage(to, type, timestamp, pid, groupMembersToVector());
            }
        }
    }

    void broadcastMessage(int type, vector<int> groupMembers) {
        timestamp++;

        for (auto& to : groupMembers) {
            if (to != pid) {
                // sendMessage(to, type, timestamp, pid, groupMembersToVector());
                sendPackedMessage(to, type, timestamp, pid, groupMembers);
            }
        }
    }

    void broadcastMessageInGroup(int type) {
        timestamp++;
        priority_queue<Message> tempQueue = groupMembers;
        while (!tempQueue.empty()) {
            Message msg = tempQueue.top();
            if (msg.touristId != pid) {
                sendPackedMessage(msg.touristId, type, timestamp, pid, groupMembersToVector());
            }
            tempQueue.pop();
        }
    }

    void reinitializeGroupMember() {
        groupMembers = priority_queue<Message>();
        for (int i = 0; i < groupSize; i++) {
            groupMembers.push(Message({0, 0, 0, vector<int>({0,0,0,0,0,0,0})}));
        }
    }

    vector<int> groupMembersToVector() {
        priority_queue<Message> tempQueue = groupMembers;
        vector<int> groupMembersVector;
        while (!tempQueue.empty()) {
            Message msg = tempQueue.top();
            groupMembersVector.push_back(msg.touristId);
            tempQueue.pop();
        }
        return groupMembersVector;
    }

    void joinTripQueue(int numberOfTourists) {
        broadcastMessage(REQ_TRIP);
        tripQueue.push({timestamp, pid, REQ_TRIP, groupMembersToVector()});
        sendRequest = true;
        receivedReleaseGroup = 0;
        removedCounter = 0;
        tripCounter = 0;
    }  

    void joinGuideQueue(int numberOfTourists) {
        broadcastMessage(REQ_GUIDE);
        guideQueue.push({timestamp, pid, REQ_GUIDE, groupMembersToVector()});

        sendRequest = true;
    }

    void removeTouristsFromQueue(vector<int> tourists) {
        auto it = find(tourists.begin(), tourists.end(), pid);
        if (it != tourists.end()) {
            inTripQueue = false;
            inTrip = false;
        }
        for (int i = 0; i < tourists.size(); i++) {
            tripQueue = removeTouristFromQueue(tripQueue, tourists[i]);
            // guideQueue = removeTouristFromQueue(guideQueue, tourists[i]); TUTAJ SIE ZASTANOWIC
        }
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

    

    void releaseGroup() {
        if (isLeader) {
            waitingForReleaseGroup = true;
            sendPackedMessage(pid, RELEASE_GROUP, timestamp, pid, groupMembersToVector());
        } else {
            inTrip = false;
            waitingForReleaseGroup = true;
            sendPackedMessage(leaderID, RELEASE_GROUP, timestamp, pid, groupMembersToVector());
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
        priority_queue<Message> tempQueue = tripQueue;
        priority_queue<Message> newGroupMembers;
        if (isLeader) {
            int index = findTouristIndexInTripQueue();
            while (!tempQueue.empty()) {
                Message msg = tempQueue.top();
                if ((index - findTouristIndexInTripQueue(msg.touristId)) < groupSize && (index - findTouristIndexInTripQueue(msg.touristId)) >= 0) {
                    newGroupMembers.push(msg);
                }
                tempQueue.pop();
            
            }
        }
        groupMembers = newGroupMembers;
    }

    void formGroup(vector<int> groupMembers) {
        priority_queue<Message> tempQueue;
        for (int i = 0; i < groupMembers.size(); i++) {
            tempQueue.push({0, groupMembers[i], 0, groupMembers});
        }
        this->groupMembers = tempQueue;
    }
    
    void startTrip() {
        timestamp++;
        inTrip = true;
        priority_queue<Message> tempQueue = groupMembers;
        
        while (!tempQueue.empty()) {
            Message msg = tempQueue.top();
            if (msg.touristId != pid) {
                sendPackedMessage(msg.touristId, START_TRIP, timestamp, pid, groupMembersToVector());
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

