/* KallistiOS ##version##

   examples/dreamcast/cpp/concurrency/concurrency.cpp

   Copyright (C) 2023 Falco Girgis

*/

/* 
    This file serves as both an example of using and as validation test suite
    for all of standard C++ concurrency, through C++20. It is composed of 8 
    standalone test cases, which have been either created from scratch or have
    been adopted from examples on cppreference.com. These test cases aim to 
    flex the various synchronization primitives and threading constructs,
    demonstrating their usage and ensuring that KOS, the toolchain, and 
    libstdc++ are behaving properly. After the tests run, a final result 
    message will be displayed, indicating whether the test suite passed or 
    failed. 

    The following constructs are tested:
        - atomic
        - thread_local
        - async
        - thread/jthread
        - mutex
        - shared_mutex
        - unique_lock
        - lock_guard
        - future
        - promise
        - semaphore
        - latch
        - shared_lock
        - condition_variable
        - scoped_lock
        - barrier
        - stop_source
        - stop_token
        - coroutine
        - syncstream
*/

#include <iostream>
#include <atomic>
#include <array>
#include <future>
#include <chrono>
#include <functional>
#include <coroutine>
#include <semaphore>
#include <thread>
#include <barrier>
#include <cstdlib>
#include <latch>
#include <syncstream>
#include <shared_mutex>
#include <condition_variable>
#include <stop_token>
#include <span>
#include <random>
#include <exception>
#include <string>

#include <arch/wdt.h>

// 20 seconds in us
inline constexpr unsigned WATCHDOG_TIMEOUT = (20 * 1000 * 1000);  
// Number of threads to spawn -- each of which runs the entire test suite
inline constexpr int THREAD_COUNT = 10; 

using namespace std::chrono_literals;

// Exception type for test-case errors
class TestCaseException: public std::exception { 
        std::string what_;
    public:
        TestCaseException(std::string str) noexcept:
            what_(std::move(str)) {}

        TestCaseException& 
        operator=(const TestCaseException& other) noexcept = default;

        const char* what() const noexcept override {
            return what_.c_str();
        }
};

/* ===== TEST CASE 1: std::binary_semaphore ===== 
    Simply spawns a worker thread and ping-pongs acquiring
    and releasing two binary_semaphores between the worker and
    parent thread.
*/
static void run_semaphore(std::binary_semaphore &sem_main_to_thread,
                          std::binary_semaphore &sem_thread_to_main) {
    // wait for a signal from the main proc
    // by attempting to decrement the semaphore
    sem_main_to_thread.acquire();
 
    // this call blocks until the semaphore's count
    // is increased from the main proc
 
    std::cout << "[BINARY_SEMAPHORE] thread: Got the signal" << std::endl;
 
    // wait for 3 seconds to imitate some work
    // being done by the thread
    std::this_thread::sleep_for(1ms);
 
    std::cout << "[BINARY_SEMAPHORE] thread: Send the signal" << std::endl;
 
    // signal the main proc back
    sem_thread_to_main.release();
}

static void test_semaphore() {
    std::binary_semaphore sem_main_to_thread{0}, sem_thread_to_main{0};
    std::cout << "[BINARY_SEMAPHORE] Starting test." << std::endl;

    // create some worker thread
    std::thread thrWorker(run_semaphore, 
                          std::ref(sem_main_to_thread), 
                          std::ref(sem_thread_to_main));
 
    std::cout << "[BINARY_SEMAPHORE] main: Send the signal" << std::endl;
 
    // signal the worker thread to start working
    // by increasing the semaphore's count
    sem_main_to_thread.release();
 
    // wait until the worker thread is done doing the work
    // by attempting to decrement the semaphore's count
    sem_thread_to_main.acquire();
 
    std::cout << "[BINARY_SEMAPHORE] main: Got the signal" << std::endl;
    thrWorker.join();

    std::cout << "[BINARY_SEMAPHORE] Finished test." << std::endl;
}

