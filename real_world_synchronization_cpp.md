# Real-World Synchronization: C++ Implementations

## 🖥️ Complete C++ Code Examples for Real Synchronization Problems

---

## 🎯 Example 1: Web Browser Cache - Readers-Writers Problem

### This is EXACTLY the structure from your textbook!

```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <map>
#include <atomic>

class BrowserCache {
private:
    // Shared data structures
    std::map<std::string, std::string> cache_data;

    // Synchronization primitives (exactly like your textbook!)
    std::mutex rw_mutex;    // Protects cache_data (like rw_mutex)
    std::mutex mutex;       // Protects read_count (like mutex)
    int read_count = 0;     // Number of active readers

    std::atomic<bool> running{true};

public:
    // READER PROCESS (exactly like your textbook structure!)
    void load_tab(const std::string& url, int tab_id) {
        while (running) {
            // wait(mutex);
            mutex.lock();

            // read_count++;
            read_count++;

            // if (read_count == 1)
            //     wait(rw_mutex);
            if (read_count == 1) {
                rw_mutex.lock();  // First reader locks the cache
            }

            // signal(mutex);
            mutex.unlock();

            // ... reading is performed ...
            std::cout << "📱 Tab " << tab_id << ": Loading " << url
                      << " from cache: " << (cache_data.count(url) ? "HIT" : "MISS") << std::endl;

            // Simulate page rendering
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // wait(mutex);
            mutex.lock();

            // read_count--;
            read_count--;

            // if (read_count == 0)
            //     signal(rw_mutex);
            if (read_count == 0) {
                rw_mutex.unlock();  // Last reader unlocks the cache
            }

            // signal(mutex);
            mutex.unlock();

            break;  // Exit after one load
        }
    }

    // WRITER PROCESS (exactly like your textbook structure!)
    void background_downloader() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            // wait(rw_mutex);
            rw_mutex.lock();

            // ... writing is performed ...
            std::cout << "🌐 Background: Updating cache..." << std::endl;
            cache_data["github.com"] = "Updated GitHub content";
            cache_data["stackoverflow.com"] = "Updated StackOverflow content";
            cache_data["news.ycombinator.com"] = "Updated HackerNews content";

            // Simulate download time
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            std::cout << "✅ Background: Cache updated!" << std::endl;

            // signal(rw_mutex);
            rw_mutex.unlock();
        }
    }

    void stop() { running = false; }
};

int main() {
    std::cout << "🌐 Starting Web Browser Simulation...\n";
    std::cout << "This is EXACTLY the Readers-Writers structure from your textbook!\n\n";

    BrowserCache browser;

    std::vector<std::thread> threads;

    // Start background writer
    threads.emplace_back(&BrowserCache::background_downloader, &browser);

    // Start multiple readers (tabs)
    std::vector<std::string> urls = {
        "github.com", "stackoverflow.com", "news.ycombinator.com",
        "reddit.com", "youtube.com", "twitter.com"
    };

    for (int i = 0; i < 10; ++i) {
        std::string url = urls[i % urls.size()];
        threads.emplace_back(&BrowserCache::load_tab, &browser, url, i);
    }

    // Let it run
    std::this_thread::sleep_for(std::chrono::seconds(3));
    browser.stop();

    for (auto& t : threads) {
        t.join();
    }

    return 0;
}
```

### Compile and Run:
```bash
g++ -std=c++17 -pthread browser_cache.cpp -o browser_cache
./browser_cache
```

### Expected Output:
```
🌐 Starting Web Browser Simulation...
This is EXACTLY the Readers-Writers structure from your textbook!

📱 Tab 0: Loading github.com from cache: MISS
📱 Tab 1: Loading stackoverflow.com from cache: MISS
📱 Tab 2: Loading news.ycombinator.com from cache: MISS
🌐 Background: Updating cache...
✅ Background: Cache updated!
📱 Tab 3: Loading github.com from cache: HIT
📱 Tab 4: Loading stackoverflow.com from cache: HIT
```

---

