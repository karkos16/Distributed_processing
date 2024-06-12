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
const int NACK_START_TRIP = 6;
const int ACK_START_TRIP = 7;
const int ACK_GUIDE = 8;
const int START_TRIP = 9;
const int RELEASE_GROUP = 10;
const int REQ_START_TRIP = 11;


// PARAMS
const int numberOfGuides = 2;
const int groupSize = 7;
const int MAX_TOURISTS = 20;
const int tripTime = 5;
const double probabilityOfBeaten = 0.0;
const int hospitalizationTime = 2;