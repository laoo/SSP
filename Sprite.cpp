#include "Sprite.hpp"
#include "Ex.hpp"
#include "Color.hpp"
#include "Log.hpp"
#include <cassert>
#include <fstream>
#include <functional>
#include "lodepng.h"

namespace
{

class BitAssembler
{
  std::vector<uint8_t> buffer;
  int accum;
  int size;
public:

  BitAssembler() : buffer{}, accum{}, size{}
  {
  }

  ~BitAssembler()
  {
    assert( size == 0 );
  }

  void shift( int bits, int value )
  {
    assert( bits > 0 );
    while ( bits > 0 )
    {
      if ( size + bits >= 8 )
      {
        accum <<= 8 - size;
        accum |= value >> ( bits - 8 + size );
        value &= ( 1 << ( bits - 8 + size ) ) - 1;
        bits -= 8 - size;
        buffer.push_back( accum );
        accum = size = 0;
      }
      else
      {
        accum <<= bits;
        accum |= value;
        size += bits;
        break;
      }
    }
  }

  void flushRow( bool literal = false )
  {
    if ( size > 0 )
    {
      accum <<= 8 - size;
      buffer.push_back( accum );
      accum = size = 0;
    }
    else if ( !literal )
    {
      // There is a bug in the hardware that requires that the last meaningful bit of the data packet at the end of a scan line does not occur in the last bit of a byte (bit 0).
      // This means that the data packet creation process must check for this case, and if found, must pad this data packet with a byte of all 0s.
      buffer.push_back( 0 );
    }
  }

  std::vector<uint8_t> const& getData() const
  {
    return buffer;
  }

  void update( size_t offset, uint8_t value )
  {
    assert( offset < buffer.size() );
    buffer[offset] = value;
  }

};


class SpriteRow
{
  explicit SpriteRow() : bitAssembler{}
  {
  }

  SpriteRow( BitAssembler bitAssembler ) : bitAssembler{ std::move( bitAssembler ) }
  {
  }

  BitAssembler bitAssembler;

public:

  static SpriteRow packed( int bpp, ImageRow const& row, std::function<int( Color )> mapper )
  {
    struct TempBuffer
    {
      TempBuffer( size_t pensSize ) : cmds{}, pens{}
      {
        pens.reserve( pensSize );
      }

      struct Cmd
      {
        bool rle;
        int off;
        int size;
      };

      std::vector<Cmd> cmds;
      std::vector<uint8_t> pens;


      void tokenize()
      {
        int streak{};
        bool rle{};
        for ( size_t i = 0; i < pens.size(); ++i, ++streak )
        {
          switch ( streak )
          {
          case 0:
            rle = false;
            break;
          case 1:
            rle = pens[i - 1] == pens[i];
            break;
          default:
            if ( ( pens[i - 1] == pens[i] ) ^ rle )
            {
              Cmd cmd{ rle, ( int )( i - streak ), streak - ( rle ? 0 : 1 ) };
              cmds.push_back( cmd );
              streak = rle ? 0 : 1;
              rle = !rle;
            }
            break;
          }
        }
        if ( cmds.empty() || cmds.back().off + cmds.back().size < ( int )pens.size() )
        {
          Cmd cmd{ rle, ( int )( pens.size() - streak ), streak };
          cmds.push_back( cmd );
        }
      }

      Cmd optimize( int off, Cmd const* beg, Cmd const* end )
      {
        uint32_t optScore = ~0;
        uint32_t optPerm = 0;

        auto size = [off]( Cmd const* it )
        {
          int result = it->size - std::max( off - it->off, 0 );
          result = std::min( result, 16 );
          return result;
        };

        for ( int perm = 0;; ++perm )
        {
          bool literal = false;
          int idx = 0;
          uint32_t score = 0;

          for ( auto it = beg; it != end; ++it )
          {
            if ( it->rle )
            {
              if ( ( perm & ( 1 << idx++ ) ) == 0 )
              {
                if ( literal )
                {
                  score += size( it ) * 4;
                }
                else
                {
                  literal = true;
                  score += 4 + 1 + size( it ) * 4;
                }
              }
              else
              {
                score += 9;
                literal = false;
              }
            }
            else
            {
              if ( literal )
              {
                score += size( it ) * 4;
              }
              else
              {
                literal = true;
                score += 4 + 1 + size( it ) * 4;
              }
            }
          }

          if ( score < optScore )
          {
            optScore = score;
            optPerm = perm;
          }

          if ( perm >= ( 1 << idx ) - 1 )
            break;
        }

        Cmd cmd{ false, off, 0 };

        int idx = 0;
        bool literal = false;
        for ( auto it = beg; it != end; ++it )
        {
          if ( it->rle && ( optPerm & ( 1 << idx++ ) ) != 0 )
          {
            if ( literal )
            {
              return cmd;
            }
            cmd.rle = true;
            cmd.size = size( it );
            return cmd;
          }
          else
          {
            literal = true;
            cmd.size += size( it );
            if ( cmd.size >= 16 )
            {
              cmd.size = 16;
              return cmd;
            }
          }
        }

        return cmd;
      }


      void optimize()
      {
        std::vector<Cmd> result;

        int off = 0;
        for ( auto it = cmds.cbegin(); it < cmds.cend(); )
        {
          assert( off >= it->off );
          for ( auto jt = it; jt < cmds.cend(); ++jt )
          {
            if ( jt + 1 == cmds.cend() || jt->off - off >= 16 )
            {
              Cmd cmd = optimize( off, &*it, &*jt + ( jt + 1 == cmds.cend() ? 1 : 0 ) );
              off += cmd.size;
              result.push_back( cmd );

              for ( ; it < cmds.cend(); ++it )
              {
                if ( it->off + it->size > off )
                {
                  break;
                }
              }
              break;
            }
          }
        }

        std::swap( cmds, result );
      }

      BitAssembler pack( int bpp ) const
      {
        BitAssembler result;

        for ( Cmd cmd : cmds )
        {
          result.shift( 1, cmd.rle ? 0 : 1 );                   // 0 marks that the next data will be RLE, 1 marks that the next data will be LITERAL
          result.shift( 4, cmd.size - 1 );                      // The next 4 bits will be the number of pixels to draw-1 ... we will call this N
          if ( cmd.rle )                                        // If the block is RLE
          {
            result.shift( bpp, pens[cmd.off] );                 // the next N * (1/2/3/4) bits (depending on bitdepth) will be used for the color of the next N pixels 
          }
          else                                                  // If the block is LITERAL
          {
            for ( auto it = pens.cbegin() + cmd.off; it != pens.cbegin() + cmd.off + cmd.size; ++it )
            {
              result.shift( bpp, *it );                         // the next N * (1/2/3/4) bits (depending on bitdepth) will be used for the color of the next N pixels 
            }
          }
        }

        result.flushRow();
        return result;
      }
    } tempBuffer{ row.size() };

    std::vector<uint8_t> pens;
    bool rle{};

    for ( auto c : row )
    {
      int pen = mapper( c );
      tempBuffer.pens.push_back( ( uint8_t )pen );
    }

    tempBuffer.tokenize();
    tempBuffer.optimize();

    return SpriteRow{ tempBuffer.pack( bpp ) };
  }

  static SpriteRow literal( int bpp, ImageRow const& row, std::function<int( Color )> mapper )
  {
    BitAssembler result;

    for ( auto c : row )
    {
      int pen = mapper( c );
      result.shift( bpp, pen );
    }

    return SpriteRow{ result };
  }

  std::vector<uint8_t>::const_iterator begin() const
  {
    return bitAssembler.getData().cbegin();
  }

  std::vector<uint8_t>::const_iterator end() const
  {
    return bitAssembler.getData().cend();
  }

  int size() const
  {
    return ( int )bitAssembler.getData().size();
  }
};

}