/* ===== TEST CASE 2: std::latch ===== 
    Creates 3 "Job" objects, each of which gets
    managed by its own thread. First, the main thread
    waits until all job threads have hit a synchronization
    point (creating work). Then the workers wait until 
    the main thread hits a synchronization point to begin
    cleaning their work. 
*/
struct LatchJob {
    const std::string name;
    std::string product{"not worked"};
    std::thread action{std::thread()};
};
 
static void test_latch() {
    LatchJob jobs[]{{"Sonic"}, {"Knuckles"}, {"Tails"}};
 
    std::cout << "[LATCH] Starting test." << std::endl;

    std::latch work_done{std::size(jobs)};
    std::latch start_clean_up{1};
 
    auto work = [&](LatchJob &my_job) {
        my_job.product = my_job.name + " worked";
        work_done.count_down();
        start_clean_up.wait();
        my_job.product = my_job.name + " cleaned";
    };
 
    std::cout << "[LATCH] Work is starting... " << std::endl;
    for (auto &job : jobs)
        job.action = std::thread{work, std::ref(job)};
 
    work_done.wait();
    std::cout << "[LATCH] done." << std::endl;
    for (auto const &job : jobs)
        std::cout << "[LATCH]  " << job.product << '\n';
 
    std::cout << "[LATCH] Workers are cleaning up... ";
    start_clean_up.count_down();
    for (auto &job : jobs)
        job.action.join();
 
    std::cout << "[LATCH] done." << std::endl;
    for (auto const& job : jobs) {
        if(job.product == "not worked") {
            throw TestCaseException("[LATCH] Job failed to produce!");
        }
        std::cout << "[LATCH]  " << job.product << std::endl;
    }

    std::cout << "[LATCH] Finished test." << std::endl;
}

/* ===== TEST CASE 3: std::shared_lock ===== 
    Creates a SharedLockCounter in the parent thread, which then
    gets passed to two child threads, both of which attempt to 
    increment its value 3 times and print it. Write operations 
    are protected via std::unique_locks, while the read operation,
    get(), is protected by a std::shared_lock. This models a 
    traditional "ReadWrite" lock.
*/

class SharedLockCounter {
public:
    SharedLockCounter() = default;
 
    // Multiple threads/readers can read the counter's value at the same time.
    unsigned int get() const {
        std::shared_lock lock(mutex_);
        return value_;
    }
 
    // Only one thread/writer can increment/write the counter's value.
    void increment() {
        std::unique_lock lock(mutex_);
        ++value_;
    }
 
    // Only one thread/writer can reset/write the counter's value.
    void reset() {
        std::unique_lock lock(mutex_);
        value_ = 0;
    }
 
private:
    mutable std::shared_mutex mutex_;
    unsigned int value_{};
};
 
static void test_shared_lock() {
    SharedLockCounter counter;

    std::cout << "[SHARED_LOCK] Starting test." << std::endl;

    auto increment_and_print = [&counter] {
        for(int i{}; i != 3; ++i) {
            counter.increment();
            std::osyncstream(std::cout)
                << "[SHARED_LOCK] "
                << std::this_thread::get_id() << ' ' 
                << counter.get() << std::endl;
        }
    };
 
    std::thread thread1(increment_and_print);
    std::thread thread2(increment_and_print);
 
    thread1.join();
    thread2.join();

    if(counter.get() != 6) 
        throw TestCaseException("[SHARED_LOCK]: Unexpected counter value!");

    std::cout << "[SHARED_LOCK] Finished test." << std::endl;
}

/* ===== TEST CASE 4: std::condition_variable ===== 
    Spawns a worker thread, passing it a state object with both a 
    mutex and a condition_variable. The parent thread populates the 
    data field within the state object then uses the condition_variable
    to signal to one thread (the child) that its data is ready to process.
    The parent thread then waits on the condition_variable, which the
    child thread signals back when it is done processing its work data.

    Basically the parent and child threads swap between signalling to 
    each other to proceed execution via the condition_variable.
*/
struct CondVarState { 
    std::mutex m;
    std::condition_variable cv;
    std::string data;
    bool ready = false;
    bool processed = false;
};

