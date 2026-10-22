#ifndef EMULATOR_SRC_UWP_PKGFORMAT_H_
#define EMULATOR_SRC_UWP_PKGFORMAT_H_

#include <cstddef>
#include <cstdint>

namespace Kyty::Uwp {

// Checks only the four-byte PS4 PKG signature. This does not decrypt, unpack, or load a package.
[[nodiscard]] bool HasPs4PkgMagic(const uint8_t* data, size_t size);

} // namespace Kyty::Uwp

#endif // EMULATOR_SRC_UWP_PKGFORMAT_H_
