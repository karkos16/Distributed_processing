#pragma once
#include <iostream>
#include "tourist.h"
#include "communication.h"

using namespace std;

// MESSAGE TYPES
const int REQ_TRIP = 1;
const int REQ_GUIDE = 2;
const int ACK_TRIP = 3;
const int RELEASE_TRIP = 4;
const int RELEASE_GUIDE = 5;
// const int BEATEN = 6;
// const int LEAVE_HOSPITAL = 7;
const int ACK_GUIDE = 8;
const int START_TRIP = 9;
const int RELEASE_GROUP = 10;


// PARAMS
const int numberOfGuides = 2;
const int groupSize = 7;
const int MAX_TOURISTS = 20;
const int tripTime = 5;
const double probabilityOfBeaten = 0.9;
const int hospitalizationTime = 2;