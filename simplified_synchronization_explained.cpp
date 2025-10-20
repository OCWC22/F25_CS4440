// ============================================================================
// SYNCHRONIZATION PROBLEMS EXPLAINED SIMPLY FOR ENGINEERS
// ============================================================================

#include <iostream>
#include <thread>
#include <mutex>
#include <semaphore>
#include <queue>
#include <chrono>
#include <vector>
#include <atomic>

// ============================================================================
// PROBLEM 1: LARRY, CURLY, MOE (LCM) - THE FARMING PROBLEM
// ============================================================================
//
// WHAT'S HAPPENING:
// - Larry digs holes (needs shovel)
// - Curley plants seeds in holes (no shovel needed)
// - Moe fills holes after seeds planted (needs shovel)
// - Only ONE shovel exists (shared resource)
// - Larry can only dig N holes ahead of Moe (rate limiting)
//
// WHY THIS MATTERS:
// This is like a production line where resources are limited
// Think: Assembly line, food processing, data pipeline
//
// SYNCHRONIZATION NEEDED:
// 1. Shovel: Binary semaphore (only one person can use it)
// 2. DigHole: Counting semaphore (limits how many holes Larry can dig)
// 3. Curley2go: Signal from Larry to Curley (hole ready for planting)
// 4. Moe2go: Signal from Curley to Moe (seed ready for filling)

class LCM_Farming {
private:
    // SHARED RESOURCES
    std::queue<std::string> holes;       // The holes being worked on

    // SEMAPHORES (explained in detail)
    std::counting_semaphore<> shovel{1}; // SHOVEL: Only 1 shovel available
                                        // Initial value 1 = one shovel exists
                                        // wait() = take shovel, signal() = return shovel

    std::counting_semaphore<> dig_hole{3}; // DIG_HOLE: Larry can dig 3 holes ahead
                                           // Initial value 3 = max holes ahead
                                           // Prevents Larry from digging too many
                                           // wait() = permission to dig, signal() = hole filled

    std::counting_semaphore<> curley_signal{0}; // CURLY2GO: Signal to Curley
                                               // Initial value 0 = Curley waits initially
                                               // Larry signals when hole is ready
                                               // Curley waits (acquires) then plants

    std::counting_semaphore<> moe_signal{0};    // MOE2GO: Signal to Moe
                                               // Initial value 0 = Moe waits initially
                                               // Curley signals when seed is planted
                                               // Moe waits (acquires) then fills

    std::atomic<bool> running{true};

public:
    // ========================================================================
    // LARRY (THE DIGGER) - PRODUCER
    // ========================================================================
    // Larry's job: Dig holes, but he's limited in how many he can dig ahead
    void larry_digger() {
        int hole_count = 0;

        while (running) {
            std::cout << "🚜 Larry: I want to dig hole #" << hole_count << std::endl;

            // STEP 1: Check if Larry can dig more holes (rate limiting)
            // wait(dig_hole): Larry asks "Can I dig another hole?"
            // If dig_hole > 0: Larry can dig, dig_hole decreases by 1
            // If dig_hole == 0: Larry must wait until Moe fills a hole
            dig_hole.acquire();

            std::cout << "🚜 Larry: Got permission to dig! Starting hole #" << hole_count << std::endl;

            // STEP 2: Get the shovel (resource protection)
            // wait(shovel): Larry asks for the shovel
            // Only one farmer can use shovel at a time!
            shovel.acquire();

            // STEP 3: Actually dig the hole
            std::string hole = "hole_" + std::to_string(hole_count);
            std::cout << "🚜 Larry: Digging " << hole << " [SHOVEL IN USE]" << std::endl;
            holes.push(hole);  // Add hole to the queue

            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Time to dig

            // STEP 4: Return the shovel
            std::cout << "🚜 Larry: Done digging, returning shovel" << std::endl;
            shovel.release();  // signal(shovel): Return shovel for others

            // STEP 5: Tell Curley there's a hole to plant
            // signal(curley_signal): Larry tells Curley "Hole is ready!"
            // Increments curley_signal from 0 to 1, waking up Curley
            std::cout << "🚜 Larry: Telling Curley to plant in " << hole << std::endl;
            curley_signal.release();

            hole_count++;

            if (hole_count >= 5) break; // Stop after 5 holes for demo
        }

        std::cout << "🚜 Larry: Finished digging!" << std::endl;
    }

