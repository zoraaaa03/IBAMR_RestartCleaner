#include "restart_cleaner_standalone.h"
#include <iostream>

/**
 * Function: show_usage
 * Purpose: Display usage information
 */
void show_usage(const char* program_name) {
    std::cout << "IBAMR Restart Cleanup Tool" << std::endl;
    std::cout << "Usage: " << program_name << " --recent N <restart_dir> [--dry-run]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --recent N     Keep the N most recent restore directories" << std::endl;
    std::cout << std::endl;
    std::cout << "Flags:" << std::endl;
    std::cout << "  --dry-run      Preview mode - show what would be deleted without actual deletion" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << program_name << " --recent 5 ./restart_IB2d" << std::endl;
    std::cout << "  " << program_name << " --recent 3 ./restart_IB2d --dry-run" << std::endl;
}

/**
 * Function: main
 * Purpose: Program entry point, handles command line arguments
 */
int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc < 4 || argc > 5) {
        show_usage(argv[0]);
        return 1;
    }
    
    std::string option = argv[1];
    std::string restart_dir = argv[3];
    
    // Only support --recent for now
    if (option != "--recent") {
        std::cerr << "Error: Currently only --recent option is supported." << std::endl;
        show_usage(argv[0]);
        return 1;
    }
    
    // Check for dry-run flag
    bool dry_run = false;
    if (argc == 5) {
        if (std::string(argv[4]) == "--dry-run") {
            dry_run = true;
        } else {
            std::cerr << "Error: Unknown flag '" << argv[4] << "'." << std::endl;
            show_usage(argv[0]);
            return 1;
        }
    }
    
    // Parse the number parameter
    int keep_count;
    try {
        keep_count = std::stoi(argv[2]);
    } catch (const std::invalid_argument&) {
        std::cerr << "Error: '" << argv[2] << "' is not a valid number." << std::endl;
        return 1;
    } catch (const std::out_of_range&) {
        std::cerr << "Error: Number '" << argv[2] << "' is out of range." << std::endl;
        return 1;
    }
    
    // Validate keep_count
    if (keep_count <= 0) {
        std::cerr << "Error: Keep count must be positive, got " << keep_count << std::endl;
        return 1;
    }
    
    try {
        // Create RestartCleaner and run cleanup
        RestartCleaner cleaner(restart_dir, keep_count, "KEEP_RECENT_N", dry_run);
        cleaner.cleanup();
        
        // Show final results
        auto remaining = cleaner.getAvailableIterations();
        std::cout << "\nFinal result: " << remaining.size() << " directories remaining." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}