void createSprite( Image const& image, Bounds const& bounds, Palette const& pal, std::filesystem::path outputSprite, std::optional<std::filesystem::path> outputImage, std::optional<int> forcedBPP,
  bool background, bool literal, bool verbose, bool separateFiles )
{
  LOG << "\nProcessing frame " << bounds.minx << "," << bounds.miny << " to " << bounds.maxx << "," << bounds.maxy;

  std::vector<Color> const& originalColors = pal.colors();

  assert( originalColors.size() > 0 );

  struct ColorMapping
  {
    Color c;
    size_t idx;
    Color mapped;
  };

  std::vector<ColorMapping> colorMapping;

  image.eachPixel( [&]( Color c )
  {
    auto it = std::find_if( colorMapping.cbegin(), colorMapping.cend(), [&]( ColorMapping const& cm )
    {
      return cm.c == c;
    } );
    if ( it == colorMapping.cend() )
    {
      size_t mappedIdx = pal.mapNearest( c );
      ColorMapping cm{ c, mappedIdx, originalColors[mappedIdx] };
      colorMapping.push_back( cm );
    }
  } );

  auto colorsUsed = originalColors;

  if ( colorMapping.size() < colorsUsed.size() )
  {
    colorsUsed.erase( std::remove_if( colorsUsed.begin() + ( background ? 0 : 1 ), colorsUsed.end(), [&]( Color c )
    {
      auto it = std::find_if( colorMapping.cbegin(), colorMapping.cend(), [&]( ColorMapping const& cm )
      {
        return cm.mapped == c;
      } );
      return it == colorMapping.cend();
    } ), colorsUsed.end() );

    for ( auto& cm : colorMapping )
    {
      auto it = std::find( colorsUsed.cbegin(), colorsUsed.cend(), cm.mapped );
      assert( it != colorsUsed.cend() );
      cm.idx = std::distance( colorsUsed.cbegin(), it );
    }
  }

  auto mapper = [&]( Color c )
  {
    for ( auto const& cm : colorMapping )
    {
      if ( cm.c == c )
        return (int)cm.idx;
    }
    assert( false );
    return 0;
  };

  int colors = (int)colorsUsed.size();

  LOG << "Frame uses " << colors << " colors";

  int paletteSize = std::max( 2, colors );
  int bpp;
  for ( bpp = 0; ( 1 << bpp ) < paletteSize; ++bpp );

  if ( forcedBPP )
  {
    if ( forcedBPP.value() < bpp )
    {
      throw Ex{} << "Forced bitdepth is too low. Sprite requires " << bpp << " as there is " << colors << " colors";
    }
    else
    {
      LOG << "Sprites requires " << bpp << " bits per pixel, but forced bitdepth is " << forcedBPP.value() << ". This will result in wasted space in the sprite data.";
    }

    bpp = forcedBPP.value();
  }

  LOG << "Creating " << ( literal ?  "literal" : "packed" ) << " sprite with " << bpp << " bits per pixel";

  std::vector<SpriteRow> literalRows;
  std::vector<SpriteRow> packedRows;
  std::vector<uint8_t> literalData;
  std::vector<uint8_t> packedData;

  if ( literal || verbose )
  {
    for ( int y = bounds.miny; y <= bounds.maxy; ++y )
    {
      literalRows.push_back( SpriteRow::literal( bpp, image.row( y, bounds.minx, bounds.maxx + 1 ), mapper ) );
    }

    for ( auto const& row : literalRows )
    {
      literalData.push_back( ( uint8_t )row.size() + 1 );
      for ( uint8_t b : row )
      {
        literalData.push_back( b );
      }
    }

    literalData.push_back( 0 );
  }

  if ( !literal || verbose )
  {
    for ( int y = bounds.miny; y <= bounds.maxy; ++y )
    {
      packedRows.push_back( SpriteRow::packed( bpp, image.row( y, bounds.minx, bounds.maxx + 1 ), mapper ) );
    }

    for ( auto const& row : packedRows )
    {
      packedData.push_back( ( uint8_t )row.size() + 1 );
      for ( uint8_t b : row )
      {
        packedData.push_back( b );
      }
    }

    packedData.push_back( 0 );
  }

  std::vector<int> mapping{};
  mapping.reserve( 16 );

  for ( auto color : colorsUsed )
  {
    auto it = std::find( originalColors.cbegin(), originalColors.cend(), color );
    if ( it == originalColors.cend() )
    {
      throw Ex{} << "Palette processing error";
    }
    mapping.push_back( ( int )std::distance( originalColors.cbegin(), it ) );
  }

  std::vector<uint8_t> penMapping = std::vector<uint8_t>( ( mapping.size() + 1 ) / 2, 0 );

  for ( int i = 0; i < ( int )mapping.size(); ++i )
  {
    penMapping[i / 2] |= mapping[i] << ( ( i % 2 == 0 ) ? 4 : 0 );
  }

  if ( outputImage )
  {
    auto colortype = LCT_PALETTE;
    int paletteSize = std::max( 2, ( int )originalColors.size() );
    std::vector<Color> palette;
    palette.resize( 256 );
    std::copy( originalColors.begin(), originalColors.end(), palette.begin() );

    unsigned char* out;
    size_t outsize;
    unsigned error;

    LodePNGState state;
    lodepng_state_init( &state );
    state.info_raw.colortype = colortype;
    state.info_raw.bitdepth = 8;
    state.info_raw.palette = ( uint8_t* )palette.data();
    state.info_raw.palettesize = 1ull << bpp;
    state.info_png.color.colortype = colortype;
    state.info_png.color.bitdepth = 8;
    state.info_png.color.palettesize = 1ull << bpp;

    std::vector<uint8_t> indices;

    for ( int y = bounds.miny; y <= bounds.maxy; ++y )
    {
      for ( int x = bounds.minx; x <= bounds.maxx; ++x )
      {
        Color c = image( x, y );
        for ( auto const& cm : colorMapping )
        {
          if ( cm.c == c )
          {
            indices.push_back( uint8_t( cm.idx ) );
            break;
          }
        }
      }
    }

    lodepng_encode( &out, &outsize, indices.data(), ( uint32_t )( bounds.maxx - bounds.minx + 1 ), ( uint32_t )( bounds.maxy - bounds.miny + 1 ), &state );
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

    std::ofstream fout{ *outputImage, std::ios::binary };
    fout.write( ( char* )data.get(), outsize );

    LOG << "Output image written to " << *outputImage;
  }

  {
    std::ofstream fout{ outputSprite, std::ios::binary };

    LOG << "Writing output to " << outputSprite;

    auto off = fout.tellp();

    if ( !separateFiles )
    {
      fout.write( ( char* )penMapping.data(), penMapping.size() );
      LOG << fout.tellp() - off << " bytes of pen mapping data written at offset " << off;

      off = fout.tellp();

      for ( auto const& color : originalColors )
      {
        fout.put( color.g4() );
      }
      LOG << fout.tellp() - off << " bytes of green palette data written at offset " << off;

      off = fout.tellp();

      for ( auto const& color : originalColors )
      {
        fout.put( color.br4() );
      }
      LOG << fout.tellp() - off << " bytes of blue and red palette data written at offset " << off;
    }

    off = fout.tellp();

    if ( literal )
    {
      fout.write( ( char* )literalData.data(), literalData.size() );
      LOG << literalData.size() << " bytes of sprite data written to " << outputSprite << " at offset " << off << ". Packed sprite would have " << packedData.size() << " bytes";
    }
    else
    {
      fout.write( ( char* )packedData.data(), packedData.size() );
      LOG << packedData.size() << " bytes of sprite data written to " << outputSprite << " at offset " << off << ". Literal sprite would have " << literalData.size() << " bytes";
    }
  }

  if ( separateFiles )
  {
    outputSprite.replace_extension( ".pal" );
    std::ofstream fout{ outputSprite };

    fout << "redir:\t.byte ";
    bool firstRedir = true;

    for ( auto const& pen : penMapping )
    {
      if ( firstRedir )
        firstRedir = false;
      else
        fout << ',';
      fout << " $" << std::hex << std::setfill( '0' ) << std::setw( 2 ) << ( int )pen;
    }

    fout << '\n';

    fout << "pal:\t.word ";
    bool firstPal = true;

    for ( auto const& color : originalColors )
    {
      if ( firstPal )
        firstPal = false;
      else
        fout << ',';
      fout << " $" << color.str();
    }

    fout << '\n';

    LOG << "Palette data written to " << outputSprite;
  }

}