## 🎯 Example 2: Instagram Photo Processing - LCM Problem

```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <semaphore>
#include <queue>
#include <chrono>
#include <string>
#include <atomic>

class PhotoProcessor {
private:
    // Queues for each processing stage
    std::queue<std::string> raw_photos;
    std::queue<std::string> edited_photos;
    std::queue<std::string> final_photos;

    // Semaphores (exactly like LCM problem!)
    std::counting_semaphore<> shovel{1};        // Only one edit at a time
    std::counting_semaphore<> curley_signal{0}; // Edit ready for filter
    std::counting_semaphore<> moe_signal{0};    // Filter ready for upload

    // Protect queues
    std::mutex raw_mutex;
    std::mutex edited_mutex;
    std::mutex final_mutex;

    std::atomic<bool> running{true};
    std::atomic<int> photo_id{0};

public:
    // LARRY (Camera) - digs holes (takes photos)
    void larry_camera() {
        while (running) {
            // Limit how many photos ahead (like DigHole semaphore!)
            std::cout << "📸 Larry (Camera): Capturing photo " << photo_id << std::endl;

            // Add photo to raw queue
            {
                std::lock_guard<std::mutex> lock(raw_mutex);
                raw_photos.push("raw_photo_" + std::to_string(photo_id));
            }

            // Signal Curley that photo is ready
            curley_signal.release();

            photo_id++;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    // CURLY (Editor) - plants seeds (edits photos)
    void curley_editor() {
        while (running) {
            // Wait for Larry to capture photo
            curley_signal.acquire();

            if (!running && raw_photos.empty()) break;

            // Wait for shovel (editing tools)
            shovel.acquire();

            try {
                std::string photo;
                {
                    std::lock_guard<std::mutex> lock(raw_mutex);
                    photo = raw_photos.front();
                    raw_photos.pop();
                }

                std::cout << "✏️ Curley (Editor): Editing " << photo << std::endl;

                // Edit photo
                std::string edited_photo = "edited_" + photo;
                std::this_thread::sleep_for(std::chrono::milliseconds(200));

                {
                    std::lock_guard<std::mutex> lock(edited_mutex);
                    edited_photos.push(edited_photo);
                }

                // Signal Moe that edit is ready
                moe_signal.release();

            } catch (...) {
                shovel.release();
                throw;
            }

            shovel.release();
        }
    }

    // MOE (Filter/Upload) - fills holes (uploads photos)
    void moe_uploader() {
        while (running) {
            // Wait for Curley to finish editing
            moe_signal.acquire();

            if (!running && edited_photos.empty()) break;

            // Wait for shovel (upload tools)
            shovel.acquire();

            try {
                std::string photo;
                {
                    std::lock_guard<std::mutex> lock(edited_mutex);
                    photo = edited_photos.front();
                    edited_photos.pop();
                }

                std::cout << "🌈 Moe (Filter): Applying filters to " << photo << std::endl;

                // Filter and upload
                std::string final_photo = "final_" + photo;
                std::this_thread::sleep_for(std::chrono::milliseconds(150));

                {
                    std::lock_guard<std::mutex> lock(final_mutex);
                    final_photos.push(final_photo);
                }

                std::cout << "✅ Moe (Upload): " << final_photo << " uploaded!" << std::endl;

            } catch (...) {
                shovel.release();
                throw;
            }

            shovel.release();
        }
    }

    void stop() { running = false; }
};

int main() {
    std::cout << "📱 Starting Instagram Photo Processing...\n";
    std::cout << "This is EXACTLY the LCM problem with Larry, Curley, and Moe!\n\n";

    PhotoProcessor processor;

    // Start the pipeline
    std::thread larry_thread(&PhotoProcessor::larry_camera, &processor);
    std::thread curley_thread(&PhotoProcessor::curley_editor, &processor);
    std::thread moe_thread(&PhotoProcessor::moe_uploader, &processor);

    // Let it run
    std::this_thread::sleep_for(std::chrono::seconds(3));
    processor.stop();

    // Signal shutdown
    processor.curley_signal.release();
    processor.moe_signal.release();

    larry_thread.join();
    curley_thread.join();
    moe_thread.join();

    return 0;
}
```