    // ========================================================================
    // CURLY (THE PLANTER) - MIDDLE PROCESSOR
    // ========================================================================
    // Curley's job: Plant seeds in holes dug by Larry
    void curley_planter() {
        while (running) {
            // STEP 1: Wait for Larry to finish digging a hole
            // wait(curley_signal): Curley asks "Is there a hole ready?"
            // If curley_signal > 0: Hole is ready, curley_signal decreases by 1
            // If curley_signal == 0: Curley must wait for Larry to signal
            std::cout << "🌱 Curley: Waiting for Larry to finish digging..." << std::endl;
            curley_signal.acquire();

            // STEP 2: Get the hole from the queue
            std::string hole = holes.front();
            holes.pop();

            // STEP 3: Plant the seed (no shovel needed!)
            std::cout << "🌱 Curley: Planting seed in " << hole << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(300)); // Time to plant

            // STEP 4: Put the hole back (now with seed)
            holes.push(hole + "_with_seed");

            // STEP 5: Tell Moe there's a seed to fill
            // signal(moe_signal): Curley tells Moe "Seed is planted!"
            std::cout << "🌱 Curley: Telling Moe to fill " << hole << std::endl;
            moe_signal.release();

            if (hole.find("hole_4") != std::string::npos) break;
        }

        std::cout << "🌱 Curley: Finished planting!" << std::endl;
    }

    // ========================================================================
    // MOE (THE FILLER) - CONSUMER
    // ========================================================================
    // Moe's job: Fill holes after seeds are planted
    void moe_filler() {
        while (running) {
            // STEP 1: Wait for Curley to finish planting
            // wait(moe_signal): Moe asks "Is there a seed ready to fill?"
            std::cout << "🏗️ Moe: Waiting for Curley to finish planting..." << std::endl;
            moe_signal.acquire();

            // STEP 2: Get the hole with seed
            std::string hole = holes.front();
            holes.pop();

            // STEP 3: Get the shovel
            // wait(shovel): Moe needs shovel to fill hole
            std::cout << "🏗️ Moe: Need shovel to fill " << hole << std::endl;
            shovel.acquire();

            // STEP 4: Fill the hole
            std::cout << "🏗️ Moe: Filling " << hole << " [SHOVEL IN USE]" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(400)); // Time to fill

            // STEP 5: Return shovel and signal Larry can dig more
            std::cout << "🏗️ Moe: Filled " << hole << ", returning shovel" << std::endl;
            shovel.release();

            // STEP 6: Tell Larry he can dig another hole
            // signal(dig_hole): Moe tells Larry "Hole is filled, you can dig more!"
            // This is crucial - it prevents Larry from getting too far ahead
            std::cout << "🏗️ Moe: Telling Larry he can dig another hole" << std::endl;
            dig_hole.release();

            if (hole.find("hole_4") != std::string::npos) break;
        }

        std::cout << "🏗️ Moe: Finished filling!" << std::endl;
    }

    void stop() { running = false; }
};

