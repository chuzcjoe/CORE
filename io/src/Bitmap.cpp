#include "Bitmap.h"

namespace core {
namespace io {

template class Bitmap<float, 3>;
template class Bitmap<float, 4>;
template class Bitmap<unsigned char, 3>;
template class Bitmap<unsigned char, 4>;

}  // namespace io
}  // namespace core