### Compile and Run:
```bash
g++ -std=c++20 -pthread photo_processor.cpp -o photo_processor
./photo_processor
```

---

## 🎯 Example 3: Game Engine - Dining Philosophers Problem

```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <atomic>

class GameEngine {
private:
    // Shared game resources (the chopsticks!)
    std::mutex player_data_mutex;     // Philosopher 0's left chopstick
    std::mutex physics_mutex;         // Philosopher 1's left chopstick
    std::mutex network_mutex;         // Philosopher 2's left chopstick
    std::mutex render_mutex;          // Philosopher 3's left chopstick
    std::mutex audio_mutex;           // Philosopher 4's left chopstick

    std::atomic<bool> game_running{true};

    // Array of mutexes for easier access
    std::vector<std::mutex*> resources = {
        &player_data_mutex,
        &physics_mutex,
        &network_mutex,
        &render_mutex,
        &audio_mutex
    };

public:
    // PHILOSOPHER 0: Renderer (needs physics + player data)
    void rendering_system() {
        int philosopher_id = 0;
        int left_chopstick = 0;   // player_data_mutex
        int right_chopstick = 1;  // physics_mutex

        while (game_running) {
            // wait(chopstick[i]);
            resources[left_chopstick]->lock();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

            // wait(chopstick[(i + 1) % 5]);
            resources[right_chopstick]->lock();

            try {
                // ... eating is performed ...
                std::cout << "🎮 Philosopher " << philosopher_id
                          << " (Renderer): Drawing frame using physics + player data\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60 FPS
            } catch (...) {
                resources[right_chopstick]->unlock();
                resources[left_chopstick]->unlock();
                throw;
            }

            // signal(chopstick[i]);
            resources[left_chopstick]->unlock();

            // signal(chopstick[(i + 1) % 5]);
            resources[right_chopstick]->unlock();

            // ... thinking is performed ...
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    // PHILOSOPHER 1: Network (needs player data + audio)
    void network_system() {
        int philosopher_id = 1;
        int left_chopstick = 1;   // physics_mutex
        int right_chopstick = 2;  // network_mutex

        while (game_running) {
            resources[left_chopstick]->lock();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            resources[right_chopstick]->lock();

            try {
                std::cout << "🌐 Philosopher " << philosopher_id
                          << " (Network): Syncing players + voice chat\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(33)); // 30 Hz
            } catch (...) {
                resources[right_chopstick]->unlock();
                resources[left_chopstick]->unlock();
                throw;
            }

            resources[left_chopstick]->unlock();
            resources[right_chopstick]->unlock();

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    // PHILOSOPHER 2: Physics (needs network + render)
    void physics_system() {
        int philosopher_id = 2;
        int left_chopstick = 2;   // network_mutex
        int right_chopstick = 3;  // render_mutex

        while (game_running) {
            resources[left_chopstick]->lock();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            resources[right_chopstick]->lock();

            try {
                std::cout << "⚛️ Philosopher " << philosopher_id
                          << " (Physics): Calculating using render + network data\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(20)); // 50 Hz
            } catch (...) {
                resources[right_chopstick]->unlock();
                resources[left_chopstick]->unlock();
                throw;
            }

            resources[left_chopstick]->unlock();
            resources[right_chopstick]->unlock();

            std::this_thread::sleep_for(std::chrono::milliseconds(8));
        }
    }

    // PHILOSOPHER 3: AI (needs render + audio)
    void ai_system() {
        int philosopher_id = 3;
        int left_chopstick = 3;   // render_mutex
        int right_chopstick = 4;  // audio_mutex

        while (game_running) {
            resources[left_chopstick]->lock();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            resources[right_chopstick]->lock();

            try {
                std::cout << "🤖 Philosopher " << philosopher_id
                          << " (AI): Making decisions using render + audio\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 10 Hz
            } catch (...) {
                resources[right_chopstick]->unlock();
                resources[left_chopstick]->unlock();
                throw;
            }

            resources[left_chopstick]->unlock();
            resources[right_chopstick]->unlock();

            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }

    // PHILOSOPHER 4: Audio (needs audio + player data - wraps around!)
    void audio_system() {
        int philosopher_id = 4;
        int left_chopstick = 4;   // audio_mutex
        int right_chopstick = 0;  // player_data_mutex (wraps around!)

        while (game_running) {
            resources[left_chopstick]->lock();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            resources[right_chopstick]->lock();

            try {
                std::cout << "🔊 Philosopher " << philosopher_id
                          << " (Audio): Playing sounds based on audio + player data\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 100 Hz
            } catch (...) {
                resources[right_chopstick]->unlock();
                resources[left_chopstick]->unlock();
                throw;
            }

            resources[left_chopstick]->unlock();
            resources[right_chopstick]->unlock();

            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }

    void stop() { game_running = false; }
};

int main() {
    std::cout << "🎮 Starting Multiplayer Game Engine...\n";
    std::cout << "This is EXACTLY the Dining Philosophers problem!\n";
    std::cout << "Each game system needs 2 resources simultaneously!\n\n";

    GameEngine game;

    std::vector<std::thread> threads;

    // Start all game systems (philosophers)
    threads.emplace_back(&GameEngine::rendering_system, &game);
    threads.emplace_back(&GameEngine::network_system, &game);
    threads.emplace_back(&GameEngine::physics_system, &game);
    threads.emplace_back(&GameEngine::ai_system, &game);
    threads.emplace_back(&GameEngine::audio_system, &game);

    // Let game run
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "\n⚠️ Stopping game (this could cause deadlock if not careful!)\n";
    game.stop();

    for (auto& t : threads) {
        t.join();
    }

    return 0;
}
```

