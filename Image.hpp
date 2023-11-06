#pragma once

#include <memory>
#include <filesystem>
#include "Color.hpp"
#include "ImageRow.hpp"

class Image : public std::enable_shared_from_this<Image>
{
public:
  Image( std::filesystem::path const& path );
  ~Image() = default;

  int width() const;
  int height() const;
  Color operator()( int x, int y ) const;

  ImageRow row( int row, int begin, int end ) const
  {
    return ImageRow{ *this, row, begin, end };
  }

  ImageRow row( int r ) const
  {
    return row( r, 0, width() );
  }

  template<typename P>
  void eachPixel( P const& fun ) const
  {
    for ( int y = 0; y < height(); ++y )
    {
      for ( int x = 0; x < width(); ++x )
      {
        fun( operator()( x, y ) );
      }
    }
  }

private:
  std::shared_ptr<Color> mData;
  uint32_t mWidth;
  uint32_t mHeight;
};

