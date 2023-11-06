#include "Palette.hpp"
#include "Image.hpp"
#include "Ex.hpp"
#include "Log.hpp"
#include <cassert>
#include <algorithm>
#include <numeric>
#include <fstream>
#include <iostream>

#include "lodepng.h"

namespace
{

class ColorSet
{
public:
  ColorSet( Color c )
  {
    add( c );
  }

  void add( Color c )
  {
    if ( c )
    {
      mColors.push_back( c );
      mMeanColor = Color::mean( mColors );
    }
  }

  void merge( ColorSet const& other )
  {
    for ( auto c : other.mColors )
    {
      mColors.push_back( c );
    }
    mMeanColor = Color::mean( mColors );
  }

  Color mean() const
  {
    return mMeanColor;
  }

private:
  std::vector<Color> mColors = {};
  Color mMeanColor = {};
};

}

Palette::Palette( std::filesystem::path const& path, int maxColors, std::optional<std::filesystem::path> optimalPalette ) : mColors{}
{
  Image src{ path };

  std::vector<ColorSet> colorSets;

  src.eachPixel( [&]( Color c )
  {
    auto it = std::find_if( colorSets.cbegin(), colorSets.cend(), [c]( ColorSet const& cs )
    {
      return cs.mean() == c;
    });
    if ( it == colorSets.cend() )
      colorSets.push_back( c );
  } );

  LOG << "Palette " << path << " has " << colorSets.size() << " colors";

  if ( colorSets.size() > 256 )
  {
    throw Ex{} << "Image " << path << " with palette hase too many (" << colorSets.size() << ") colors";
  }

 
  if ( colorSets.size() > maxColors )
  {

    // merge closest colors until we have maxColors
    // O(n^3) but n is small
    while ( colorSets.size() > maxColors )
    {
      size_t si = ~0;
      size_t sj = ~0;
      int d = std::numeric_limits<int>::max();

      for ( size_t i = 0; i < colorSets.size(); ++i )
      {
        for ( size_t j = 0; j < colorSets.size(); ++j )
        {
          if ( int td = dist( colorSets[i].mean(), colorSets[j].mean() ) )
          {
            if ( td < d )
            {
              d = td;
              si = i;
              sj = j;
            }
          }
        }
      }

      assert( si != sj );

      size_t dst = std::min( si, sj );
      size_t src = std::max( si, sj );
      colorSets[dst].merge( colorSets[src] );

      colorSets.erase( colorSets.begin() + src );
    }
    LOG << "Reduced to " << maxColors << " colors";
  }

  for ( auto const& cs : colorSets )
  {
    mColors.push_back( cs.mean() );
  }

  std::sort( mColors.begin(), mColors.end() );

  if ( optimalPalette )
  {
    if ( !std::filesystem::exists( *optimalPalette ) || std::filesystem::last_write_time( *optimalPalette ) < std::filesystem::last_write_time( path ) )
    {
      auto colortype = LCT_PALETTE;
      int paletteSize = std::max( 2, ( int )mColors.size() );
      int bitdepth;
      for ( bitdepth = 0; ( 1 << bitdepth ) < paletteSize; ++bitdepth );
      std::vector<Color> palette;
      palette.resize( 256 );
      std::copy( mColors.begin(), mColors.end(), palette.begin() );

      unsigned char* out;
      size_t outsize;
      unsigned error;

      LodePNGState state;
      lodepng_state_init( &state );
      state.info_raw.colortype = colortype;
      state.info_raw.bitdepth = 8;
      state.info_raw.palette = (uint8_t*)palette.data();
      state.info_raw.palettesize = 1ull << bitdepth;
      state.info_png.color.colortype = colortype;
      state.info_png.color.bitdepth = 8;
      state.info_png.color.palettesize = 1ull << bitdepth;


      std::vector<uint8_t> indices;
      indices.resize( mColors.size() );
      std::iota( indices.begin(), indices.end(), 0 );

      lodepng_encode( &out, &outsize, indices.data(), (uint32_t)indices.size(), 1, &state );
      error = state.error;
      state.info_raw.palette = nullptr;
      lodepng_state_cleanup( &state );

      std::shared_ptr<uint8_t> data( out, []( uint8_t* p )
      {
        if ( p )
          ::free( ( void* )p );
      } );


      if ( error )
      {
        throw Ex{} << "PNG encode error " << error << ": " << lodepng_error_text( error );
      }


      std::ofstream ofs{ *optimalPalette, std::ios::binary };
      ofs.write( ( char* )data.get(), outsize );

      LOG << "Optimal palette wrote to " << *optimalPalette;
    }
  }
}

size_t Palette::size() const
{
  return mColors.size();
}

size_t Palette::mapNearest( Color src ) const
{
  //if color is transparent -> always return index 0
  if ( !src )
    return 0;

  int minDist = std::numeric_limits<int>::max();
  size_t idx{};
  assert( !mColors.empty() );
  for ( size_t i = mColors[0] ? 0 : 1; i < mColors.size(); ++i )
  {
    int dst = dist( mColors[i], src );
    if ( dst < minDist )
    {
      minDist = dst;
      idx = i;
    }
  }
  return idx;
}

std::vector<Color> const & Palette::colors() const
{
  return mColors;
}
