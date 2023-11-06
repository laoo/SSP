#include "ProgramOptions.hpp"
#include <iostream>
#include "Ex.hpp"

namespace po = boost::program_options;

ProgramOptions::ProgramOptions( int argc, char const* argv[] )
{
  mDesc.add_options()
    ( "help,h", "produce help message" )
    ( "input,i", po::value<std::string>(), "input file, fist implicit agrument" )
    ( "output,o", po::value<std::string>(), "output file, second implicit argument (default: input with .spr extension)" )
    ( "palette,p", po::value<std::string>(), "image to compute palette from (default: input)" )
    ( "require-palette,r", "report error if palette file does not exist (default: substitue input for palette if it does not exist)" )
    ( "save-optimal-palette,s", po::value<std::string>(), "saves optimal palette under given filename if it does not exist or is older than source palette (default: does nothing)" )
    ( "write-output-image,w", po::value<std::string>(), "write PNG image with reduced palette (default: does nothing)" )
    ( "separate-outputs,t", "write palette and sprite data separate files (default: one file)" )
    ( "frame-width,f", po::value<int>(), "width of one frame of animation, must be a divisor of image width (default: image width)" )
    ( "max-colors,c", po::value<int>(), "maximal number of colors in the palette (default: 16)" )
    ( "bpp,b", po::value<int>(), "forced bits per pixel (default: smallest possible)" )
    ( "literal,l", "do not compress sprite (default: off)" )
    ( "background,g", "sprite is a background sprite (first color does not need to be black/transparent (default: off)" )
    ( "verbose,v", "write verbose information to standard output (default: off)" )
    ;

  mPosDesc
    .add( "input", 1 )
    .add( "output", 1 );


  if ( argc == 1 )
  {
    throw Ex{} << mDesc << "\n";
  }

  try
  {
    po::store( po::command_line_parser( argc, argv ).options( mDesc ).positional( mPosDesc ).run(), mMap );
    po::notify( mMap );
  }
  catch ( std::exception const& e )
  {
    throw Ex{} << "Error parsing command line: " << e.what();
  }

  if ( mMap.count( "help" ) )
  {
    throw Ex{} << mDesc << "\n";
  }

  if ( !mMap.count( "input" ) )
  {
    throw Ex{} << "Input file not specified.\n";
  }

  std::filesystem::path const input = mMap["input"].as<std::string>();
  if ( !std::filesystem::exists( input ) )
  {
    throw Ex{} << "Input file does not exist.\n";
  }

  mInput = std::filesystem::absolute( input );

  if ( mMap.count( "palette" ) )
  {
    std::filesystem::path const palette = mMap["palette"].as<std::string>();

    if ( std::filesystem::exists( palette ) )
    {
      mPalette = std::filesystem::absolute( palette );
    }
  }

  if ( mPalette.empty() )
  {
    if ( mMap.count( "require-palette" ) )
      throw Ex{} << "Palette file does not exist.\n";

    mPalette = mInput;
  }

  if ( mMap.count( "output" ) )
  {
    std::filesystem::path const output = mMap["output"].as<std::string>();

    mOutput = std::filesystem::absolute( output );
  }
  else
  {
    mOutput = mInput;
    mOutput.replace_extension( ".spr" );
  }

}


std::filesystem::path ProgramOptions::input() const
{
  return mInput;
}

std::filesystem::path ProgramOptions::palette() const
{
  return mPalette;
}

std::optional<std::filesystem::path> ProgramOptions::optimalPalette() const
{
  return mMap.count( "save-optimal-palette" ) ? std::optional<std::filesystem::path>{ mMap["save-optimal-palette"].as<std::string>() } : std::nullopt;
}

std::optional<std::filesystem::path> ProgramOptions::outputImage() const
{
  return mMap.count( "write-output-image" ) ? std::optional<std::filesystem::path>{ mMap["write-output-image"].as<std::string>() } : std::nullopt;
}

std::optional<int> ProgramOptions::frameWidth() const
{
  return mMap.count( "frame-width" ) ? std::optional<int>{ mMap["frame-width"].as<int>() } : std::nullopt;
}

std::filesystem::path ProgramOptions::output() const
{
  return mOutput;
}

int ProgramOptions::maxColors() const
{
  if ( mMap.count( "max-colors" ) )
  {
    int result = mMap["max-colors"].as<int>();
    if ( result > 16 || result < 1 )
      throw Ex{} << "Maximal number of colors must be between 1 and 16.\n";

    return result;
  }

  return 16;
}

std::optional<int> ProgramOptions::forcedBPP() const
{
  if ( mMap.count( "bpp" ) )
  {
    int result = mMap["bpp"].as<int>();
    if ( result > 4 || result < 1 )
      throw Ex{} << "Forced number of bits per pixel must be between 1 and 4.\n";
    return result;
  }

  return std::nullopt;
}

bool ProgramOptions::literal() const
{
  return mMap.count( "literal" ) > 0;
}

bool ProgramOptions::backround() const
{
  return mMap.count( "background" ) > 0;
}

bool ProgramOptions::verbose() const
{
  return mMap.count( "verbose" ) > 0;
}

bool ProgramOptions::separateOutput() const
{
  return mMap.count( "separate-outputs" ) > 0;
}
