// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef air_Waiter_hpp
#define air_Waiter_hpp

#include <chrono>
#include <iostream>
#include <atomic>
#include "common/Common.hpp"
#include "common/common_utils/Utils.hpp"
#include "common/ClockFactory.hpp"

namespace msr { namespace airlib {

class CancelableBase {
public:
    CancelableBase(std::atomic_bool* token) {
        cancelled_ = token;
    }
    // it is handy if this object is copyable so we can pass it safely to async lambdas with [=] copy semantics.
    // async lamdas with move semantics [&] are a headache...
    CancelableBase(CancelableBase& other) {
        cancelled_ = other.cancelled_;
    }
    void operator=(CancelableBase& other) {
        cancelled_ = other.cancelled_;
    }
    virtual void complete() {
        // long running tasks must call this to notify when they are done.
    }
    bool isCancelled() const {
        return *cancelled_;
    }
    bool sleep(double secs) const
    {
        //We can pass duration directly to sleep_for however it is known that on 
        //some systems, sleep_for makes system call anyway even if passed duration 
        //is <= 0. This can cause 50us of delay due to context switch.
        if (isCancelled()) {
            Utils::logMessage("Sleep was prempted");
            return false;
        }

        if (secs > 0)
            std::this_thread::sleep_for(std::chrono::duration<double>(secs));
        else
            Utils::logMessage("Missed sleep: %f ms", secs*1000);

        return !isCancelled();
    }

private:
    std::atomic_bool* cancelled_;
};

class Waiter {
private:
    ClockBase* clock_ = ClockFactory::get();

    TTimePoint proc_start_;
    TTimePoint loop_start_;

    TTimeDelta sleep_duration_, timeout_duration_;
public:
    Waiter(TTimeDelta sleep_duration_seconds, TTimeDelta timeout_duration = std::numeric_limits<TTimeDelta>::max())
        : sleep_duration_(sleep_duration_seconds), timeout_duration_(timeout_duration)
    {
        proc_start_ = loop_start_ = clock_->nowNanos();
    }

    virtual bool sleep(CancelableBase& cancelable_action)
    {
        // Sleeps for the time needed to get current running time up to the requested sleep_duration_.
        // So this can be used to "throttle" any loop to check something every sleep_duration_ seconds.
        TTimeDelta running_time = clock_->elapsedSince(loop_start_);
        double remaining = sleep_duration_ - running_time;
        bool completed = cancelable_action.sleep(clock_->toWallDelta(remaining));
        loop_start_ = clock_->nowNanos();
        return completed;
    }

    void resetSleep()
    {
    	loop_start_ = clock_->nowNanos();
    }
    void resetTimeout()
    {
    	proc_start_ = clock_->nowNanos();
    }

    bool is_timeout() const
    {
    	return clock_->elapsedSince(proc_start_) >= timeout_duration_;
    }
};

}} //namespace
#endif
