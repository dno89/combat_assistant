/**
 * @file Logger.cpp
 * @author Daniele Molinari (dmolina@vislab.it)
 * @brief some debug utilities
 * @version 1.0
 */

////include
//std
#include <string>
//debug
#include "Logger.h"
//boost
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/chrono.hpp>


////static member initialization
//time origin
const boost::chrono::high_resolution_clock::time_point Logger::ms_hr_time_origin(boost::chrono::high_resolution_clock::now());
const boost::chrono::system_clock::time_point Logger::ms_sc_time_origin(boost::chrono::system_clock::now());

//master log
boost::scoped_ptr<std::ofstream> Logger::ms_pmaster_log(new std::ofstream(std::string("/tmp/MasterLog-" + boost::posix_time::to_iso_extended_string(boost::posix_time::second_clock::local_time()) + ".log").c_str(), std::ios_base::trunc));

//the sync mutex
boost::scoped_ptr<boost::mutex> Logger::ms_pmutex(new boost::mutex());