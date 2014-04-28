/**
 * @file Logger.h
 * @author Daniele Molinari (dmolina@vislab.it)
 * @brief some debug utilities
 * @version 1.0
 */

#ifndef DMOLINA_DEBUG_H
#define DMOLINA_DEBUG_H

////include
//std
#include <iostream>
// #include <ostream>
#include <fstream>
#include <ctime>
#include <stack>
//boost
#include <boost/chrono.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/smart_ptr/scoped_ptr.hpp>
#include <boost/thread/lock_guard.hpp>

template<typename T>
std::string AbsoluteChronoDateTime(T time) {
    static const int BSIZE = 50;
    
    using namespace boost::chrono;
    using namespace std;
    
    //the date/time
    time_t tt = system_clock::to_time_t(time); 
    tm* t = localtime(&tt);
    char buffer[BSIZE];
    int written = strftime(buffer, BSIZE, "%F %T", t);
    return string(buffer);
}

// template<typename T>
// std::string AbsoluteChronoTime(T time) {
//     static const int BSIZE = 50;
//     
//     using namespace boost::chrono;
//     using namespace std;
//     
//     //get the second and nanoseconds since epoch
//     nanoseconds ns = time_point_cast<ToDuration, system_clock>(time);
// //     seconds s = duration_cast<seconds>(ns);
//     //the date/time
// //     time_t tt = s.count();
//     time_t tt = system_clock::to_time_t(time);
//     tm* t = localtime(&tt);
//     
//     char buffer[BSIZE];
//     int written = strftime(buffer, BSIZE, "%T", t);
//     sprintf(buffer+written, ".%d", static_cast<int>(ns.count()%1000000000));
//     
//     return string(buffer);
// }

//////////////////////////////////////////////////////////////////////
//// LOGGER CLASS
//////////////////////////////////////////////////////////////////////

/**
 * @brief class that provides logging capability with some useful macro
 * @note it also mantain a master log where the log from all Logger instance is logged
 * @warning this implementation is quite inefficient since it involves many memory movement through the stringstring but succeed to minimize the synchronization to its minimum
 */
class Logger {
public:
    Logger(const char* log_file, bool relative = true, std::string logger_name = "DefaultLogger", bool unique = true) : 
    m_out(unique?std::string(std::string(log_file)+"-"+boost::posix_time::to_iso_extended_string(boost::posix_time::second_clock::local_time())+".log").c_str():std::string(std::string(log_file)+".log").c_str(), std::ios_base::trunc),
    m_relative(relative),
    m_name(logger_name)
    {
        using namespace boost;
        using namespace std;
        {
            lock_guard<mutex> lck(*ms_pmutex);
            (*ms_pmaster_log) << boost::posix_time::to_iso_extended_string(boost::posix_time::microsec_clock::local_time()) << "\t[**] '" << m_name << "' registered." << endl;
        }
        
        //log the init event
        printTime();
        m_out << "[**] " << m_name << " initialized with time origin: " << AbsoluteChronoDateTime(ms_sc_time_origin) << endl;
    }
    
    ~Logger() {
        m_out.close();
    }
    
    /**
     * @brief check if the logger express time in relative manner
     */
    bool IsRelative() const {
        return m_relative;
    }
    
    /**
     * @brief set relative mode
     */
    void SetRelative(bool relative) {
        m_relative = relative;
    }
    /**
     * @brief support class to enable concatenation
     */
    friend class LoggerProxy;
    class LoggerProxy {
    public:
        ////constructor-destructor
        LoggerProxy(LoggerProxy& p) : m_ssp(p.m_ssp), m_log(p.m_log) {
            //get the property of the logger
            p.m_log = NULL;
        }
        LoggerProxy(Logger* logger, std::stringstream* ssp) : m_log(logger), m_ssp(ssp) {}
        ~LoggerProxy() {
            if(m_log) {
                //add the final endl
                m_log->m_out << std::endl;
                
                boost::lock_guard<boost::mutex> lck(*Logger::ms_pmutex);
                *Logger::ms_pmaster_log << m_ssp->str() << std::endl;
            }
        }
        //operator overloading
        template<typename T>
        LoggerProxy operator<<(const T& arg) {
            //print on the local stream
            m_log->m_out << arg;
            //build the stream for the final dump on master log
            *m_ssp << arg;
            
            //return a proxy
            LoggerProxy lp(*this);
            return lp;
        }
    private:
        Logger* m_log;
        std::stringstream* m_ssp;
    };
    
