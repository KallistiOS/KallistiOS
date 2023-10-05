/* KallistiOS ##version##

   examples/dreamcast/cpp/concurrency/concurrency.cpp

   Copyright (C) 2023 Falco Girgis



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

using namespace std::chrono_literals;

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

/* ===== TEST CASE 1: std::binary_semaphore ===== */
static std::binary_semaphore sem_main_to_thread, sem_thread_to_main;

void run_semaphore() {
    // wait for a signal from the main proc
    // by attempting to decrement the semaphore
    sem_main_to_thread.acquire();
 
    // this call blocks until the semaphore's count
    // is increased from the main proc
 
    std::cout << "[BINARY_SEMAPHORE] thread: Got the signal" << std::endl;
 
    // wait for 3 seconds to imitate some work
    // being done by the thread
    std::this_thread::sleep_for(1s);
 
    std::cout << "[BINARY_SEMAPHORE] thread: Send the signal" << std::endl;
 
    // signal the main proc back
    sem_thread_to_main.release();
}

void test_semaphore(void) {
    std::cout << "[BINARY_SEMAPHORE] Starting test." << std::endl;

    // create some worker thread
    std::thread thrWorker(run_semaphore);
 
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

/* ===== TEST CASE 2: std::latch ===== */
struct LatchJob {
    const std::string name;
    std::string product{"not worked"};
    std::thread action{};
};
 
void test_latch() {
    LatchJob jobs[]{{"Sonic"}, {"Knuckles"}, {"Tails"}};
 
    std::cout << "[LATCH] Starting test." << std::endl;

    std::latch work_done{std::size(jobs)};
    std::latch start_clean_up{1};
 
    auto work = [&](Job& my_job) {
        my_job.product = my_job.name + " worked";
        work_done.count_down();
        start_clean_up.wait();
        my_job.product = my_job.name + " cleaned";
    };
 
    std::cout << "[LATCH] Work is starting... ";
    for (auto& job : jobs)
        job.action = std::thread{work, std::ref(job)};
 
    work_done.wait();
    std::cout << "done." << std::endl;
    for (auto const& job : jobs)
        std::cout << "  " << job.product << '\n';
 
    std::cout << "[LATCH] Workers are cleaning up... ";
    start_clean_up.count_down();
    for (auto& job : jobs)
        job.action.join();
 
    std::cout << "done." << std::endl;
    for (auto const& job : jobs) {
        if(job.product == "not worked") {
            throw TestCaseException("[LATCH] Job failed to produce!");
        }
        std::cout << "  " << job.product << std::endl;
    }

    std::cout << "[LATCH] Finished test." << std::endl;
}

/* ===== TEST CASE 3: std::shared_lock ===== */

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
 
void test_shared_lock(void) {
    SharedLockCounter counter;

    std::cout << "[SHARED_MUTEX] Starting test." << std::endl;

    auto increment_and_print = [&counter]() {
        for(int i{}; i != 3; ++i) {
            counter.increment();
            std::osyncstream(std::cout)
                << std::this_thread::get_id() << ' ' 
                << counter.get() << std::endl;
        }
    };
 
    std::thread thread1(increment_and_print);
    std::thread thread2(increment_and_print);
 
    thread1.join();
    thread2.join();

    if(counter.get() != 3) 
        throw TestCaseException("[SHARED_MUTEX]: Unexpected counter value!");

    std::cout << "[SHARED_MUTEX] Finished test." << std::endl;
}

/* ===== TEST CASE 4: std::condition_variable ===== */
static struct { 
    std::mutex m;
    std::condition_variable cv;
    std::string data;
    bool ready = false;
    bool processed = false;
} cond_variable_state;

void run_condition_variable() {
    // Wait until main() sends data
    std::unique_lock lk(cond_variable_state.m);
    cond_variable_state.cv.wait(cond_variable_state.lk, 
                                []{ return ready; });
 
    // after the wait, we own the lock.
    std::cout << "[COND_VARIABLE]: Worker thread is processing data" << std::endl;
    cond_variable_state.data += " after processing";
 
    // Send data back to main()
    cond_variable_state.processed = true;
    std::cout << "[COND_VARIABLE]: Worker thread signals data "
                 "processing completed" << std::endl;
 
    // Manual unlocking is done before notifying, to avoid waking up
    // the waiting thread only to block again (see notify_one for details)
    cond_variable_state.lk.unlock();
    cond_variable_state.cv.notify_one();
}
 
void test_condition_variable() {
    std::cout << "[COND_VARIABLE] Starting test." << std::endl;

    std::thread worker(run_condition_variable);
 
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
        cond_variable_state.cv.wait(cond_variable_state.lk, 
                                    []{ return processed; });
    }
    std::cout << "[COND_VARIABLE] Back in main(), data = " 
              << data << std::endl;
 
    if(cond_variable_state.data != "Example data after processing")
        throw TestCaseException("[COND_VARIABLE]: Unexpected value for data!");

    worker.join();

    std::cout << "[COND_VARIABLE] Finished test." << std::endl;
}