static void run_condition_variable(CondVarState &cond_variable_state) {
    // Wait until main() sends data
    std::unique_lock lk(cond_variable_state.m);
    cond_variable_state.cv.wait(lk, [&]{ return cond_variable_state.ready; });
 
    // after the wait, we own the lock.
    std::cout << "[COND_VARIABLE]: Worker thread is processing data" << std::endl;
    cond_variable_state.data += " after processing";
 
    // Send data back to main()
    cond_variable_state.processed = true;
    std::cout << "[COND_VARIABLE]: Worker thread signals data "
                 "processing completed" << std::endl;
 
    // Manual unlocking is done before notifying, to avoid waking up
    // the waiting thread only to block again (see notify_one for details)
    lk.unlock();
    cond_variable_state.cv.notify_one();
}
 
static void test_condition_variable() {
    CondVarState cond_variable_state;

    std::cout << "[COND_VARIABLE] Starting test." << std::endl;

    std::thread worker(run_condition_variable, std::ref(cond_variable_state));
 
    cond_variable_state.data = "Example data";
    // send data to the worker thread
    {
        std::lock_guard lk(cond_variable_state.m);
        cond_variable_state.ready = true;
        std::cout << "[COND_VARIABLE] main() signals data ready "
                     "for processing" << std::endl;
    }
    cond_variable_state.cv.notify_one();
 
    // wait for the worker
    {
        std::unique_lock lk(cond_variable_state.m);
        cond_variable_state.cv.wait(lk, [&]{ return cond_variable_state.processed; });
    }
    std::cout << "[COND_VARIABLE] Back in main(), data = " 
              << cond_variable_state.data << std::endl;
 
    if(cond_variable_state.data != "Example data after processing")
        throw TestCaseException("[COND_VARIABLE]: Unexpected value for data!");

    worker.join();

    std::cout << "[COND_VARIABLE] Finished test." << std::endl;
}
 
/* ===== TEST CASE 5: std::scoped_lock ===== 
    The parent thread creates 4 different employees,
    each of which has a list of lunch_partners as well as a 
    mutex to control access to them. The main thread then 
    creates 4 child threads, passing each a different pair 
    of employees. Each child thread uses a std::scoped_lock
    to acquire the two employee locks simultaneously, before
    adding them to each other's lunch partner lists. Finally, 
    the resulting lunch partner lists are printed. 
*/
struct Employee {
    std::vector<std::string> lunch_partners;
    std::string id;
    std::mutex m;
    Employee(std::string id) : id(id) {}
    std::string partners() const
    {
        std::string ret = "Employee " + id + " has lunch partners: ";
        for (const auto& partner : lunch_partners)
            ret += partner + " ";
        return ret;
    }
};
 
static void send_mail(Employee &, Employee &) {
    // simulate a time-consuming messaging operation
    std::this_thread::yield();
}
 
static void assign_lunch_partner(Employee &e1, Employee &e2) {
    static thread_local std::mutex io_mutex;
    {
        std::lock_guard<std::mutex> lk(io_mutex);
        std::cout << "[SCOPED_LOCK] " << e1.id << " and " << e2.id 
                  << " are waiting for locks" << std::endl;
    }
 
    {
        // use std::scoped_lock to acquire two locks without worrying about
        // other calls to assign_lunch_partner deadlocking us
        // and it also provides a convenient RAII-style mechanism
 
        std::scoped_lock lock(e1.m, e2.m);
 
        // Equivalent code 1 (using std::lock and std::lock_guard)
        // std::lock(e1.m, e2.m);
        // std::lock_guard<std::mutex> lk1(e1.m, std::adopt_lock);
        // std::lock_guard<std::mutex> lk2(e2.m, std::adopt_lock);
 
        // Equivalent code 2 (if unique_locks are needed, e.g. for condition variables)
        // std::unique_lock<std::mutex> lk1(e1.m, std::defer_lock);
        // std::unique_lock<std::mutex> lk2(e2.m, std::defer_lock);
        // std::lock(lk1, lk2);
        {
            std::lock_guard<std::mutex> lk(io_mutex);
            std::cout << "[SCOPED_LOCK] " << e1.id << " and " << e2.id 
                      << " got locks" << std::endl;
        }
        e1.lunch_partners.push_back(e2.id);
        e2.lunch_partners.push_back(e1.id);
    }
 
    send_mail(e1, e2);
    send_mail(e2, e1);
}
 