// ============================================================================
// PROBLEM 2: PRODUCER-CONSUMER (BUFFERED PIPELINE)
// ============================================================================
//
// WHAT'S HAPPENING:
// - Producers create items and put them in a buffer
// - Consumers take items from buffer and process them
// - Buffer has limited capacity
// - Producers must wait if buffer is full
// - Consumers must wait if buffer is empty
//
// WHY THIS MATTERS:
// This is EVERYWHERE in computing:
// - Web servers: Producers=requests, Consumers=workers
// - Video streaming: Producers=encoder, Consumers=player
// - Printers: Producers=applications, Consumers=printer
//
// SYNCHRONIZATION NEEDED:
// 1. mutex: Protect buffer from concurrent access
// 2. empty: Count how many empty slots in buffer
// 3. full: Count how many full slots in buffer

class ProducerConsumer {
private:
    std::queue<std::string> buffer;         // Shared buffer
    const int BUFFER_SIZE = 5;              // Maximum buffer capacity

    // SYNCHRONIZATION PRIMITIVES
    std::mutex buffer_mutex;                // Protect buffer access
    std::counting_semaphore<> empty{5};     // Empty slots (initially all empty)
    std::counting_semaphore<> full{0};      // Full slots (initially none full)

    std::atomic<bool> running{true};

public:
    // ========================================================================
    // PRODUCER - CREATES ITEMS
    // ========================================================================
    void producer(int producer_id) {
        int item_count = 0;

        while (running) {
            // STEP 1: Check if buffer has space
            // wait(empty): Producer asks "Is there space in buffer?"
            // If empty > 0: Space available, empty decreases by 1
            // If empty == 0: Buffer full, producer must wait
            std::cout << "🏭 Producer " << producer_id << ": Checking buffer space..." << std::endl;
            empty.acquire();

            // STEP 2: Get exclusive access to buffer
            // wait(mutex): Protect buffer from concurrent modification
            std::cout << "🏭 Producer " << producer_id << ": Getting buffer access" << std::endl;
            buffer_mutex.lock();

            // STEP 3: Produce item and add to buffer
            std::string item = "item_" + std::to_string(producer_id) + "_" + std::to_string(item_count);
            buffer.push(item);
            std::cout << "🏭 Producer " << producer_id << ": Produced " << item
                      << " (buffer size: " << buffer.size() << "/" << BUFFER_SIZE << ")" << std::endl;

            item_count++;

            // STEP 4: Release buffer access
            buffer_mutex.unlock();

            // STEP 5: Signal that buffer has one more item
            // signal(full): Producer tells consumers "New item available!"
            std::cout << "🏭 Producer " << producer_id << ": Signaling item ready" << std::endl;
            full.release();

            std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Production time

            if (item_count >= 3) break; // Stop after 3 items
        }

        std::cout << "🏭 Producer " << producer_id << ": Finished production!" << std::endl;
    }

    // ========================================================================
    // CONSUMER - PROCESSES ITEMS
    // ========================================================================
    void consumer(int consumer_id) {
        int consumed = 0;

        while (running) {
            // STEP 1: Check if buffer has items
            // wait(full): Consumer asks "Are there items in buffer?"
            // If full > 0: Items available, full decreases by 1
            // If full == 0: Buffer empty, consumer must wait
            std::cout << "👤 Consumer " << consumer_id << ": Checking for items..." << std::endl;
            full.acquire();

            // STEP 2: Get exclusive access to buffer
            std::cout << "👤 Consumer " << consumer_id << ": Getting buffer access" << std::endl;
            buffer_mutex.lock();

            // STEP 3: Consume item from buffer
            std::string item = buffer.front();
            buffer.pop();
            std::cout << "👤 Consumer " << consumer_id << ": Consuming " << item
                      << " (buffer size: " << buffer.size() << "/" << BUFFER_SIZE << ")" << std::endl;

            consumed++;

            // STEP 4: Release buffer access
            buffer_mutex.unlock();

            // STEP 5: Signal that buffer has one more empty slot
            // signal(empty): Consumer tells producers "Space available!"
            std::cout << "👤 Consumer " << consumer_id << ": Signaling space available" << std::endl;
            empty.release();

            // STEP 6: Process the item
            std::cout << "👤 Consumer " << consumer_id << ": Processing " << item << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(300)); // Processing time

            if (consumed >= 3) break; // Stop after 3 items
        }