/* ===== TEST CASE 5: std::shared_timed_mutex ===== */
class R {
    mutable std::shared_timed_mutex mut;
    /* data */
public:
    R& operator=(const R& other) {
        // requires exclusive ownership to write to *this
        std::unique_lock<std::shared_timed_mutex> lhs(mut, std::defer_lock);
        // requires shared ownership to read from other
        std::shared_lock<std::shared_timed_mutex> rhs(other.mut, std::defer_lock);
        std::lock(lhs, rhs);
        /* assign data */
        return *this;
    }
};
 
void test_shared_timed_mutex() {
    R r;
}
 
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
 
void send_mail(Employee &, Employee &) {
    // simulate a time-consuming messaging operation
    std::this_thread::sleep_for(1s);
}
 
void assign_lunch_partner(Employee &e1, Employee &e2) {
    static std::mutex io_mutex;
    {
        std::lock_guard<std::mutex> lk(io_mutex);
        std::cout << e1.id << " and " << e2.id << " are waiting for locks" << std::endl;
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
            std::cout << e1.id << " and " << e2.id << " got locks" << std::endl;
        }
        e1.lunch_partners.push_back(e2.id);
        e2.lunch_partners.push_back(e1.id);
    }
 
    send_mail(e1, e2);
    send_mail(e2, e1);
}
 
void test_scoped_lock() {
    Employee alice("Alice"), bob("Bob"), christina("Christina"), dave("Dave");
 
    // assign in parallel threads because mailing users about lunch assignments
    // takes a long time
    std::vector<std::thread> threads;
    threads.emplace_back(assign_lunch_partner, std::ref(alice), std::ref(bob));
    threads.emplace_back(assign_lunch_partner, std::ref(christina), std::ref(bob));
    threads.emplace_back(assign_lunch_partner, std::ref(christina), std::ref(alice));
    threads.emplace_back(assign_lunch_partner, std::ref(dave), std::ref(bob));
 
    for (auto &thread : threads)
        thread.join();

    std::cout << alice.partners() << '\n'  << bob.partners() << '\n'
              << christina.partners() << '\n' << dave.partners() << '\n';
}

