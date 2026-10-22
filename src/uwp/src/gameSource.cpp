#include "gameSource.h"

#include "fileAccess.h"
#include "log.h"

#include <windows.h>
#include <fileapifromapp.h>
#include <fcntl.h>
#include <io.h>

#include <winrt/base.h>

#include <zarchive/zarchivereader.h>

#include <algorithm>
#include <cstdio>
#include <cwctype>
#include <fstream>
#include <memory>
#include <mutex>

namespace Kyty::Uwp {

namespace {

constexpr size_t   MAX_OPEN_ARCHIVES = 4;
constexpr uint64_t MAX_FILE_SIZE     = uint64_t {32} << 20;

struct OpenArchive {
	std::wstring                    key;
	std::shared_ptr<ZArchiveReader> reader;
};

std::mutex               g_mutex;
std::vector<OpenArchive> g_open; // most recently used last

// Opens the archive's file with the *FromApp functions (the Xbox lets the app read a USB drive only that way; a plain C++ stream cannot open the
// file there) and gives the reader a stream on it. The stream does not own the C file, so the reader's deleter closes it.
std::shared_ptr<ZArchiveReader> OpenReader(const std::filesystem::path& path) {
	HANDLE handle = CreateFile2FromAppW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, nullptr);
	if (handle == INVALID_HANDLE_VALUE) {
		Log("cannot open archive %ls (error %lu)\n", path.c_str(), GetLastError());
		return nullptr;
	}
	const int descriptor = _open_osfhandle(reinterpret_cast<intptr_t>(handle), _O_RDONLY | _O_BINARY);
	if (descriptor < 0) {
		CloseHandle(handle);
		Log("cannot open archive %ls (no C file descriptor)\n", path.c_str());
		return nullptr;
	}
	FILE* file = _fdopen(descriptor, "rb");
	if (file == nullptr) {
		_close(descriptor);
		Log("cannot open archive %ls (no C file)\n", path.c_str());
		return nullptr;
	}
	std::ifstream   stream(file);
	ZArchiveReader* reader = ZArchiveReader::OpenFromStream(std::move(stream));
	if (reader == nullptr) {
		fclose(file);
		Log("archive %ls is not a readable .zar\n", path.c_str());
		return nullptr;
	}
	return std::shared_ptr<ZArchiveReader>(reader, [file](ZArchiveReader* archive) {
		delete archive;
		fclose(file);
	});
}

// The archive at `path`, kept open for the next read (opening reads its index).
std::shared_ptr<ZArchiveReader> Open(const std::filesystem::path& path) {
	std::lock_guard lock(g_mutex);
	const auto      key = path.wstring();
	for (auto it = g_open.begin(); it != g_open.end(); ++it) {
		if (it->key == key) {
			std::rotate(it, it + 1, g_open.end());
			return g_open.back().reader;
		}
	}
	auto reader = OpenReader(path);
	if (!reader) {
		return nullptr;
	}
	if (g_open.size() >= MAX_OPEN_ARCHIVES) {
		g_open.erase(g_open.begin());
	}
	g_open.push_back({key, reader});
	return reader;
}

// Splits `path` at an archive that is a file: the archive and the path inside it, with forward slashes.
bool SplitArchivePath(const std::filesystem::path& path, std::filesystem::path& archive, std::string& inner) {
	std::filesystem::path walked;
	bool                  found = false;
	for (const auto& part: path) {
		if (found) {
			inner += inner.empty() ? "" : "/";
			inner += winrt::to_string(part.wstring());
			continue;
		}
		walked /= part;
		if (IsArchivePath(walked) && IsFile(walked)) {
			archive = walked;
			found   = true;
		}
	}
	return found;
}

} // namespace