static void test_scoped_lock() {
    Employee ryo("RyoHazuki"), amigo("SambaDeAmigo"), 
             eggman("Dr.Eggman"), ulala("Ulala");
 
    // assign in parallel threads because mailing users about lunch assignments
    // takes a long time
    std::vector<std::thread> threads;
    threads.emplace_back(assign_lunch_partner, std::ref(ryo), std::ref(amigo));
    threads.emplace_back(assign_lunch_partner, std::ref(eggman), std::ref(amigo));
    threads.emplace_back(assign_lunch_partner, std::ref(eggman), std::ref(ryo));
    threads.emplace_back(assign_lunch_partner, std::ref(ulala), std::ref(amigo));
 
    for (auto &thread : threads)
        thread.join();

    std::cout << "[SCOPED_LOCK] " << ryo.partners() << '\n'  
              << "[SCOPED_LOCK] " << amigo.partners() << '\n'
              << "[SCOPED_LOCK] " << eggman.partners() << '\n' 
              << "[SCOPED_LOCK] " << ulala.partners() << std::endl;
}

/* ===== TEST CASE 6: std::barrier ===== 
    The parent thread creates a list of 4 workers and a std::sync_point
    of the same size with a completion callback. It then spawns a child
    thread for each worker, which performs some work then awaits for all
    threads to hit the sync point before continuing on to clean the work
    and finally hitting a second sync point before exiting. 
*/
static void test_barrier() {
    const auto workers = { "Dreamcast", "Playstation 2", "Gamecube", "Xbox" };
 
    auto on_completion = [] {
        // locking not needed here
        static auto phase = "... done\n" "Cleaning up...\n";
        std::cout << "[BARRIER] " << phase;
        phase = "... done\n";
    };
 
    std::barrier sync_point(std::ssize(workers), on_completion);
 
    auto work = [&](std::string name) {
        std::string product = "  " + name + " worked\n";
        std::cout << "[BARRIER] " << product;  // ok, op<< call is atomic
        sync_point.arrive_and_wait();
 
        product = "  " + name + " cleaned\n";
        std::cout << "[BARRIER] " << product;
        sync_point.arrive_and_wait();
    };
 
    std::cout << "[BARRIER] Starting test." << std::endl;
    std::vector<std::jthread> threads;
    threads.reserve(std::size(workers));
    for (auto const& worker : workers)
        threads.emplace_back(work, worker);

    std::cout << "[BARRIER] Finishing test." << std::endl;
}

/* ===== TEST CASE 7: std::stop_source ===== 
    The parent thread creates a stop_source, then creates
    4 child threads, passing each one the stop_source. Each 
    child thread begins a loop of simply sleeping unless a
    stop is requested. After 150ms elapse, the parent thread
    requests that the child threads stop, after which they
    return and join with their parent thread. 
*/
static void run_stop_source(int id, std::stop_source stop_source, std::atomic<unsigned> &counter) {
    std::stop_token stoken = stop_source.get_token();

    for (int i = 10; i; --i) {
        std::this_thread::sleep_for(1ms);
        
        if (stoken.stop_requested()) {
            std::cout << "[STOP_SOURCE] worker " << id 
                      << " is requested to stop" << std::endl;
            break;
        }
        
        std::cout << "[STOP_SOURCE] worker " << id 
                  << " goes back to sleep" << std::endl;
    }

    ++counter;
}
 
