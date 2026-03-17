// Copyright 2021 GHA Test Team
#include "TimedDoor.h"
#include <chrono>
#include <thread>
#include <stdexcept>

DoorTimerAdapter::DoorTimerAdapter(TimedDoor& d) : door(d) {}

void DoorTimerAdapter::Timeout() {
    door.throwState();
}

TimedDoor::TimedDoor(int timeout) 
    : adapter(new DoorTimerAdapter(*this))
    , iTimeout(timeout)
    , isOpened(false) {}

TimedDoor::~TimedDoor() {
    delete adapter;
}

bool TimedDoor::isDoorOpened() {
    return isOpened;
}

void TimedDoor::unlock() {
    isOpened = true;
    if (iTimeout == 0) {
        throw std::runtime_error("Zero timeout door cannot be unlocked");
    }
    Timer timer;
    timer.tregister(iTimeout, adapter);
}

void TimedDoor::lock() {
    isOpened = false;
}

int TimedDoor::getTimeOut() const {
    return iTimeout;
}

void TimedDoor::throwState() {
    if (isOpened) {
        throw std::runtime_error("Door is still opened after timeout");
    }
}

void Timer::sleep(int seconds) {
    if (seconds > 0) {
        std::this_thread::sleep_for(std::chrono::seconds(seconds));
    }
}

void Timer::tregister(int timeout, TimerClient* newClient) {
    client = newClient;
    sleep(timeout);
    if (client != nullptr) {
        client->Timeout();
    }
}
