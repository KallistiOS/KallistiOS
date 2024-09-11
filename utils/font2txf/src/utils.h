/* font2txf
 *
 * This code was contributed to KallistiOS (KOS) by Mickaël Cardoso (SiZiOUS).
 * It was originally made by Chris Laurel and the Celestia project team, for
 * producing the ttf2txf utility. The TXF format was created by Mark J. Kilgard.
 *
 * This code is licensed under GNU GPL 2, check LICENSE for details.
 */

#ifndef __UTILS_H__
#define __UTILS_H__

#include <memory>
#include <cstdio>
#include <string>
#include <iostream>
#include <utility>
#include <stdexcept>

/* Extract the stem (basename) of the current program (usually 'font2txf'). 
 * This function should be called once at the beginning of the program. */
void program_name_initialize( char* argv0 );

/* Return the name of the current program (usually 'font2txf'). */
std::string program_name_get();

/* Translate a boolean value to 'true' or 'false'. */
std::string bool_to_str( bool b );

/* Convert an integer value (base 10) into a hexadecimal value (base 16).
 * This is only used for printing formatted values.
 * Thanks to HeavenHM.
 * See: https://stackoverflow.com/a/69328575/3726096
 */
std::string int_to_hex(int hex_val);

/* Check if a file exists */
bool file_exists( const std::string& name );

/* Log level set up from the command line */
enum LogLevel 
{
    Quiet = 0, 
    Standard = 1, 
    Verbose = 2
};

/* Console is a "browser-like" logger, based on the work done by kungfooman and Nikos Athanasiou.
 * See: https://stackoverflow.com/a/52970404
 * See: https://stackoverflow.com/a/33869493/3726096
 */
class Console 
{
    enum class Severity { Info, Warning, Error, Fatal, Debug };

    private:
        /* Translate severity level into the corresponding string.
         * This probably could be better in C++17 (but we want to keep compatibility with older compilers...). */
        std::string get_severity_name(Severity severity)
        {
            std::string result = std::string();

            switch(severity) {
                case Severity::Info:
                    break;                  
                case Severity::Warning:
                    result = "warning";
                    break;
                case Severity::Error:
                    result = "error";
                    break;
                case Severity::Fatal:
                    result = "fatal";
                    break;                    
                case Severity::Debug:
                    result = "DEBUG";
                    break;
            }

            return result;
        }

        /* Get the corresponding stream depending of the severity.
         * Basically, this returns cout or cerr. */
        std::ostream& get_output_stream(Severity severity)
        {
            if ( severity == Severity::Error || severity == Severity::Fatal )
            {
                return std::cerr;
            }
            return std::cout;
        }

    protected:
        template <typename T>
        void log_argument(std::ostream& stream, T type)
        {
            stream << type;
        }

        template <typename... Args>
        void log_trigger(Severity severity, Args&&... args)
        {
            bool allowed = true;
#ifndef _DEBUG
            allowed = ( severity != Severity::Debug );
#endif
            if( allowed )
            {
                std::ostream& stream = get_output_stream(severity);
                std::string severity_name = get_severity_name(severity);

                stream << program_name_get() << ": " << severity_name << ( !severity_name.empty() ? ": " : std::string() );

                using expander = int[];
                (void) expander { 0, ( (void) log_argument( stream, std::forward<Args>(args) ), 0 )... };
                
                stream << std::endl;
            }
        }

    public:
        /* Display a message. Uses stdout. */
        template <typename... Args>
        void log(Args&&... args)
        {
            log_trigger(Severity::Info, args ...);            
        }

        /* Display a warning message: prefix with "warning". Uses stdout. */
        template <typename... Args>
        void warn(Args&&... args)
        {
            log_trigger(Severity::Warning, args ...);
        }

        /* Display an error message: prefix with "error". Uses stderr. */
        template <typename... Args>
        void error(Args&&... args)
        {
            log_trigger(Severity::Error, args ...);
        }

        /* Display a fatal error message: prefix with "fatal". Uses stderr. */
        template <typename... Args>
        void fatal(Args&&... args)
        {
            log_trigger(Severity::Fatal, args ...);
        }
        

        /* Display a debug message: prefix with "DEBUG". This will work only if _DEBUG macro is defined. */
        template <typename... Args>
        void debug(Args&&... args)
        {
            log_trigger(Severity::Debug, args ...);
        }
};

#endif /* __UTILS_H__ */
