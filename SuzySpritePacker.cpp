
#include <iostream>
#include <cstdio>
#include "Ex.hpp"
#include "Palette.hpp"
#include "Image.hpp"
#include "Sprite.hpp"
#include "Log.hpp"

#include "ProgramOptions.hpp"

int main( int argc, char const* argv[] )
{
  try
  {
    ProgramOptions options{ argc, argv };

    Log::instance().verbose( options.verbose() );

    LOG << "Suzy Sprite Packer";

    Palette palette{ options.palette(), options.maxColors(), options.optimalPalette() };
    Image image{ options.input() };
    Bounds bounds{ 0, 0, image.width() - 1, image.height() - 1 };
    LOG << "Image " << options.input() << " dimensions: " << image.width() << "x" << image.height();

    if ( options.frameWidth() )
    {
      int width = options.frameWidth().value();
      LOG << "Frame width: " << width;

      if ( image.width() % width != 0 )
        throw Ex{} << "Image dimensions are not evenly divisible by frame width.\n";

      int frames = image.width() / width;

      LOG << "Number of frames in animation: " << frames;

      bounds.maxx = width - 1;
      int maxFrameLength = std::snprintf( nullptr, 0, "%d", frames - 1 );
      for ( int i = 0; i < frames; ++i )
      {
        std::string outputPath;
        std::string outputImagePath;
        {
          std::stringstream ss;
          ss << options.output().replace_extension().string() << "_" << std::setfill( '0' ) << std::setw( maxFrameLength ) << i << options.output().extension().string();
          outputPath = ss.str();
        }
        if ( options.outputImage() )
        {
          std::stringstream ss;
          ss << options.outputImage()->replace_extension().string() << "_" << std::setfill( '0' ) << std::setw( maxFrameLength ) << i << options.outputImage()->extension().string();
          outputImagePath = ss.str();
        }
        createSprite( image, bounds, palette, outputPath, options.outputImage() ? std::optional<std::filesystem::path>{ outputImagePath } : std::optional<std::filesystem::path>{}, options.forcedBPP(), options.backround(), options.literal(), options.verbose(), options.separateOutput()  );
        bounds.minx += width;
        bounds.maxx += width;
      }
    }
    else
    {
      createSprite( image, bounds, palette, options.output(), options.outputImage(), options.forcedBPP(), options.backround(), options.literal(), options.verbose(), options.separateOutput() );
    }
  }
  catch ( Ex const& e )
  {
    std::cerr << e.what();
    return 1;
  }

  return 0;
}
