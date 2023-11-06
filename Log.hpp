#pragma once

#include <string>
#include <sstream>

class Log
{
public:

  void verbose( bool isVerbose );

  void log( std::string const& message );

  static Log & instance();

private:
  Log();
  bool mVerbose = false;

};


class Formatter
{
public:
  Formatter();
  ~Formatter();

  template<typename T>
  Formatter & operator<<( T const& t )
  {
    mSS << t;
    return *this;
  }

private:
  std::stringstream mSS;
};

#define LOG Formatter{}

