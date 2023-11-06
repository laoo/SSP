#pragma once

#include "Image.hpp"
#include "Palette.hpp"

struct Bounds
{
  int minx;
  int miny;
  int maxx;
  int maxy;
};

void createSprite( Image const& image, Bounds const& bounds, Palette const& pal, std::filesystem::path outputSprite, std::optional<std::filesystem::path> outputImage, std::optional<int> forcedBPP,
  bool background, bool literal, bool verbose, bool separateFiles );