        std::cout << "👤 Consumer " << consumer_id << ": Finished consumption!" << std::endl;
    }

    void stop() { running = false; }
};

// ============================================================================
// PROBLEM 3: DINING PHILOSOPHERS - DEADLOCK PROBLEM
// ============================================================================
//
// WHAT'S HAPPENING:
// - 5 philosophers sitting at table
// - Each needs 2 chopsticks to eat
// - Chopsticks are shared between adjacent philosophers
// - Philosophers alternate between thinking and eating
//
// WHY THIS MATTERS:
// This represents resource allocation problems:
// - Database connections: Each transaction needs multiple resources
// - Network routing: Routers need multiple links
// - Process scheduling: Processes need multiple resources
//
// THE DEADLOCK PROBLEM:
// All philosophers pick up left chopstick simultaneously
// Then all wait for right chopstick (which is held by neighbor)
// DEADLOCK! Everyone waits forever
//
// SOLUTION: Resource ordering or limiting concurrent philosophers

class DiningPhilosophers {
private:
    // Each chopstick is a mutex (binary semaphore)
    std::mutex chopsticks[5];  // 5 chopsticks, one between each philosopher

    std::atomic<bool> dining{true};

public:
    // ========================================================================
    // PHILOSOPHER - THE ALGORITHM THAT CAUSES DEADLOCK
    // ========================================================================
    // This is the textbook algorithm that causes deadlock!
    void philosopher(int philosopher_id) {
        int left_chopstick = philosopher_id;                    // My left chopstick
        int right_chopstick = (philosopher_id + 1) % 5;        // My right chopstick
        // % 5 makes it circular: philosopher 4's right is philosopher 0's left

        while (dining) {
            // STEP 1: Think
            std::cout << "🤔 Philosopher " << philosopher_id << ": Thinking..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            // STEP 2: Get hungry and try to eat
            std::cout << "🍽️ Philosopher " << philosopher_id << ": Getting hungry!" << std::endl;

            // STEP 3: Pick up left chopstick
            // wait(chopstick[i]): Pick up left chopstick
            std::cout << "🥢 Philosopher " << philosopher_id << ": Picking up left chopstick "
                      << left_chopstick << std::endl;
            chopsticks[left_chopstick].lock();

            // STEP 4: Pick up right chopstick (THIS IS WHERE DEADLOCK HAPPENS!)
            // wait(chopstick[(i + 1) % 5]): Pick up right chopstick
            std::cout << "🥢 Philosopher " << philosopher_id << ": Picking up right chopstick "
                      << right_chopstick << std::endl;
            chopsticks[right_chopstick].lock();

            // STEP 5: Eat (only if got both chopsticks)
            std::cout << "🍝 Philosopher " << philosopher_id << ": EATING! 🥢🥢" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(300));

            // STEP 6: Put down chopsticks (reverse order)
            // signal(chopstick[i]): Put down left chopstick
            std::cout << "🥢 Philosopher " << philosopher_id << ": Putting down right chopstick"
                      << right_chopstick << std::endl;
            chopsticks[right_chopstick].unlock();

            // signal(chopstick[(i + 1) % 5]): Put down right chopstick
            std::cout << "🥢 Philosopher " << philosopher_id << ": Putting down left chopstick"
                      << left_chopstick << std::endl;
            chopsticks[left_chopstick].unlock();

            std::cout << "🤤 Philosopher " << philosopher_id << ": Finished eating, back to thinking" << std::endl;
        }
    }

    // ========================================================================
    // DEADLOCK-FREE PHILOSOPHER (WITH SOLUTION)
    // ========================================================================
    void philosopher_deadlock_free(int philosopher_id) {
        int left_chopstick = philosopher_id;
        int right_chopstick = (philosopher_id + 1) % 5;

        while (dining) {
            // Think
            std::cout << "🤔 Philosopher " << philosopher_id << ": Thinking..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            std::cout << "🍽️ Philosopher " << philosopher_id << ": Getting hungry!" << std::endl;

            // SOLUTION 1: RESOURCE ORDERING
            // Always pick up chopstick with lower number first
            // This prevents circular wait condition!
            int first = std::min(left_chopstick, right_chopstick);
            int second = std::max(left_chopstick, right_chopstick);

            std::cout << "🥢 Philosopher " << philosopher_id << ": Picking up chopstick "
                      << first << " (lower number first)" << std::endl;
            chopsticks[first].lock();

            std::cout << "🥢 Philosopher " << philosopher_id << ": Picking up chopstick "
                      << second << " (higher number second)" << std::endl;
            chopsticks[second].lock();

            std::cout << "🍝 Philosopher " << philosopher_id << ": EATING! 🥢🥢" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(300));

            // Put down in reverse order
            chopsticks[second].unlock();
            chopsticks[first].unlock();

            std::cout << "🤤 Philosopher " << philosopher_id << ": Finished eating" << std::endl;

            break; // Exit after one meal for demo
        }
    }

    void stop_dining() { dining = false; }
};

