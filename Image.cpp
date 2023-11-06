#include "Image.hpp"
#include "Ex.hpp"

#include "lodepng.h"


Image::Image( std::filesystem::path const & path ) : mData{}, mWidth{}, mHeight{}
{
  uint8_t* data;
  unsigned error = lodepng_decode32_file( &data, &mWidth, &mHeight, path.string().c_str() );

  if ( error )
  {
    throw Ex{} << "PNG decoder error " << error << ": " << lodepng_error_text( error );
  }

  mData.reset( reinterpret_cast< Color* >( data ), []( Color* p )
  {
    if ( p )
      ::free( ( void* )p );
  } );
}

int Image::width() const
{
  return (int)mWidth;
}

int Image::height() const
{
  return (int)mHeight;
}

Color Image::operator()( int x, int y ) const
{
  size_t offset = y * width() + x;
  if ( offset >= mWidth * mHeight * sizeof(Color) )
  {
    return Color{};
  }
  else
  {
    return *( mData.get() + offset );
  }
}