static void test_stop_source() {
    std::thread threads[4];
    std::atomic<unsigned> counter = 0;

    std::cout << "[STOP_SOURCE] Starting test." << std::endl;

    std::cout << std::boolalpha;
    auto print = [](const std::stop_source &source) {
        std::cout << "[STOP_SOURCE] stop_possible = "
                  << source.stop_possible() << " stop_requested = " 
                  << source.stop_requested() << std::endl;
    };
 
    // Common source
    std::stop_source stop_source;
 
    print(stop_source);
 
    // Create worker threads
    for (int i = 0; i < 4; ++i)
        threads[i] = std::thread(run_stop_source, i + 1, stop_source, std::ref(counter));
 
    std::this_thread::sleep_for(150ms);
 
    std::cout << "[STOP_SOURCE] Request stop" << std::endl;
    stop_source.request_stop();
 
    print(stop_source);

    std::cout << "[STOP_SOURCE] Finishing test." << std::endl; 

    for (int i = 0; i < 4; ++i)
        threads[i].join();

    if(counter != 4)
        throw TestCaseException("[STOP_SOURCE]: Unexpected value for atomic counter!");
}

/* ===== TEST CASE 8: coroutines ===== 
    This test creates a coroutine-based game of Russian
    Roulette. 

    The parent thread creates an array of 6 user_behavior_t
    coroutine objects, which model the players. The parent
    thread then creates a revolver, whose chamber size is 
    as large as the number of players, and which chooses
    one chamber position at random to load a bullet into. 

    Finally, the parent thread calls russian_roulette(), 
    which iterates over each user coroutine, updating its
    state based on whether the revolver fired or hit the
    player. It continues to do this until a victim has 
    been found. 
*/

class promise_manual_control {
  public:
    auto initial_suspend() {
        return std::suspend_always{}; // suspend after invoke
    }
    auto final_suspend() noexcept {
        return std::suspend_always{}; // suspend after return
    }
    void unhandled_exception() {
        // this example never 'throw'. so nothing to do
    }
};

//  behavior will be defined as a coroutine
class user_behavior_t : public std::coroutine_handle<void> {
  public:
    class promise_type : public promise_manual_control {
      public:
        void return_void() {}
        auto get_return_object() -> user_behavior_t {
            return {this};
        }
    };

  private:
    user_behavior_t(promise_type* p) : std::coroutine_handle<void>{} {
        std::coroutine_handle<void> &self = *this;
        self = std::coroutine_handle<promise_type>::from_promise(*p);
    }

  public:
    user_behavior_t() = default;
};

//  for this example,
//  chamber is an indices of the revolver's cylinder
using chamber_t = uint32_t;

static auto select_chamber() -> chamber_t {
    std::random_device device{};
    std::mt19937_64 gen{device()};
    return static_cast<chamber_t>(gen());
}

//  trigger fires the bullet
//  all players will 'wait' for it
class trigger_t {
    const chamber_t &loaded;
    chamber_t &current;

  public:
    trigger_t(const chamber_t &_loaded, chamber_t &_current)
        : loaded{_loaded}, current{_current} {
    }

  private:
    bool pull() { // pull the trigger. is it the bad case?
        return --current == loaded;
    }

  public:
    bool await_ready() {
        return false;
    }
    void await_suspend(std::coroutine_handle<void>) {}
    bool await_resume() {
        return pull();
    }
};

//  this player will ...
//  1. be bypassed
//     (fired = false; then return)
//  2. receive the bullet
//     (fired = true; then return)
//  3. be skipped because of the other player became a victim
//     (destroyed when it is suspended - no output)
static auto player(std::size_t id, bool &fired, trigger_t &trigger) -> user_behavior_t {
    // bang !
    fired = co_await trigger;
    fired ? std::cout << "[COROUTINES]: Player " << id << " dead  :(" << std::endl
          : std::cout << "[COROUTINES]: Player " << id << " alive :)" << std::endl;
}

