#include "pkgFormat.h"

#include <array>
#include <cstdint>

namespace Kyty::Uwp {

namespace {

constexpr std::array<uint8_t, 4> PKG_MAGIC {0x7f, 0x43, 0x4e, 0x54}; // "\x7fCNT"

} // namespace

bool HasPs4PkgMagic(const uint8_t* data, size_t size) {
	if (data == nullptr || size < PKG_MAGIC.size()) {
		return false;
	}
	for (size_t i = 0; i < PKG_MAGIC.size(); ++i) {
		if (data[i] != PKG_MAGIC[i]) {
			return false;
		}
	}
	return true;
}

} // namespace Kyty::Uwp
