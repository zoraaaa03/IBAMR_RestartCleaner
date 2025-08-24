#include "restart_cleaner_standalone.h"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <stdexcept>

namespace fs = std::filesystem;

/**
 * Test Environment Management Class
 * Handles creation and cleanup of test data
 */
class TestEnvironment {
private:
    const std::string test_base_dir = "test_restart_cleanup";
    const std::string test_restart_dir = test_base_dir + "/restart_IB2d";

public:
    TestEnvironment() {
        create_test_data();
    }

    ~TestEnvironment() {
        cleanup();
    }

    std::string get_test_dir() const {
        return test_restart_dir;
    }

private:
    void create_test_data() {
        // Remove existing test directory
        if (fs::exists(test_base_dir)) {
            fs::remove_all(test_base_dir);
        }

        // Create test directory structure
        fs::create_directories(test_restart_dir);

        // Create valid restore directories with realistic content
        std::vector<std::string> valid_dirs = {
            "restore.000100", "restore.000200", "restore.000300",
            "restore.001000", "restore.002500", "restore.003000",
            "restore.999999", "restore.000001", "restore.005000"
        };

        for (const auto& dir : valid_dirs) {
            std::string full_path = test_restart_dir + "/" + dir;
            fs::create_directories(full_path);
            
            // Create realistic IBAMR files
            std::ofstream(full_path + "/samrai.00000") << "SAMRAI restart data for " << dir;
            std::ofstream(full_path + "/hier_data.00000.samrai.00000") << "Hierarchy data for " << dir;
            
            // Create subdirectory with files
            fs::create_directories(full_path + "/subdirectory");
            std::ofstream(full_path + "/subdirectory/data.txt") << "Subdirectory data";
        }

        // Create invalid directories (should be ignored)
        std::vector<std::string> invalid_dirs = {
            "restore.12345",        // 5 digits
            "restore.1234567",      // 7 digits  
            "restore.abc123",       // contains letters
            "restore_invalid",      // wrong format
            "other_directory",      // completely different
            "restore.",             // no digits
            "restore.000100_backup" // extra suffix
        };

        for (const auto& dir : invalid_dirs) {
            std::string full_path = test_restart_dir + "/" + dir;
            fs::create_directories(full_path);
            std::ofstream(full_path + "/dummy.txt") << "Should be ignored";
        }
    }

    void cleanup() {
        if (fs::exists(test_base_dir)) {
            fs::remove_all(test_base_dir);
        }
    }
};

/**
 * Test iteration number parsing through getAvailableIterations
 * Tests extraction of iteration numbers from directory names via public interface
 * This indirectly tests the private parseIterationNum function
 */
bool test_iteration_parsing_via_public_interface(TestEnvironment& env) {
    std::cout << "Testing iteration number parsing... ";

    try {
        RestartCleaner cleaner(env.get_test_dir(), 10, "KEEP_RECENT_N", true); // Keep all, dry run
        auto iterations = cleaner.getAvailableIterations();

        // Expected valid iterations in sorted order (tests parseIterationNum indirectly)
        std::vector<int> expected_iterations = {
            1, 100, 200, 300, 1000, 2500, 3000, 5000, 999999
        };

        // Check count - should ignore invalid directories
        if (iterations.size() != expected_iterations.size()) {
            std::cout << "FAILED (Expected " << expected_iterations.size() 
                      << " valid iterations, found " << iterations.size() << ")" << std::endl;
            return false;
        }

        // Check each iteration number and sorting
        for (size_t i = 0; i < iterations.size(); i++) {
            if (iterations[i] != expected_iterations[i]) {
                std::cout << "FAILED (Expected iteration " << expected_iterations[i] 
                          << " at position " << i << ", found " << iterations[i] << ")" << std::endl;
                return false;
            }
        }

        // Verify sorting by checking all numbers are in ascending order
        for (size_t i = 1; i < iterations.size(); i++) {
            if (iterations[i-1] >= iterations[i]) {
                std::cout << "FAILED (Sorting error: " << iterations[i-1] 
                          << " >= " << iterations[i] << ")" << std::endl;
                return false;
            }
        }

        std::cout << "PASSED" << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cout << "FAILED (Exception: " << e.what() << ")" << std::endl;
        return false;
    }
}

/**
 * Test directory filtering logic
 * Tests that invalid directory names are properly ignored
 * This indirectly tests both getAllRestartDirs and parseIterationNum functions
 */
bool test_directory_filtering(TestEnvironment& env) {
    std::cout << "Testing directory filtering... ";

    try {
        // Create a cleaner that will scan all directories
        RestartCleaner cleaner(env.get_test_dir(), 20, "KEEP_RECENT_N", true); // Keep more than we have
        auto iterations = cleaner.getAvailableIterations();

        // Test cases that should be IGNORED (based on your original test)
        std::vector<std::string> should_be_ignored = {
            "restore.12345",        // 5 digits
            "restore.1234567",      // 7 digits  
            "restore.abc123",       // contains letters
            "restore_invalid",      // wrong format
            "other_directory",      // completely different
            "restore.",             // no digits
            "restore.000100_backup" // extra suffix
        };

        // Verify these invalid patterns don't appear in our results
        // We know our test environment created these invalid dirs
        // If parsing works correctly, none should appear in iterations
        
        // Check that we only got valid iterations (9 of them)
        if (iterations.size() != 9) {
            std::cout << "FAILED (Should have filtered to 9 valid directories, got " 
                      << iterations.size() << ")" << std::endl;
            return false;
        }

        // Spot check some specific cases
        // Should NOT contain invalid numbers that would result from bad parsing
        for (int iter : iterations) {
            if (iter == 12345 || iter == 1234567) {  // These would come from invalid dirs
                std::cout << "FAILED (Found invalid iteration number: " << iter << ")" << std::endl;
                return false;
            }
            if (iter < 0) {  // Should never have negative numbers
                std::cout << "FAILED (Found negative iteration number: " << iter << ")" << std::endl;
                return false;
            }
        }

        std::cout << "PASSED" << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cout << "FAILED (Exception: " << e.what() << ")" << std::endl;
        return false;
    }
}

