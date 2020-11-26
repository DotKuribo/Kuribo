#include "filesystem.hxx"
#include <debug/log.h>

static inline u32 swap32(u32 v);
static inline u16 swap16(u16 v);

#define _BSWAP_16(v) (((v & 0xff00) >> 8) | ((v & 0x00ff) << 8))

#define _BSWAP_32(v)                                                           \
  (((v & 0xff000000) >> 24) | ((v & 0x00ff0000) >> 8) |                        \
   ((v & 0x0000ff00) << 8) | ((v & 0x000000ff) << 24))

#if defined(__llvm__) || (defined(__GNUC__) && !defined(__ICC))
static inline u32 swap32(u32 v) { return __builtin_bswap32(v); }
static inline u16 swap16(u16 v) { return _BSWAP_16(v); }
#elif defined(_MSC_VER)
#include <stdlib.h>
static inline u32 swap32(u32 v) { return _byteswap_ulong(v); }
static inline u16 swap16(u16 v) { return _byteswap_ushort(v); }
#else
static inline u32 swap32(u32 v) { return _BSWAP_32(v); }
static inline u16 swap16(u16 v) { return _BSWAP_16(v); }
#endif

namespace kuribo::io::fs {

const Node* gNodes;
const char* gStrings;

void InitFilesystem() {
#ifdef _WIN32
  FILE* pFile = fopen("FST.bin", "rb");
  gNodes = (Node*)malloc(0x1cee0);
  fread((void*)gNodes, 0x1cee0, 1, pFile);

  const u32 next = swap32(gNodes->folder.sibling_next);

  for (int i = 0; i < next; ++i) {
    Node* node = (Node*)gNodes + i;
    //    node->is_folder   = swap32(node->is_folder);
    node->packed = swap32(node->packed);
    node->file.offset = swap32(node->file.offset);
    node->file.size = swap32(node->file.size);
  }
  fclose(pFile);
#else // HW
  gNodes = *reinterpret_cast<const Node**>(0x80000038);
#endif
  gStrings = reinterpret_cast<const char*>(gNodes + gNodes[0].folder.sibling_next);
}

inline bool equalsIgnoreCase(char a, char b) {
  const s32 delta = a - b;
  const s32 abs_delta = delta > 0 ? delta : -delta;
  return delta == 0 || abs_delta == ('a' - 'A');
}

inline bool pathCompare(const eastl::string_view& lhs, const eastl::string_view& rhs) {
  // Left -- full path
  // Right -- fragment
  if (lhs.size() < rhs.size()) return false;

  for (u32 i = 0; i < rhs.size(); ++i) {
    if (!equalsIgnoreCase(lhs[i], rhs[i]))
      return false;
  }

  return lhs.size() == rhs.size() || lhs[rhs.size()] == '/';
}

// Path search based on WiiCore
s32 resolvePath(const Node* nodes, eastl::string_view path, u32 search_from) {
  u32 it = search_from;
  while (true) {
    KURIBO_LOG("Searching from %s\n", gStrings + gNodes[it].name_offset);
    // End of string -> return what we have
    if (path.empty())
      return it;

    // Ignore initial slash: /Path/File vs Path/File
    if (path[0] == '/') {
      it = 0;
      path = path.substr(1);
      continue;
    }

    // Handle special cases:
    // -../-, -.., -./-, -.
    if (path[0] == '.') {
      if (path[1] == '.') {
        // Seek to parent ../
        if (path[2] == '/') {
          it = nodes[it].folder.parent;
          path = path.substr(3);
          continue;
        }
        // Return parent folder immediately
        if (path.size() == 2)
          return nodes[it].folder.parent;

        goto compare;
      }

      // "." directory does nothing
      if (path[1] == '/') {
        path = path.substr(2);
        continue;
      }

      // Ignore trailing dot
      if (path.size() == 1)
        return it;
    }

  compare:
    const bool name_delimited_by_slash = path.rfind('/') != eastl::string_view::npos;

    // Traverse all children of the parent.
    const u32 anchor = it;
    ++it;
    while (it < nodes[anchor].folder.sibling_next) {
      while (true) {
        if (nodes[it].is_folder || !name_delimited_by_slash) {
          eastl::string_view name_of_it = gStrings + nodes[it].name_offset;

          // Skip empty directories
          if (name_of_it == eastl::string_view(".")) {
            ++it;
            continue;
          }

          // Advance to the next item in the path
          if (pathCompare(path, name_of_it)) {
            // If the path was truncated, there is nowhere else to go
            if (!name_delimited_by_slash)
              return it;
            path = path.substr(name_of_it.size() + 1);
            goto descend;
          }
        }

        if (nodes[it].is_folder) {
          it = nodes[it].folder.sibling_next;
          break;
        }

        ++it;
        break;
      }
    }

    return -1;

  descend:;
  }
}

Path::Path(eastl::string_view path, u32 search_from) {
  const s32 resolved = resolvePath(gNodes, path, search_from);
  mNode = resolved >= 0 ? gNodes + resolved : nullptr;
}
} // namespace kuribo::io::fs