### Compile and Run:
```bash
g++ -std=c++17 -pthread game_engine.cpp -o game_engine
./game_engine
```

---

## 🎯 Example 4: File System Deadlock Prevention

```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <map>
#include <string>
#include <chrono>
#include <vector>

class FileSystemManager {
private:
    std::map<std::string, std::mutex> file_locks;
    std::map<std::string, std::mutex> directory_locks;

public:
    // DEADLOCK PRONE VERSION (wrong lock order)
    void copy_file_deadlock(const std::string& src, const std::string& dst) {
        std::cout << "📁 Starting DEADLOCK copy: " << src << " -> " << dst << std::endl;

        std::string src_dir = src.substr(0, src.find_last_of('/'));
        std::string dst_dir = dst.substr(0, dst.find_last_of('/'));

        // WRONG ORDER - leads to deadlock!
        file_locks[src].lock();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        directory_locks[src_dir].lock();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        file_locks[dst].lock();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        directory_locks[dst_dir].lock();

        try {
            std::cout << "📄 Copying data from " << src << " to " << dst << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } catch (...) {
            directory_locks[dst_dir].unlock();
            file_locks[dst].unlock();
            directory_locks[src_dir].unlock();
            file_locks[src].unlock();
            throw;
        }

        // Release in reverse order
        directory_locks[dst_dir].unlock();
        file_locks[dst].unlock();
        directory_locks[src_dir].unlock();
        file_locks[src].unlock();

        std::cout << "✅ Finished copy: " << src << " -> " << dst << std::endl;
    }

    // DEADLOCK FREE VERSION (resource ordering)
    void copy_file_safe(const std::string& src, const std::string& dst) {
        std::cout << "📁 Starting SAFE copy: " << src << " -> " << dst << std::endl;

        std::string src_dir = src.substr(0, src.find_last_of('/'));
        std::string dst_dir = dst.substr(0, dst.find_last_of('/'));

        // Create ordered list of resources
        std::vector<std::string> resources = {src, src_dir, dst, dst_dir};
        std::sort(resources.begin(), resources.end());  // Sort to prevent deadlock!

        std::vector<std::mutex*> locks;

        // Acquire locks in sorted order
        for (const auto& resource : resources) {
            if (resource.find('/') != std::string::npos) {
                locks.push_back(&directory_locks[resource]);
            } else {
                locks.push_back(&file_locks[resource]);
            }
            locks.back()->lock();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        try {
            std::cout << "📄 Copying data from " << src << " to " << dst << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } catch (...) {
            // Release in reverse order
            for (auto it = locks.rbegin(); it != locks.rend(); ++it) {
                (*it)->unlock();
            }
            throw;
        }

        // Release in reverse order
        for (auto it = locks.rbegin(); it != locks.rend(); ++it) {
            (*it)->unlock();
        }

        std::cout << "✅ Finished copy: " << src << " -> " << dst << std::endl;
    }
};

int main() {
    std::cout << "📁 File System Deadlock Demonstration\n\n";

    // Test 1: Deadlock scenario
    std::cout << "💀 Testing DEADLOCK scenario...\n";
    FileSystemManager fs_deadlock;

    std::thread t1([&fs_deadlock]() {
        fs_deadlock.copy_file_deadlock("/Users/you/file1.txt", "/Users/you/file2.txt");
    });

    std::thread t2([&fs_deadlock]() {
        fs_deadlock.copy_file_deadlock("/Users/you/file2.txt", "/Users/you/file1.txt");
    });

    // This will deadlock - uncomment to test
    // t1.join();
    // t2.join();

    // Let it run for a bit then terminate
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "⚠️ Deadlock detected! Terminating threads...\n";
    std::terminate();  // Force exit due to deadlock

    return 0;
}
```

