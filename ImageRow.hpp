#pragma once

#include "Color.hpp"
class Image;

class ImageRow
{
  Image const& mParent;
  int mRow;
  int mLeft;  //first pixel
  int mRight; //pixel after last pixel

public:

  struct Iterator
  {
    ImageRow const& row;
    int position;

    Color operator*();
    Iterator & operator++();
    friend bool operator!=( Iterator const& left, Iterator const& right )
    {
      return left.row != right.row || left.position != right.position;
    }
  };

  ImageRow( Image const& parent, int row, int left, int right );
  ~ImageRow() = default;

  Iterator begin() const;
  Iterator end() const;
  size_t size() const;

  friend bool operator!=( ImageRow const& left, ImageRow const& right )
  {
    return left.mRow != right.mRow || left.mLeft != right.mLeft || left.mRight != right.mRight;
  }

private:
  Color color( int position ) const;


};

#pragma once
