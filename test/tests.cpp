// Copyright 2021 GHA Test Team

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <chrono>
#include <stdexcept>
#include "TimedDoor.h"

using ::testing::_;
using ::testing::Return;

// ========== МОК-КЛАССЫ ==========

class MockTimerClient : public TimerClient {
 public:
    MOCK_METHOD(void, Timeout, (), (override));
};

class MockDoor : public Door {
 public:
    MOCK_METHOD(void, lock, (), (override));
    MOCK_METHOD(void, unlock, (), (override));
    MOCK_METHOD(bool, isDoorOpened, (), (override));
};

// ========== ВСПОМОГАТЕЛЬНЫЕ КЛАССЫ ==========

class TimerHelper {
 public:
    void triggerTimeout(TimerClient* client) {
        client->Timeout();
    }
};

class DoorHelper {
 public:
    void closeDoor(Door* door) { door->lock(); }
    void openDoor(Door* door) { door->unlock(); }
    bool getDoorState(Door* door) { return door->isDoorOpened(); }
};

// ========== ТЕСТОВЫЙ ФИКСТУР ==========

class TimedDoorTest : public ::testing::Test {
 protected:
    TimedDoor* door;
    DoorTimerAdapter* adapter;

    void SetUp() override {
        door = new TimedDoor(2);
        adapter = new DoorTimerAdapter(*door);
    }

    void TearDown() override {
        delete adapter;
        delete door;
    }
};

// ========== ТЕСТЫ ДЛЯ TimedDoor ==========

TEST_F(TimedDoorTest, InitiallyClosed) {
    EXPECT_FALSE(door->isDoorOpened());
}

TEST(TimedDoorStandaloneTest, ConstructorSetsTimeout) {
    TimedDoor testDoor(5);
    EXPECT_EQ(testDoor.getTimeOut(), 5);
}

TEST_F(TimedDoorTest, LockClosesDoor) {
    door->unlock();
    door->lock();
    EXPECT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorTest, UnlockWithZeroTimeoutThrows) {
    TimedDoor zeroDoor(0);
    EXPECT_THROW(zeroDoor.unlock(), std::runtime_error);
}

TEST_F(TimedDoorTest, ThrowStateOnClosedDoorNoThrow) {
    door->lock();
    EXPECT_NO_THROW(door->throwState());
}

TEST_F(TimedDoorTest, ThrowStateOnOpenDoorThrows) {
    EXPECT_THROW(door->unlock(), std::runtime_error);
    EXPECT_THROW(door->throwState(), std::runtime_error);
}

TEST_F(TimedDoorTest, DoorStaysOpenAfterUnlockThrows) {
    EXPECT_THROW(door->unlock(), std::runtime_error);
    EXPECT_TRUE(door->isDoorOpened());
}

// ========== ТЕСТЫ ДЛЯ АДАПТЕРА ==========

TEST_F(TimedDoorTest, AdapterTimeoutOnClosedDoorSafe) {
    door->lock();
    EXPECT_NO_THROW(adapter->Timeout());
}

TEST_F(TimedDoorTest, AdapterTimeoutOnOpenDoorThrows) {
    EXPECT_THROW(door->unlock(), std::runtime_error);
    EXPECT_THROW(adapter->Timeout(), std::runtime_error);
}

// ========== ТЕСТЫ С МОКАМИ (Timer) ==========

TEST(TimerMockTest, RegisterCallsTimeout) {
    Timer timer;
    MockTimerClient mockClient;

    EXPECT_CALL(mockClient, Timeout()).Times(1);
    timer.tregister(0, &mockClient);
}

TEST(TimerClientMockTest, TimeoutViaHelper) {
    MockTimerClient mockClient;
    TimerHelper helper;

    EXPECT_CALL(mockClient, Timeout()).Times(1);
    helper.triggerTimeout(&mockClient);
}

// ========== ТЕСТЫ С МОКАМИ (Door) ==========

TEST(DoorMockTest, LockViaHelper) {
    MockDoor mockDoor;
    DoorHelper helper;

    EXPECT_CALL(mockDoor, lock()).Times(1);
    helper.closeDoor(&mockDoor);
}

TEST(DoorMockTest, UnlockViaHelper) {
    MockDoor mockDoor;
    DoorHelper helper;

    EXPECT_CALL(mockDoor, unlock()).Times(1);
    helper.openDoor(&mockDoor);
}

TEST(DoorMockTest, StateCheckViaHelper) {
    MockDoor mockDoor;
    DoorHelper helper;

    EXPECT_CALL(mockDoor, isDoorOpened())
        .Times(1)
        .WillOnce(Return(true));

    EXPECT_TRUE(helper.getDoorState(&mockDoor));
}

// ========== ИНТЕГРАЦИОННЫЙ ТЕСТ ==========

TEST(IntegrationTest, CloseBeforeTimeoutPreventsException) {
    TimedDoor testDoor(1);

    std::thread closer([&testDoor]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        testDoor.lock();
    });

    EXPECT_NO_THROW(testDoor.unlock());
    closer.join();
    EXPECT_FALSE(testDoor.isDoorOpened());
}