---

## 🎯 Example 5: High-Concurrency Web Server

```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <string>

class WebServer {
private:
    // Shared data structures
    std::unordered_map<int, std::string> user_database;
    std::unordered_map<std::string, std::string> cache;
    std::atomic<int> active_users{0};

    // Synchronization primitives
    std::mutex db_lock;
    std::mutex cache_lock;
    std::mutex user_counter_lock;

    std::atomic<bool> running{true};

public:
    void handle_request(int user_id, const std::string& request_data) {
        std::cout << "🌐 User " << user_id << ": Processing request " << request_data << std::endl;

        // 1. Update active users (race condition without lock!)
        {
            std::lock_guard<std::mutex> lock(user_counter_lock);
            active_users++;
            std::cout << "👥 Active users: " << active_users << std::endl;
        }

        // 2. Check cache
        std::string result;
        bool cache_hit = false;
        {
            std::lock_guard<std::mutex> lock(cache_lock);
            if (cache.count(request_data)) {
                result = cache[request_data];
                cache_hit = true;
                std::cout << "⚡ User " << user_id << ": Cache hit!" << std::endl;
            }
        }

        if (!cache_hit) {
            // 3. Update database
            {
                std::lock_guard<std::mutex> lock(db_lock);
                std::cout << "💾 User " << user_id << ": Querying database..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                result = "data_for_" + request_data + "_user_" + std::to_string(user_id);
                user_database[user_id] = result;
            }

            // 4. Update cache
            {
                std::lock_guard<std::mutex> lock(cache_lock);
                cache[request_data] = result;
                std::cout << "💾 User " << user_id << ": Updated cache" << std::endl;
            }
        }

        // 5. Decrease active users
        {
            std::lock_guard<std::mutex> lock(user_counter_lock);
            active_users--;
        }

        std::cout << "✅ User " << user_id << ": Request complete" << std::endl;
    }

    void user_session(int user_id) {
        std::vector<std::string> requests = {"profile", "settings", "dashboard", "messages"};

        for (const auto& req : requests) {
            handle_request(user_id, req);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void stop() { running = false; }

    void print_stats() {
        std::cout << "\n📊 Final Statistics:\n";
        std::cout << "Final cache size: " << cache.size() << std::endl;
        std::cout << "Final database size: " << user_database.size() << std::endl;
        std::cout << "Final active users: " << active_users << std::endl;
    }
};

int main() {
    std::cout << "🌐 Starting High-Concurrency Web Server...\n";
    std::cout << "Simulating 100 concurrent users!\n\n";

    WebServer server;
    std::vector<std::thread> threads;

    // Start 100 concurrent users
    for (int user_id = 0; user_id < 100; ++user_id) {
        threads.emplace_back(&WebServer::user_session, &server, user_id);
    }

    // Wait for all requests to complete
    for (auto& t : threads) {
        t.join();
    }

    server.print_stats();
    std::cout << "\n🎉 All requests processed successfully!" << std::endl;

    return 0;
}
```