// ============================================================================
// MAIN FUNCTION - DEMONSTRATE ALL PROBLEMS
// ============================================================================

int main() {
    std::cout << "========================================================================\n";
    std::cout << "SYNCHRONIZATION PROBLEMS EXPLAINED FOR ENGINEERS\n";
    std::cout << "========================================================================\n\n";

    // =========================================================================
    // DEMO 1: LCM PROBLEM (Larry, Curley, Moe)
    // =========================================================================
    std::cout << "DEMO 1: LCM FARMING PROBLEM\n";
    std::cout << "Larry digs, Curley plants, Moe fills - with limited resources!\n\n";

    LCM_Farming farm;
    std::thread larry(&LCM_Farming::larry_digger, &farm);
    std::thread curley(&LCM_Farming::curley_planter, &farm);
    std::thread moe(&LCM_Farming::moe_filler, &farm);

    larry.join();
    curley.join();
    moe.join();

    std::cout << "\n" << std::string(70, '=') << "\n\n";

    // =========================================================================
    // DEMO 2: PRODUCER-CONSUMER PROBLEM
    // =========================================================================
    std::cout << "DEMO 2: PRODUCER-CONSUMER BUFFER\n";
    std::cout << "Producers create items, consumers process them!\n\n";

    ProducerConsumer pc;
    std::thread producer1(&ProducerConsumer::producer, &pc, 1);
    std::thread producer2(&ProducerConsumer::producer, &pc, 2);
    std::thread consumer1(&ProducerConsumer::consumer, &pc, 1);
    std::thread consumer2(&ProducerConsumer::consumer, &pc, 2);

    producer1.join();
    producer2.join();
    consumer1.join();
    consumer2.join();

    std::cout << "\n" << std::string(70, '=') << "\n\n";

    // =========================================================================
    // DEMO 3: DINING PHILOSOPHERS (DEADLOCK-FREE VERSION)
    // =========================================================================
    std::cout << "DEMO 3: DINING PHILOSOPHERS (DEADLOCK-FREE VERSION)\n";
    std::cout << "Philosophers eating with resource ordering solution!\n\n";

    DiningPhilosophers table;
    std::vector<std::thread> philosophers;

    // Start philosophers (using deadlock-free version)
    for (int i = 0; i < 5; i++) {
        philosophers.emplace_back(&DiningPhilosophers::philosopher_deadlock_free, &table, i);
    }

    for (auto& p : philosophers) {
        p.join();
    }

    std::cout << "\n========================================================================\n";
    std::cout << "ALL DEMONSTRATIONS COMPLETE!\n";
    std::cout << "========================================================================\n";

    return 0;
}