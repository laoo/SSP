#include "ImageRow.hpp"
#include "Image.hpp"



ImageRow::ImageRow( Image const& parent, int row, int left, int right ) : mParent{ parent }, mRow{ row }, mLeft{ left }, mRight{ right }
{
}

ImageRow::Iterator ImageRow::begin() const
{
  return Iterator{ *this, mLeft };
}

ImageRow::Iterator ImageRow::end() const
{
  return Iterator{ *this, mRight };
}

size_t ImageRow::size() const
{
  return mRight - mLeft;
}

Color ImageRow::color( int position ) const
{
  return mParent( position, mRow );
}


Color ImageRow::Iterator::operator*()
{
  return row.color( position );
}

ImageRow::Iterator & ImageRow::Iterator::operator++()
{
  position += 1;
  return *this;
}

