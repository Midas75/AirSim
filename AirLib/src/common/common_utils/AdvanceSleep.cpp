#include "common/common_utils/AdvanceSleep.h"
#if SLEEP_MODE != 0
double nowMs()
{
    return std::chrono::duration<double, std::milli>(std::chrono::system_clock::now().time_since_epoch()).count();
}
#endif
#if SLEEP_MODE == 1
namespace advance_sleep
{
atomic_queue::AtomicQueueB<
    Event*,
    std::allocator<Event*>,
    (Event*)NULL,
    false,
    false,
    false>
    eventQueue(1024);
std::priority_queue<Event*, std::vector<Event*>, CompareEvent> pq;
volatile bool busySpinQuit = false;
void busySpin()
{
    busySpinQuit = false;
    while (!busySpinQuit) {
        Event* p;
        while (eventQueue.try_pop(p)) {
            pq.push(p);
        }
        if (pq.empty()) {
            continue;
        }
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        double ts = std::chrono::duration<double, std::milli>(duration).count();
        while (!pq.empty() && pq.top()->wakeUpTimeMs <= ts) {
            pq.top()->p.set_value();
            pq.pop();
        }
    }
}
}
void advanceSleep(double ms)
{
    static std::thread busySpinThread(advance_sleep::busySpin);
    advance_sleep::Event e;
    e.wakeUpTimeMs = ms + nowMs();
    advance_sleep::eventQueue.push(&e);
    e.p.get_future().wait();
}
#elif SLEEP_MODE == 3
void advanceSleep(double ms)
{
    auto start = nowMs();
    while (nowMs() - start < ms) {
        std::this_thread::yield();
    }
}
#elif SLEEP_MODE == 4
void advanceSleep(double ms)
{
    auto start = nowMs();
    static constexpr std::chrono::duration<double> MinSleepDuration(0);
    while (nowMs() - start < ms) {
        std::this_thread::sleep_for(MinSleepDuration);
    }
}
#elif SLEEP_MODE == 5
void advanceSleep(double ms)
{
    auto start = nowMs();
    while (nowMs() - start < ms) {
    }
}
#elif SLEEP_MODE != 0 // SLEEP_MODE == 2
void advanceSleep(double ms)
{
    std::this_thread::sleep_for(std::chrono::nanoseconds((long long)(ms * 1e6)));
}
#endif