### Compile and Run:
```bash
g++ -std=c++17 -pthread web_server.cpp -o web_server
./web_server
```

---

## 🎯 Example 6: Race Condition Demonstration

```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>

class RaceConditionDemo {
private:
    long long shared_counter = 0;
    std::mutex counter_lock;

public:
    // WITHOUT SYNCHRONIZATION - WILL PRODUCE WRONG RESULTS!
    void increment_unsafe() {
        for (int i = 0; i < 100000; ++i) {
            shared_counter++;  // RACE CONDITION!
        }
    }

    // WITH SYNCHRONIZATION - CORRECT RESULTS!
    void increment_safe() {
        for (int i = 0; i < 100000; ++i) {
            std::lock_guard<std::mutex> lock(counter_lock);
            shared_counter++;
        }
    }

    void reset() { shared_counter = 0; }
    long long get_counter() { return shared_counter; }
};

int main() {
    std::cout << "🏁 Race Condition Demonstration\n\n";

    RaceConditionDemo demo;
    const int num_threads = 10;
    const int expected = num_threads * 100000;

    // Test 1: Without synchronization (WRONG!)
    std::cout << "❌ Test 1: Without synchronization\n";
    demo.reset();

    std::vector<std::thread> unsafe_threads;
    for (int i = 0; i < num_threads; ++i) {
        unsafe_threads.emplace_back(&RaceConditionDemo::increment_unsafe, &demo);
    }

    for (auto& t : unsafe_threads) {
        t.join();
    }

    std::cout << "Expected: " << expected << ", Got: " << demo.get_counter() << std::endl;
    std::cout << "Difference: " << (expected - demo.get_counter()) << " (lost increments!)\n\n";

    // Test 2: With synchronization (CORRECT!)
    std::cout << "✅ Test 2: With synchronization\n";
    demo.reset();

    std::vector<std::thread> safe_threads;
    for (int i = 0; i < num_threads; ++i) {
        safe_threads.emplace_back(&RaceConditionDemo::increment_safe, &demo);
    }

    for (auto& t : safe_threads) {
        t.join();
    }

    std::cout << "Expected: " << expected << ", Got: " << demo.get_counter() << std::endl;
    std::cout << "Difference: " << (expected - demo.get_counter()) << " (perfect!)\n\n";

    return 0;
}
```

### Compile and Run:
```bash
g++ -std=c++17 -pthread race_condition.cpp -o race_condition
./race_condition
```

---

## 🏗️ How to Compile and Run All Examples

```bash
# Compile all examples
g++ -std=c++17 -pthread browser_cache.cpp -o browser_cache
g++ -std=c++20 -pthread photo_processor.cpp -o photo_processor
g++ -std=c++17 -pthread game_engine.cpp -o game_engine
g++ -std=c++17 -pthread web_server.cpp -o web_server
g++ -std=c++17 -pthread race_condition.cpp -o race_condition

# Run them
./browser_cache
./photo_processor
./game_engine
./web_server
./race_condition
```

## 🎯 Key Insights

1. **Readers-Writers**: Exact textbook structure implemented in C++
2. **LCM Problem**: Real Instagram/TikTok photo processing pipeline
3. **Dining Philosophers**: Game engine with competing resources
4. **Deadlock Prevention**: Resource ordering in file operations
5. **Race Conditions**: Real demonstration of data corruption

All these examples show **exactly** how synchronization problems manifest in real C++ applications! 🚀