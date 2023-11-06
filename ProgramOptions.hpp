#pragma once

#include <boost/program_options.hpp>
#include <string>
#include <filesystem>
#include <memory>
#include <optional>

class ProgramOptions
{
public:
  ProgramOptions( int argc, char const* argv[] );
  ProgramOptions( ProgramOptions const& ) = delete;
  ProgramOptions& operator=( ProgramOptions const& ) = delete;

  std::filesystem::path input() const;
  std::filesystem::path palette() const;
  std::optional<std::filesystem::path> optimalPalette() const;
  std::filesystem::path output() const;
  std::optional<std::filesystem::path> outputImage() const;
  std::optional<int> frameWidth() const;
  int maxColors() const;
  std::optional<int> forcedBPP() const;
  bool literal() const;
  bool backround() const;
  bool verbose() const;
  bool separateOutput() const;
  bool noSpriteGen() const;

private:
  boost::program_options::variables_map mMap = {};
  boost::program_options::options_description mDesc = { "Suzy Sprite Packer" };
  boost::program_options::positional_options_description mPosDesc = {};

  std::filesystem::path mInput = {};
  std::filesystem::path mPalette = {};
  std::filesystem::path mOutput = {};
};