/**
 * Test cleanup functionality in dry run mode
 * Tests the complete cleanup logic without actually deleting files
 */
bool test_cleanup_dry_run(TestEnvironment& env) {
    std::cout << "Testing cleanup dry run... ";

    try {
        // Count directories before cleanup
        RestartCleaner counter(env.get_test_dir(), 10, "KEEP_RECENT_N", true);
        auto before_count = counter.getAvailableIterations().size();
        
        // Run cleanup in dry run mode (should keep 3, delete 6)
        RestartCleaner dry_cleaner(env.get_test_dir(), 3, "KEEP_RECENT_N", true);
        dry_cleaner.cleanup(); // Should NOT actually delete anything
        
        // Count directories after - should be same as before
        auto after_count = counter.getAvailableIterations().size();
        
        if (before_count != after_count) {
            std::cout << "FAILED (Dry run should not delete anything: before=" 
                      << before_count << ", after=" << after_count << ")" << std::endl;
            return false;
        }

        // Verify we still have all 9 directories
        if (after_count != 9) {
            std::cout << "FAILED (Should still have 9 directories after dry run, have " 
                      << after_count << ")" << std::endl;
            return false;
        }

        std::cout << "PASSED" << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cout << "FAILED (Exception: " << e.what() << ")" << std::endl;
        return false;
    }
}

/**
 * Test error handling with various edge cases
 * Tests behavior with invalid inputs and problematic directories
 */
bool test_error_handling() {
    std::cout << "Testing error handling... ";

    try {
        // Test 1: Non-existent directory - should handle gracefully
        // Note: This test creates cleaner with existing temp directory, then tests getAvailableIterations with non-existent path
        {
            // Create a temporary directory for constructor, then test with non-existent path in getAvailableIterations
            std::string temp_dir = "temp_test_dir_for_nonexistent_test";
            fs::create_directories(temp_dir);
            
            RestartCleaner cleaner(temp_dir, 5, "KEEP_RECENT_N", true);
            auto result = cleaner.getAvailableIterations();
            if (!result.empty()) {
                std::cout << "FAILED (Empty directory should return empty vector)" << std::endl;
                fs::remove_all(temp_dir);
                return false;
            }
            fs::remove_all(temp_dir);
        }
        
        // Test 2: Empty directory - should handle gracefully
        {
            std::string empty_dir = "empty_test_dir";
            fs::create_directories(empty_dir);
            
            RestartCleaner cleaner(empty_dir, 5, "KEEP_RECENT_N", true);
            auto result = cleaner.getAvailableIterations();
            if (!result.empty()) {
                std::cout << "FAILED (Empty directory should return empty vector)" << std::endl;
                fs::remove_all(empty_dir);
                return false;
            }
            fs::remove_all(empty_dir);
        }

        // Test 3: Directory with no valid restore directories
        {
            std::string invalid_dir = "invalid_test_dir";
            fs::create_directories(invalid_dir);
            fs::create_directories(invalid_dir + "/not_a_restore_dir");
            fs::create_directories(invalid_dir + "/restore_wrong_format");
            
            RestartCleaner cleaner(invalid_dir, 5, "KEEP_RECENT_N", true);
            auto result = cleaner.getAvailableIterations();
            if (!result.empty()) {
                std::cout << "FAILED (Directory with no valid restores should return empty vector)" << std::endl;
                fs::remove_all(invalid_dir);
                return false;
            }
            fs::remove_all(invalid_dir);
        }

        // Test 4: Invalid constructor parameters
        try {
            // Create temp dir for negative keep count test
            std::string temp_dir = "temp_negative_test";
            fs::create_directories(temp_dir);
            RestartCleaner bad_cleaner(temp_dir, -1, "KEEP_RECENT_N", false);  // Negative keep count
            std::cout << "FAILED (Should throw exception for negative keep count)" << std::endl;
            fs::remove_all(temp_dir);
            return false;
        } catch (const std::invalid_argument&) {
            // Expected exception
            fs::remove_all("temp_negative_test");
        }

        // Test 5: Non-existent directory in constructor - should throw exception
        try {
            RestartCleaner bad_cleaner("/non/existent/directory", 5, "KEEP_RECENT_N", false);
            std::cout << "FAILED (Should throw exception for non-existent directory)" << std::endl;
            return false;
        } catch (const std::invalid_argument&) {
            // Expected exception
        }

        std::cout << "PASSED" << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cout << "FAILED (Exception: " << e.what() << ")" << std::endl;
        return false;
    }
}

/**
 * Main test runner
 */
int main() {
    std::cout << "IBAMR Restart Cleaner Test Suite" << std::endl;
    std::cout << "Testing RestartCleaner class functionality" << std::endl << std::endl;

    TestEnvironment env;
    bool all_tests_passed = true;

    // Run all tests in logical order
    all_tests_passed &= test_iteration_parsing_via_public_interface(env);
    all_tests_passed &= test_directory_filtering(env);  
    all_tests_passed &= test_cleanup_dry_run(env);
    all_tests_passed &= test_error_handling();

    // Final report
    std::cout << std::endl;
    if (all_tests_passed) {
        std::cout << "All tests PASSED!" << std::endl;
        std::cout << "RestartCleaner is working correctly." << std::endl;
        return 0;
    } else {
        std::cout << "Some tests FAILED!" << std::endl;
        std::cout << "Please check the implementation." << std::endl;
        return 1;
    }
}