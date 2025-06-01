#ifndef PROGRESS_BAR_H
#define PROGRESS_BAR_H

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

class ProgressBar {
private:
    int total;
    int current;
    int bar_width;
    std::string description;
    std::chrono::steady_clock::time_point start_time;
    
    // Fun messages to keep user entertained
    std::vector<std::string> fun_messages = {
        ">> Optimizing matrix multiplication...",
        ">> Crunching numbers like a boss...",
        ">> Unleashing the power of sparse matrices...",
        ">> Hitting those cache lines perfectly...",
        ">> TCSC optimization in full swing...",
        ">> Speeding through calculations...",
        ">> Matrix circus in progress...",
        ">> Creating performance magic...",
        ">> Racing through sparse computations...",
        ">> Painting performance improvements...",
        ">> Dancing through the data...",
        ">> Serving hot optimizations...",
        ">> Level up: Matrix Multiplication Pro...",
        ">> The great TCSC performance show...",
        ">> Bullseye! Optimizations on target..."
    };
    
public:
    ProgressBar(int total_steps, const std::string& desc = "Progress", int width = 50) 
        : total(total_steps), current(0), bar_width(width), description(desc) {
        start_time = std::chrono::steady_clock::now();
    }
    
    void update(int step) {
        current = step;
        float progress = (float)current / total;
        int pos = bar_width * progress;
        
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
        auto eta = (elapsed / progress) - elapsed;
        
        std::cout << "\r" << description << " [";
        for (int i = 0; i < bar_width; ++i) {
            if (i < pos) std::cout << "█";
            else if (i == pos) std::cout << "▓";
            else std::cout << "░";
        }
        std::cout << "] " << int(progress * 100.0) << "% ";
        std::cout << "(" << current << "/" << total << ") ";
        std::cout << "Time: " << elapsed << "s ";
        if (progress > 0.05) {
            std::cout << "ETA: " << (int)eta << "s ";
        }
        
        // Show fun message occasionally
        if (current % (total / 10 + 1) == 0 && current > 0) {
            int msg_idx = (current / (total / 10 + 1)) % fun_messages.size();
            std::cout << " " << fun_messages[msg_idx];
        }
        
        std::cout.flush();
    }
    
    void finish() {
        update(total);
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
        std::cout << "\n*** Completed in " << elapsed << " seconds! ***\n";
    }
    
    static void show_thinking_animation(const std::string& message, int duration_ms = 2000) {
        std::vector<std::string> spinners = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
        auto start = std::chrono::steady_clock::now();
        int i = 0;
        
        while (std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count() < duration_ms) {
            std::cout << "\r" << spinners[i % spinners.size()] << " " << message << "     ";
            std::cout.flush();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            i++;
        }
        std::cout << "\r[OK] " << message << " - Done!                    \n";
    }
};

#endif
