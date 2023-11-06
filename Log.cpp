#include "Log.hpp"
#include <iostream>
#ifdef _WIN32
#include <Windows.h>
#endif


Log::Log()
{
}

void Log::verbose( bool isVerbose )
{
  mVerbose = isVerbose;
}

void Log::log( std::string const & message )
{
  if ( mVerbose )
  {
    std::cout << message;
#ifdef _WIN32
    OutputDebugStringA( message.c_str() );
#endif
  }
}

Log & Log::instance()
{
  static Log instance{};
  return instance;
}

Formatter::Formatter() : mSS{}
{
}

Formatter::~Formatter()
{
  mSS << std::endl;
  Log::instance().log( mSS.str() );
}