void test_barrier() {
    const auto workers = { "Dreamcast", "Playstation 2", "Gamecube", "Xbox" };
 
    auto on_completion = []() noexcept {
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

void run_stop_source(int id, std::stop_source stop_source) {
    std::stop_token stoken = stop_source.get_token();

    for (int i = 10; i; --i) {
        std::this_thread::sleep_for(300ms);
        
        if (stoken.stop_requested()) {
            std::cout << "[STOP_SOURCE] worker " << id 
                      << " is requested to stop" << std::endl;
            return;
        }
        
        std::cout << "[STOP_SOURCE] worker " << id 
                  << " goes back to sleep" << std::endl;
    }
}
 
void test_stop_source() {
    std::jthread threads[4];

    std::cout << "[STOP_SOURCE]: Starting test." << std::endl;

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
        threads[i] = std::jthread(run_stop_source, i + 1, stop_source);
 
    std::this_thread::sleep_for(500ms);
 
    std::cout << "[STOP_SOURCE] Request stop" << std::endl;
    stop_source.request_stop();
 
    print(stop_source);

    std::cout << "[STOP_SOURCE]: Finishing test." << std::endl; 

    // Note: destructor of jthreads will call join so no need for explicit calls
}

using namespace std;

class promise_manual_control {
  public:
    auto initial_suspend() {
        return suspend_always{}; // suspend after invoke
    }
    auto final_suspend() noexcept {
        return suspend_always{}; // suspend after return
    }
    void unhandled_exception() {
        // this example never 'throw'. so nothing to do
    }
};

//  behavior will be defined as a coroutine
class user_behavior_t : public coroutine_handle<void> {
  public:
    class promise_type : public promise_manual_control {
      public:
        void return_void() {}
        auto get_return_object() -> user_behavior_t {
            return {this};
        }
    };

  private:
    user_behavior_t(promise_type* p) : coroutine_handle<void>{} {
        coroutine_handle<void>& self = *this;
        self = coroutine_handle<promise_type>::from_promise(*p);
    }

  public:
    user_behavior_t() = default;
};

//  for this example,
//  chamber is an indices of the revolver's cylinder
using chamber_t = uint32_t;

auto select_chamber() -> chamber_t {
    std::random_device device{};
    std::mt19937_64 gen{device()};
    return static_cast<chamber_t>(gen());
}

//  trigger fires the bullet
//  all players will 'wait' for it
class trigger_t {
    const chamber_t& loaded;
    chamber_t& current;

  public:
    trigger_t(const chamber_t& _loaded, chamber_t& _current)
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
    void await_suspend(coroutine_handle<void>) {
    }
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
auto player(std::size_t id, bool& fired, trigger_t& trigger) -> user_behavior_t {
    // bang !
    fired = co_await trigger;
    fired ? std::cout << "[COROUTINES]: Player " << id << " dead  :(" << std::endl;
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

namespace detail {
    template <typename F> class defer_raii {
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
}  // namespace detail

template <typename F> detail::defer_raii<F> defer(F &&f) {
  return {std::forward<F>(f)};
}

// the game will go on until the revolver fires its bullet
auto russian_roulette(revolver_t& revolver, std::span<user_behavior_t> users) {
    bool fired = false;

    // spawn player coroutines with their id
    std::size_t id{};
    for (auto& user : users)
        user = player(++id, fired, revolver);

    // cleanup the game on return
    auto on_finish = defer([users] {
        for (coroutine_handle<void>& frame : users)
            frame.destroy();
    });

    // until there is a victim ...
    for (id = 0u; fired == false; id = (id + 1) % users.size()) {
        // continue the users' behavior in round-robin manner
        coroutine_handle<void>& task = users[id];
        if (task.done() == false)
            task.resume();
    }
}

void test_coroutines() {
    std::cout << "[COROUTINES]: Starting test." << std::endl;

    // select some chamber with the users
    array<user_behavior_t, 6> users{};
    revolver_t revolver{select_chamber(),
                        static_cast<chamber_t>(users.max_size())};

    russian_roulette(revolver, users);

    std::cout << "[COROUTINES]: Finishing test." << std::endl;
}

//x lock_guard
//x scoped_lock
//x std::condition_variable
//x unique_lock
//x shared_timed_mutex
//x shared_mutex (R/W lock)
//x defer_lock_t
//x latch
//x barrier
//x counting semaphore
//x stop token, stop source

int main(int argc, char* argv[]) { 
    /* Spawn an 10 automatically joined threads which will each spawn threads 
       for and asynchronously await the results of each individual test case... 
       Basically making each thread execute its own instances of every test case 
       concurrently (each of which spawns more threads for their respective tests). 

        Tests: thread, jthread, coroutines, atomics, thread-local storage, 
        lock_guard, scoped_lock, condition_variable, unique_lock, call_once
        shared_timed_mtuex, shared_mutex, defer_lock_t, latch, barrier, 
        counting_semaphore, binary_semaphore, stop_token, stop_source,
        future, syncstream
       */
    std::vector<std::jthread> threads;
    for(int i = 0; i < 10; ++i)
        threads.emplace_back([]() {
            return std::array { 
                std::async(test_semaphore),
                std::async(test_latch),
                std::async(test_shared_mutex),
                std::async(test_condition_variable),
                std::async(test_shared_timed_mutex),
                std::async(test_scoped_lock),
                std::async(test_barrier),
                std::async(test_stop_source),
                std::async(test_coroutines)
            };
        });

     return EXIT_SUCCESS;
}