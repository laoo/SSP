#pragma once

#include <cstdint>
#include <vector>
#include <string>

class Color
{
  union
  {
    uint32_t mRgba;
    struct
    {
      uint8_t mR;
      uint8_t mG;
      uint8_t mB;
      uint8_t mA;
    };
  };

public:

  Color() : mR{}, mG{}, mB{}, mA{}
  {
  }

  Color( uint8_t r, uint8_t g, uint8_t b, uint8_t a ) : mR{ r }, mG{ g }, mB{ b }, mA{ a }
  {
  }

  uint8_t a() const
  {
    return mA;
  }

  uint8_t r4() const
  {
    int ret = ( mR >> 4 ) + ( ( mR & 0x08 ) >> 3 );
    return ret > 15 ? 15 : ret;
  }

  uint8_t g4() const
  {
    int ret = ( mG >> 4 ) + ( ( mG & 0x08 ) >> 3 );
    return ret > 15 ? 15 : ret;
  }

  uint8_t b4() const
  {
    int ret = ( mB >> 4 ) + ( ( mG & 0x08 ) >> 3 );
    return ret > 15 ? 15 : ret;
  }

  uint8_t br4() const
  {
    return ( b4() << 4 ) | r4();
  }

  std::string str() const
  {
    static const char* hex = "0123456789ABCDEF";
    std::string ret;
    ret.resize( 3 );
    ret[0] = hex[ g4() ];
    ret[1] = hex[ b4() ];
    ret[2] = hex[ r4() ];
    return ret;
  }

  explicit operator bool() const
  {
    return a() != 0;
  }

  int norm() const
  {
    return mA ? ( (int)mR + mG + mB ) : 0;
  }

  friend bool operator==( Color left, Color right )
  {
    return left.a() == 0 && right.a() == 0 || left.mRgba == right.mRgba;
  }

  friend bool operator<( Color left, Color right )
  {
    return left.norm() < right.norm();
  }

  friend int dist( Color left, Color right )
  {
    int dr = left.mR - right.mR;
    int dg = left.mG - right.mG;
    int db = left.mB - right.mB;
    return dr * dr + dg * dg + db * db;
  }

  friend Color avg( Color left, Color right )
  {
    int ar = ( (int)left.mR + right.mR ) / 2;
    int ag = ( (int)left.mG + right.mG ) / 2;
    int ab = ( (int)left.mB + right.mB ) / 2;
    int aa = ( (int)left.mA + right.mA ) / 2;
    return { (uint8_t)ar, (uint8_t)ag, (uint8_t)ab, (uint8_t)aa };
  }

  static Color mean( std::vector<Color> const& colors )
  {
    int r = 0;
    int g = 0;
    int b = 0;
    int a = 0;

    for ( auto const& c : colors )
    {
      r += c.mR;
      g += c.mG;
      b += c.mB;
      a += c.mA;
    }

    return { (uint8_t)( r / colors.size() ), (uint8_t)( g / colors.size() ), (uint8_t)( b / colors.size() ), (uint8_t)( a / colors.size() ) };
  }
};