    /**
     * @brief operator<< overloading
     */
    template<typename T>
    LoggerProxy operator<<(const T& arg) {
        //print on the local stream
        //print time
        printTime();
        //print name, context and arg
        if(m_context_stack.size()) {
            m_out << '<' << m_context_stack.top() << ">\t";
        }
        m_out << arg;
        
        //reset the stream for the final dump on master log
        m_ss.str("");
        m_ss.clear();
        //print the time, name and context
        m_ss << boost::posix_time::to_iso_extended_string(boost::posix_time::microsec_clock::local_time()) << '\t' << m_name << '\t';
        if(m_context_stack.size()) {
            m_ss << '<' << m_context_stack.top() << ">\t";
        }
        m_ss << arg;
        
        //return a proxy
        LoggerProxy lp(this, &m_ss);
        return lp;
    }
    
    /**
     * @brief add new context to the stack
     */
    void PushContext(std::string context) {
        m_context_stack.push(context);
    }
    
    /**
     * @brief pop the last context from the stack
     */
    void PopContext() {
        if(m_context_stack.size()) {
            m_context_stack.pop();
        }
    }
    
    /**
     * @brief restore to the default context = empty string
     */
    void ResetContext() {
        while(m_context_stack.size()) {
            m_context_stack.pop();
        }
    }
    
    
private:
    /////////////////////////////////////////////
    //// DATA
    /////////////////////////////////////////////
    //the output stream
    std::ofstream m_out;
    //user relative times?
    bool m_relative;
    //the current name
    std::string m_name;
    //the context stack name
    std::stack<std::string> m_context_stack;
    
    //the stringstream used to write on master log
    std::stringstream m_ss;
    
    /////////////////////////////////////////////
    //// FUNCTIONS
    /////////////////////////////////////////////
    void printTime() {
        using namespace boost::chrono;
        using namespace std;
        
        if(m_relative) {
            high_resolution_clock::duration rtime = high_resolution_clock::now()-ms_hr_time_origin;
            m_out.fill('0');
            m_out << setw(4) << duration_cast<seconds>(rtime).count() << "." << setw(9) << duration_cast<nanoseconds>(rtime).count() << "\t";
            m_out.copyfmt(ios(NULL));
        } else {
            m_out << boost::posix_time::to_iso_extended_string(boost::posix_time::microsec_clock::local_time()) << "\t";
        }
    }
    
    /////////////////////////////////////////////
    //// STATIC MEMBERS
    /////////////////////////////////////////////
    //the common time origin
    static const boost::chrono::high_resolution_clock::time_point ms_hr_time_origin;
    static const boost::chrono::system_clock::time_point ms_sc_time_origin;
    //flag that show if stati data has been initialized
    static bool ms_init;
    //the centralized log
    static boost::scoped_ptr<std::ofstream> ms_pmaster_log;
    //the synchronization log
    static boost::scoped_ptr<boost::mutex> ms_pmutex;
};

//////////////////////////////////////////////////////////////////////
//// USEFUL MACRO
//////////////////////////////////////////////////////////////////////

#ifndef DMOLINA_DISABLE_LOGGING //ENABLE LOGGING

#define LOGGER()\
m_automatic_declared_logger

#define DECLARE_LOGGER()\
Logger m_automatic_declared_logger;

#define DEFINE_LOGGER(FNAME, REL_TIME, NAME, UNIQUE)\
Logger m_automatic_declared_logger(FNAME, REL_TIME, NAME, UNIQUE);

#define INIT_LOGGER(FNAME, REL_TIME, NAME, UNIQUE)\
m_automatic_declared_logger(FNAME, REL_TIME, NAME, UNIQUE)

#define DTRACE(X)\
m_automatic_declared_logger << #X << " = " << (X);

#define DPRINT(X)\
m_automatic_declared_logger << X;

#define DINFO(X)\
m_automatic_declared_logger << "[II] " << X;

#define DWARNING(X)\
m_automatic_declared_logger << "[WW] " << X;

#define DERROR(X)\
m_automatic_declared_logger << "[EE] " << X;

#define DPUSHC(X)\
m_automatic_declared_logger.PushContext(X);

#define DPOPC()\
m_automatic_declared_logger.PopContext();

#define DRESETC()\
m_automatic_declared_logger.ResetContext();

#else //DISABLE LOGGING

#define DECLARE_LOGGER()
#define LOGGER()
#define DEFINE_LOGGER(FNAME, REL_TIME, NAME, UNIQUE)
#define INIT_LOGGER(FNAME, REL_TIME, NAME, UNIQUE)
#define DTRACE(X)
#define DINFO(X)
#define DWARNING(X)
#define DERROR(X)
#endif

#endif  //DMOLINA_DEBUG_H
