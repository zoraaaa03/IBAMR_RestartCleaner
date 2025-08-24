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

/////////////////////////////// INCLUDES /////////////////////////////////////

#include "restart_cleaner_standalone.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// Future IBAMR integration:
// namespace IBTK {

/////////////////////////////// PUBLIC ///////////////////////////////////////

RestartCleaner::RestartCleaner(const std::string& restart_base_path,
                               int keep_restart_count,
                               const std::string& strategy,
                               bool dry_run)
    : d_restart_base_path(restart_base_path),
      d_strategy(parseStrategy(strategy)),
      d_keep_restart_count(keep_restart_count),
      d_dry_run(dry_run)
{
    if (keep_restart_count <= 0)
    {
        throw std::invalid_argument("RestartCleaner: keep_restart_count must be positive");
    }
    
    if (!fs::exists(restart_base_path))
    {
        throw std::invalid_argument("RestartCleaner: restart_base_path does not exist: " + restart_base_path);
    }
}

void RestartCleaner::cleanup()
{
    std::cout << "RestartCleaner: Starting cleanup of " << d_restart_base_path << std::endl;
    std::cout << "Keeping " << d_keep_restart_count << " most recent restart directories" << std::endl;
    
    executeStrategy();
}

std::vector<int> RestartCleaner::getAvailableIterations() const
{
    std::vector<int> iterations;
    
    try
    {
        std::vector<fs::path> all_dirs = getAllRestartDirs(d_restart_base_path);
        
        for (const auto& dir : all_dirs)
        {
            int iter = parseIterationNum(dir.filename().string());
            if (iter >= 0)
            {
                iterations.push_back(iter);
            }
        }
        
        std::sort(iterations.begin(), iterations.end());
    }
    catch (const std::exception& e)
    {
        std::cerr << "RestartCleaner: Error scanning directories: " << e.what() << std::endl;
    }
    
    return iterations;
}

/////////////////////////////// PRIVATE //////////////////////////////////////

RestartCleaner::CleanupStrategy RestartCleaner::parseStrategy(const std::string& strategy_str) const
{
    if (strategy_str == "KEEP_RECENT_N")
    {
        return CleanupStrategy::KEEP_RECENT_N;
    }
    
    throw std::invalid_argument("RestartCleaner: Unknown strategy: " + strategy_str);
}

void RestartCleaner::executeStrategy() const
{
    // Currently only one strategy - direct call to avoid unnecessary complexity
    // Future: expand to switch statement when more strategies are added
    keepRecentN();
}

int RestartCleaner::parseIterationNum(const std::string& dirname) const
{
    // Expect format: "restore.XXXXXX" where XXXXXX is exactly 6 digits
    if (dirname.length() != 14 || dirname.substr(0, 8) != "restore.")
    {
        return -1;
    }
    
    // Check that the remaining 6 characters are all digits
    std::string number_part = dirname.substr(8);
    if (number_part.length() != 6)
    {
        return -1;
    }
    
    for (char c : number_part)
    {
        if (!std::isdigit(c))
        {
            return -1;
        }
    }
    
    try
    {
        return std::stoi(number_part);
    }
    catch (...)
    {
        return -1;
    }
}

std::vector<fs::path> RestartCleaner::getAllRestartDirs(const std::string& restart_dir) const
{
    std::vector<fs::path> restart_dirs;
    
    try
    {
        if (fs::exists(restart_dir))
        {
            for (const auto& entry : fs::directory_iterator(restart_dir))
            {
                if (entry.is_directory())
                {
                    std::string dirname = entry.path().filename().string();
                    if (parseIterationNum(dirname) >= 0)
                    {
                        restart_dirs.push_back(entry.path());
                    }
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error("RestartCleaner: Error scanning directory: " + std::string(e.what()));
    }
    
    return restart_dirs;
}

void RestartCleaner::keepRecentN() const
{
    std::vector<fs::path> all_dirs = getAllRestartDirs(d_restart_base_path);
    
    if (all_dirs.empty())
    {
        std::cout << "No restart directories found" << std::endl;
        return;
    }
    
    std::cout << "Found " << all_dirs.size() << " restart directories" << std::endl;
    
    // Parse iteration numbers and sort
    std::vector<std::pair<int, fs::path>> dirs_with_iter;
    
    for (const auto& dir : all_dirs)
    {
        int iter = parseIterationNum(dir.filename().string());
        if (iter >= 0)
        {
            dirs_with_iter.push_back({iter, dir});
        }
    }
    
    std::sort(dirs_with_iter.begin(), dirs_with_iter.end());
    
    // Determine which directories to delete
    if (static_cast<int>(dirs_with_iter.size()) <= d_keep_restart_count)
    {
        std::cout << "No cleanup needed, keeping all " << dirs_with_iter.size() << " directories" << std::endl;
        return;
    }
    
    // Delete old directories
    int num_to_delete = dirs_with_iter.size() - d_keep_restart_count;
    std::cout << "Deleting " << num_to_delete << " old restart directories (keeping " 
              << d_keep_restart_count << " most recent)" << std::endl;
    
    for (int i = 0; i < num_to_delete; ++i)
    {
        const fs::path& dir_path = dirs_with_iter[i].second;
        
        if (d_dry_run)
        {
            std::cout << "  DRY RUN: Would delete " << dir_path << std::endl;
        }
        else
        {
            try
            {
                fs::remove_all(dir_path);
                std::cout << "  Deleted " << dir_path << std::endl;
            }
            catch (const std::exception& e)
            {
                std::cerr << "  Error deleting " << dir_path << ": " << e.what() << std::endl;
            }
        }
    }
}

// } // Future IBAMR integration namespace

//////////////////////////////////////////////////////////////////////////////