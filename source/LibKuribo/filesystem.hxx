#pragma once

#include <EASTL/string_view.h>
#include <debug/assert.h>
#include <functional>
#include <types.h>
#include <utility>

namespace kuribo::io::fs {

struct Node {
#ifdef _WIN32
  union {
    u32 packed;
    struct {
      u32 name_offset : 24;
      u32 is_folder : 8;
    };
  };
#else
  u32 is_folder : 8;
  u32 name_offset : 24;
#endif
  union {
    struct {
      u32 offset;
      u32 size;
    } file;
    struct {
      u32 parent;
      u32 sibling_next;
    } folder;
  };
};
static_assert(sizeof(Node) == 12);

// The filesystem itself is one giant folder
struct rvl_os_filesystem_root {};

// More like a folder
struct DiscFileSystem {
public:
  DiscFileSystem(rvl_os_filesystem_root);
  ~DiscFileSystem();

  DiscFileSystem(const Node* nodes, const char* strings)
      : mNodes(nodes), mStrings(strings) {}

  DiscFileSystem(const DiscFileSystem&) = default;

  inline u32 NumEntries() const { return mNodes[0].folder.sibling_next; }
  inline bool IsEntryValid(u32 entry_id) const {
    return entry_id < NumEntries();
  }

  const Node& getChild(u32 child_index) const {
    KURIBO_ASSERT(IsEntryValid(child_index) && "Invalid child index");
    return mNodes[child_index];
  }
  const char* getStringById(u32 id) const { return mStrings + id; }

  const Node* getNodes() const { return mNodes; }

private:
  const Node* mNodes;
  const char* mStrings;

  // In the windows emulator, DiscFileSystem(rvl_os_filesystem_root) allocates
  // memory.
#ifdef _WIN32
  bool mNodesOwned = false;
#endif
};

//! Unlike a STD path, a fs::Path cannot point to a file that does not exist.
//! Construction searches the disc.
class Path {
public:
  Path(const DiscFileSystem& fs, const Node* node) : mFs(&fs), mNode(node) {}
  Path(const DiscFileSystem& fs, s32 resolved) : mFs(&fs) {
    mNode = resolved >= 0 ? &fs.getChild(resolved) : nullptr;
  }

  // Search parent for path
  Path(const DiscFileSystem& fs, eastl::string_view string,
       u32 search_from = 0);

  Path(const Path&) = default;

  Path& operator=(const Path&) = default;

  Path() = default;
  ~Path() = default;

  // No special logic for absolute path
  Path& append(const eastl::string_view path) {
    return *this =
               Path(*mFs, path, mNode != nullptr ? mNode - mFs->getNodes() : 0);
  }
  Path& operator/=(const eastl::string_view path) { return append(path); }

  void clear() { mNode = nullptr; }
  //! foo/bar -> foo/
  Path& removeFilename() { return *this = getParentPath(); }
  //! foo/bar -> foo/baz
  Path& replaceFilename(const eastl::string_view path) {
    removeFilename();
    return append(path);
  }
  void swap(Path& other) { std::swap(mNode, other.mNode); }

  Path getRootPath() const { return {*mFs, mFs->getNodes()}; }
  Path getParentPath() const { return {*mFs, getParentNode()}; }

  const Node* getNode() const { return mNode; }
  const DiscFileSystem& getFs() const { return *mFs; }

  s32 getResolved() const {
    return mNode != nullptr ? mNode - mFs->getNodes() : -1;
  }

  const Node* getParentNode() const {
    if (mNode == nullptr)
      return nullptr;
    if (mNode->is_folder)
      return &mFs->getChild(mNode->folder.parent);

    const Node* it = mNode;
    while (it > mFs->getNodes() && !it->is_folder)
      --it;

    return it;
  }

  bool isFolder() const {
    KURIBO_ASSERT(mNode != nullptr);
    return mNode->is_folder;
  }

  bool isFile() const {
    KURIBO_ASSERT(mNode != nullptr);
    return !mNode->is_folder;
  }

  const char* getName() const { return mFs->getStringById(mNode->name_offset); }

  bool operator==(const Path& rhs) const { return mNode == rhs.mNode; }
  bool operator!=(const Path& rhs) const { return !operator==(rhs); }

private:
  const DiscFileSystem* mFs;
  const Node* mNode = nullptr;
};

class RecursiveDirectoryIterator {
public:
  RecursiveDirectoryIterator(Path path) : mPath(path) {}
  RecursiveDirectoryIterator() = default;

  RecursiveDirectoryIterator operator++() {
    mPath = {mPath.getFs(), mPath.getNode() + 1};
    return *this;
  }
  Path operator*() const { return mPath; }

  bool operator==(const RecursiveDirectoryIterator& rhs) const {
    return mPath == rhs.mPath;
  }
  bool operator!=(const RecursiveDirectoryIterator& rhs) const {
    return !operator==(rhs);
  }

  RecursiveDirectoryIterator begin() const {
    return Path{mPath.getFs(), mPath.getNode() + 1};
  }
  RecursiveDirectoryIterator end() const {
    if (!mPath.getNode()->is_folder)
      return begin();
    return Path{mPath.getFs(),
                static_cast<s32>(mPath.getNode()->folder.sibling_next)};
  }

private:
  Path mPath;
};

} // namespace kuribo::io::fs