// ---------------------------------------------------------------------
//
// Copyright (c) 2011 - 2025 by the IBAMR developers
// All rights reserved.
//
// This file is part of IBAMR.
//
// IBAMR is free software and is distributed under the 3-clause BSD
// license. The full text of the license can be found in the file
// COPYRIGHT at the top level directory of IBAMR.
//
// ---------------------------------------------------------------------

/////////////////////////////// INCLUDE GUARD ////////////////////////////////

#ifndef included_RestartCleaner
#define included_RestartCleaner

/////////////////////////////// INCLUDES /////////////////////////////////////

#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

// Future IBAMR integration:
// namespace IBTK {

/////////////////////////////// CLASS DEFINITION /////////////////////////////

/*!
 * \brief Class RestartCleaner provides functionality to manage restart directories.
 *
 * This class provides methods to automatically clean up old restart directories while
 * keeping the most recent ones. Supports extensible cleanup strategies.
 * It is designed to work with IBAMR's standard restart directory naming convention.
 *
 * The main functionalities include:
 * -# Scan a specified directory for subdirectories matching the pattern "restore.XXXXXX"
 * -# Parse iteration numbers from these directory names
 * -# Sort directories based on iteration numbers  
 * -# Keep the N most recent directories and delete the rest
 *
 * \note This class assumes restart directories follow the naming pattern "restore.XXXXXX"
 * where XXXXXX is a zero-padded iteration number.
 *
 * Supported cleanup strategies:
 * - "KEEP_RECENT_N": Keep the N most recent restart directories (default)
 * - Future extensions: "SMART_RETENTION", "TIME_BASED", etc.
 *
 * Sample usage:
 * \code
 * // Basic usage
 * RestartCleaner cleaner("/path/to/restores", 5);
 * cleaner.cleanup();
 * // With dry run
 * RestartCleaner cleaner("/path/to/restores", 3, "KEEP_RECENT_N", true);
 * // Future strategy example:
 * // RestartCleaner cleaner("/path/to/restores", 5, "SMART_RETENTION");
 * // Verify results
 * auto iterations = cleaner.getAvailableIterations();
 * \endcode
 */
class RestartCleaner
{
public:
    /*!
     * \brief Constructor.
     *
     * \param restart_base_path  Base directory containing restore folders
     * \param keep_restart_count Number of recent restore directories to keep
     * \param strategy          Cleanup strategy ("KEEP_RECENT_N", future: "SMART_RETENTION")  
     * \param dry_run           If true, only report what would be deleted without actually deleting
     */
    RestartCleaner(const std::string& restart_base_path,
                   int keep_restart_count,
                   const std::string& strategy = "KEEP_RECENT_N",
                   bool dry_run = false);

    // Future IBAMR integration constructor:
    // RestartCleaner(const std::string& object_name,
    //                SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> input_db);

    /*!
     * \brief Destructor.
     */
    ~RestartCleaner() = default;

    /*!
     * \brief Scan and cleanup old restart directories.
     *
     * This method performs the complete cleanup process:
     * 1. Scans the base directory for restart folders
     * 2. Parses iteration numbers from directory names
     * 3. Sorts directories by iteration number
     * 4. Keeps the N most recent directories and deletes the rest
     */
    void cleanup();

    /*!
     * \brief Get available iteration numbers.
     *
     * \return Vector of iteration numbers for available restore directories,
     *         sorted in ascending order
     */
    std::vector<int> getAvailableIterations() const;

private:
    RestartCleaner() = delete;
    RestartCleaner(const RestartCleaner& from) = delete;
    RestartCleaner& operator=(const RestartCleaner& that) = delete;

    /*!
     * \brief Internal strategy enumeration.
     */
    enum class CleanupStrategy { 
        KEEP_RECENT_N
        // Future strategies: SMART_RETENTION, TIME_BASED
    };
    
    /*!
     * \brief Parse strategy string to enum.
     */
    CleanupStrategy parseStrategy(const std::string& strategy_str) const;
    
    /*!
     * \brief Execute cleanup based on current strategy.
     */
    void executeStrategy() const;

    /*!
     * \brief Parse iteration number from directory name.
     *
     * Extracts the iteration number from directory names following the pattern
     * "restore.XXXXXX" where XXXXXX is a zero-padded number.
     *
     * \param dirname Directory name to parse
     * \return Iteration number, or -1 if parsing fails
     */
    int parseIterationNum(const std::string& dirname) const;

    /*!
     * \brief Get all restart directories from the base path.
     *
     * Scans the restart base directory and returns all subdirectories that
     * match the restart naming pattern.
     *
     * \param restart_dir Base directory to scan
     * \return Vector of filesystem paths to valid restart directories
     */
    std::vector<fs::path> getAllRestartDirs(const std::string& restart_dir) const;

    /*!
     * \brief KEEP_RECENT_N strategy implementation.
     */
    void keepRecentN() const;

    const std::string d_restart_base_path;
    const CleanupStrategy d_strategy;
    const int d_keep_restart_count;
    const bool d_dry_run;
};

// } // Future IBAMR integration namespace

//////////////////////////////////////////////////////////////////////////////

#endif // #ifndef included_RestartCleaner