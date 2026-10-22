#ifndef EMULATOR_SRC_UWP_GAMESOURCE_H_
#define EMULATOR_SRC_UWP_GAMESOURCE_H_

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

// A game is a folder (with an eboot.bin) or a .zar archive (a compressed dump the emulator reads in place). These helpers read the files of
// either: a path below a game folder, or a path inside an archive (<archive>.zar\sce_sys\param.json).
namespace Kyty::Uwp {

// A .zar file (by its name).
[[nodiscard]] bool IsArchivePath(const std::filesystem::path& path);

// Whether the file exists.
[[nodiscard]] bool GameFileExists(const std::filesystem::path& path);
// The file, up to 32 MB, or nothing when it can't be read.
[[nodiscard]] std::optional<std::vector<uint8_t>> ReadGameBinary(const std::filesystem::path& path);
[[nodiscard]] std::optional<std::string>          ReadGameText(const std::filesystem::path& path);

enum class PkgHeaderKind { Unknown, Ps4Cnt, Ps5Fih, Ps5Pkg };
struct PkgInspection {
	PkgHeaderKind kind = PkgHeaderKind::Unknown;
	uint64_t file_size = 0;
	bool structure_valid = false;
	uint32_t entry_count = 0;
	uint64_t entry_table_offset = 0;
	uint64_t entry_table_size = 0;
	std::string content_id;
	uint32_t drm_type = 0;
	bool pfs_entry_found = false;
	uint32_t encrypted_entry_count = 0;
	std::wstring detail;
};
// Reads package markers and performs bounded header/table checks when the format matches a known layout. This does not validate signatures, decrypt, extract, install, or run a package.
[[nodiscard]] std::optional<PkgInspection> InspectPkgFile(const std::filesystem::path& path);
// The .pkg files directly in folder.
[[nodiscard]] std::vector<std::filesystem::path> PackagesIn(const std::filesystem::path& folder);

// The .zar files directly in `folder`.
[[nodiscard]] std::vector<std::filesystem::path> ArchivesIn(const std::filesystem::path& folder);
// EXFAT image files directly in `folder`; detection does not mount or install them.
[[nodiscard]] std::vector<std::filesystem::path> ExfatImagesIn(const std::filesystem::path& folder);

} // namespace Kyty::Uwp

#endif // EMULATOR_SRC_UWP_GAMESOURCE_H_