bool IsArchivePath(const std::filesystem::path& path) {
	auto extension = path.extension().wstring();
	std::transform(extension.begin(), extension.end(), extension.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
	return extension == L".zar";
}

bool IsPkgPath(const std::filesystem::path& path) {
	auto extension = path.extension().wstring();
	std::transform(extension.begin(), extension.end(), extension.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
	return extension == L".pkg";
}

namespace {
bool HasPackageMagic(const std::filesystem::path& path, const uint8_t (&expected)[4]) {
	HANDLE file = CreateFile2FromAppW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, nullptr);
	if (file == INVALID_HANDLE_VALUE) {
		return false;
	}
	uint8_t magic[4] {};
	DWORD read = 0;
	const bool ok = ReadFile(file, magic, sizeof(magic), &read, nullptr) != 0 && read == sizeof(magic);
	CloseHandle(file);
	return ok && magic[0] == expected[0] && magic[1] == expected[1] && magic[2] == expected[2] && magic[3] == expected[3];
}
} // namespace

bool HasPs4PkgSignature(const std::filesystem::path& path) {
	const uint8_t magic[4] {0x7f, 'C', 'N', 'T'};
	return HasPackageMagic(path, magic);
}

bool HasPs5PkgSignature(const std::filesystem::path& path) {
	// PS5 finalized installable packages use FIH at offset 0; CNT is the embedded metadata container.
	const uint8_t magic[4] {0x7f, 'F', 'I', 'H'};
	return HasPackageMagic(path, magic);
}

bool IsPs5PkgHeaderStructurallyValid(const std::filesystem::path& path) {
	// FIH header fields are little-endian u64s. Only read the fixed header region needed below.
	constexpr DWORD HEADER_SIZE = 0xa8;
	HANDLE file = CreateFile2FromAppW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, nullptr);
	if (file == INVALID_HANDLE_VALUE) {
		return false;
	}
	LARGE_INTEGER physicalSize {};
	uint8_t header[HEADER_SIZE] {};
	DWORD read = 0;
	const bool readable = GetFileSizeEx(file, &physicalSize) != 0 && physicalSize.QuadPart >= HEADER_SIZE &&
	                      ReadFile(file, header, HEADER_SIZE, &read, nullptr) != 0 && read == HEADER_SIZE;
	CloseHandle(file);
	if (!readable || physicalSize.QuadPart < 0x10000 || header[0] != 0x7f || header[1] != 'F' || header[2] != 'I' ||
	    header[3] != 'H' || (header[5] != 0x00 && header[5] != 0x80)) {
		return false;
	}
	const auto readU64 = [&](size_t offset) -> uint64_t {
		uint64_t value = 0;
		for (size_t i = 0; i < 8; ++i) {
			value |= static_cast<uint64_t>(header[offset + i]) << (i * 8);
		}
		return value;
	};
	const uint64_t fileSize = static_cast<uint64_t>(physicalSize.QuadPart);
	const uint64_t pfsOffset = readU64(0x10);
	const uint64_t pfsSize = readU64(0x18);
	const uint64_t cntOffset = readU64(0x58);
	const uint64_t cntSize = readU64(0xa0);
	// Subtraction-based bounds checks avoid offset+size overflow.
	// The finalized format places PFS at 0x10000 and the embedded CNT after the PFS segment.
	if (pfsOffset != 0x10000 || pfsSize == 0 || pfsSize > fileSize - pfsOffset) {
		return false;
	}
	const uint64_t pfsEnd = pfsOffset + pfsSize;
	if (cntOffset < pfsEnd || cntOffset > fileSize || cntSize < 4 || cntSize > fileSize - cntOffset) {
		return false;
	}

	// Confirm the embedded metadata segment really starts with CNT, not just a plausible offset.
	HANDLE fileForCnt = CreateFile2FromAppW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, nullptr);
	if (fileForCnt == INVALID_HANDLE_VALUE) {
		return false;
	}
	LARGE_INTEGER position {};
	position.QuadPart = static_cast<LONGLONG>(cntOffset);
	uint8_t cntMagic[4] {};
	DWORD cntRead = 0;
	const bool seekOk = SetFilePointerEx(fileForCnt, position, nullptr, FILE_BEGIN) != 0;
	const bool cntOk = seekOk && ReadFile(fileForCnt, cntMagic, sizeof(cntMagic), &cntRead, nullptr) != 0 &&
	                   cntRead == sizeof(cntMagic);
	CloseHandle(fileForCnt);
	return cntOk && cntMagic[0] == 0x7f && cntMagic[1] == 'C' && cntMagic[2] == 'N' && cntMagic[3] == 'T';
}

bool GameFileExists(const std::filesystem::path& path) {
	std::filesystem::path archive;
	std::string           inner;
	if (!SplitArchivePath(path, archive, inner)) {
		return IsFile(path);
	}
	const auto reader = Open(archive);
	return reader && reader->LookUp(inner, true, false) != ZARCHIVE_INVALID_NODE;
}

std::optional<std::vector<uint8_t>> ReadGameBinary(const std::filesystem::path& path) {
	std::filesystem::path archive;
	std::string           inner;
	if (!SplitArchivePath(path, archive, inner)) {
		return ReadBinaryFile(path);
	}
	const auto reader = Open(archive);
	if (!reader) {
		return std::nullopt;
	}
	const auto node = reader->LookUp(inner, true, false);
	if (node == ZARCHIVE_INVALID_NODE) {
		return std::nullopt;
	}
	const auto size = reader->GetFileSize(node);
	if (size > MAX_FILE_SIZE) {
		return std::nullopt;
	}
	std::vector<uint8_t> data(static_cast<size_t>(size));
	if (reader->ReadFromFile(node, 0, size, data.data()) != size) {
		return std::nullopt;
	}
	return data;
}

std::optional<std::string> ReadGameText(const std::filesystem::path& path) {
	auto data = ReadGameBinary(path);
	if (!data) {
		return std::nullopt;
	}
	return std::string(data->begin(), data->end());
}

namespace {
std::vector<std::filesystem::path> FilesWithPattern(const std::filesystem::path& folder, const wchar_t* pattern) {
	std::vector<std::filesystem::path> result;
	WIN32_FIND_DATAW data {};
	HANDLE find = FindFirstFileExFromAppW((folder / pattern).c_str(), FindExInfoBasic, &data, FindExSearchNameMatch, nullptr, 0);
	if (find == INVALID_HANDLE_VALUE) {
		return result;
	}
	do {
		if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
			result.push_back(folder / data.cFileName);
		}
	} while (FindNextFileW(find, &data));
	FindClose(find);
	std::sort(result.begin(), result.end());
	return result;
}
} // namespace

std::vector<std::filesystem::path> ArchivesIn(const std::filesystem::path& folder) {
	return FilesWithPattern(folder, L"*.zar");
}

std::vector<std::filesystem::path> PackagesIn(const std::filesystem::path& folder) {
	return FilesWithPattern(folder, L"*.pkg");
}

} // namespace Kyty::Uwp
