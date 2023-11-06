#pragma once

#include <vector>
#include <filesystem>
#include "Color.hpp"

class PNG;
  
class Palette
{
public:
  Palette( std::filesystem::path const& path, int maxColors, std::optional<std::filesystem::path> optimalPalette );
  Palette( Palette const& ) = delete;
  Palette( Palette && ) = default;

  size_t size() const;
  size_t mapNearest( Color src ) const;

  std::vector<Color> const& colors() const;

private:
  std::vector<Color> mColors;
};