// revolver knows which is the loaded chamber
class revolver_t : public trigger_t {
    const chamber_t loaded;
    chamber_t current;

  public:
    revolver_t(chamber_t chamber, chamber_t num_player)
        : trigger_t{loaded, current}, //
          loaded{chamber % num_player}, current{num_player} {
    }
};

namespace {
    template <typename F> 
    class defer_raii {
    public:
        // copy/move construction and any kind of assignment would lead to the cleanup function getting
        // called twice. We can't have that.
        defer_raii(defer_raii &&) = delete;
        defer_raii(const defer_raii &) = delete;
        defer_raii &operator=(const defer_raii &) = delete;
        defer_raii &operator=(defer_raii &&) = delete;

        // construct the object from the given callable
        template <typename FF> defer_raii(FF &&f) : cleanup_function(std::forward<FF>(f)) {}

        // when the object goes out of scope call the cleanup function
        ~defer_raii() { cleanup_function(); }

    private:
        F cleanup_function;
    };

    template<typename FF>
    defer_raii(FF &&ff) -> defer_raii<decltype(ff)>;
}  // anonymous namespace

auto defer(auto &&f) {
    return defer_raii { std::forward<decltype(f)>(f) };
}

// the game will go on until the revolver fires its bullet
static void russian_roulette(revolver_t &revolver, std::span<user_behavior_t> users) {
    bool fired = false;

    // spawn player coroutines with their id
    std::size_t id{};
    for (auto &user : users)
        user = player(++id, fired, revolver);

    // cleanup the game on return
    auto on_finish = defer([users] {
        for (std::coroutine_handle<void> &frame : users)
            frame.destroy();
    });

    // until there is a victim ...
    for (id = 0u; fired == false; id = (id + 1) % users.size()) {
        // continue the users' behavior in round-robin manner
        std::coroutine_handle<void> &task = users[id];
        if (task.done() == false)
            task.resume();
    }
}

static void test_coroutines() {
    std::cout << "[COROUTINES]: Starting test." << std::endl;

    // select some chamber with the users
    std::array<user_behavior_t, 6> users{};
    revolver_t revolver{select_chamber(),
                        static_cast<chamber_t>(users.max_size())};

    russian_roulette(revolver, users);

    std::cout << "[COROUTINES]: Finished test." << std::endl;
}

// Main program entry point 
int main(int argc, char* argv[]) { 
    /* Enable the watchdog timer so that if, for some reason, this
       test deadlocks or hangs indefinitely, it will still gracefully
       exit and report test failure back to dc-tool. */
    wdt_enable_timer(0, WATCHDOG_TIMEOUT, 0xf, 
                    [](void*) { 
                        std::cerr << "FAILURE: Watchdog timeout reached!"
                                  << std::endl;
                        exit(EXIT_FAILURE);
                    }, nullptr);

    /* Spawn N automatically joined threads which will each spawn threads 
       for each test case and asynchronously wait upon their results... Basically 
       making each thread execute its own instances of every test case concurrently 
       (each of which spawns more threads within its respective test logic). */
    try { 
        std::vector<std::jthread> threads;
        
        for(int i = 0; i < THREAD_COUNT; ++i)
            threads.emplace_back([] {
                return std::array { 
                    std::async(test_semaphore),
                    std::async(test_latch),
                    std::async(test_shared_lock),
                    std::async(test_condition_variable),
                    std::async(test_scoped_lock),
                    std::async(test_barrier),
                    std::async(test_stop_source),
                    std::async(test_coroutines)
                };
            });

    } catch(TestCaseException& except) { 
        std::cerr << "\n\n****** C++ Concurrency Test: FAILURE *****\n\t" 
                  << except.what() << "\n" << std::endl;

        return EXIT_FAILURE;
    }

    std::cout << "\n\n***** C++ Concurrency Test: SUCCESS *****\n" << std::endl;

    return EXIT_SUCCESS;
}
