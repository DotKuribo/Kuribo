#pragma once

#include <types.h>
#include <EASTL/string_view.h>
#include <utility>
#include <debug/assert.h>

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
extern const Node* gNodes;
extern const char* gStrings;

void InitFilesystem();

//! Unlike a STD path, a fs::Path cannot point to a file that does not exist.
//! Construction searches the disc.
class Path {
public:
  Path(const char* string) : Path(string, 0) {}
  Path(eastl::string_view string, u32 search_from = 0);
  Path(const Node* node) : mNode(node) {}
  Path(s32 resolved) {
    KURIBO_ASSERT(gNodes != nullptr && "Call InitFilesystem() first");
    KURIBO_ASSERT(resolved < gNodes[0].folder.sibling_next && "Invalid node");
    mNode = resolved >= 0 ? &gNodes[resolved] : nullptr;
  }
  Path() = default;
  ~Path() = default;

  // No special logic for absolute path
  Path& append(const eastl::string_view path) {
    KURIBO_ASSERT(gNodes != nullptr && "Call InitFilesystem() first");

    return *this = Path(path, mNode != nullptr ? mNode - gNodes : 0);
  }
  Path& operator/=(const eastl::string_view path) {
    return append(path);
  }

  void clear() { mNode = nullptr; }
  //! foo/bar -> foo/
  Path& removeFilename() {
    return *this = getParentPath();
  }
  //! foo/bar -> foo/baz
  Path& replaceFilename(const eastl::string_view path) {
    removeFilename();
    return append(path);
  }
  void swap(Path& other) {
    std::swap(mNode, other.mNode);
  }

  Path getRootPath() const {
    return gNodes;
  }
  Path getParentPath() const {
    return getParentNode();
  }

  const Node* getNode() const { return mNode; }
  s32 getResolved() const {
    KURIBO_ASSERT(gNodes != nullptr && "Call InitFilesystem() first");

    return mNode != nullptr ? mNode - gNodes : -1;
  }

  const Node* getParentNode() const {
    KURIBO_ASSERT(gNodes != nullptr && "Call InitFilesystem() first");

    if (mNode == nullptr) return nullptr;
    if (mNode->is_folder) return gNodes + mNode->folder.parent;

    const Node* it = mNode;
    // Do not need to check lower bound, as the first entry must be a folder (the root)
    while (!it->is_folder)
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

  const char* getName() const {
    return gStrings + mNode->name_offset;
  }

  bool operator==(const Path& rhs) const { return mNode == rhs.mNode; }
  bool operator!=(const Path& rhs) const { return !operator==(rhs); }

private:
  const Node* mNode = nullptr;
};

class RecursiveDirectoryIterator {
public:
  RecursiveDirectoryIterator(Path path) : mPath(path) {}
  RecursiveDirectoryIterator() = default;

  RecursiveDirectoryIterator operator++() {
    mPath = { mPath.getNode() + 1 };
    return *this;
  }
  Path operator*() const { return mPath; }

  bool operator==(const RecursiveDirectoryIterator& rhs) const {
    return mPath == rhs.mPath;
  }
  bool operator!=(const RecursiveDirectoryIterator& rhs) const {
    return !operator==(rhs);
  }

  RecursiveDirectoryIterator begin() const { return mPath; }
  RecursiveDirectoryIterator end() const {
    if (!mPath.getNode()->is_folder) return begin();
    return Path{ gNodes + mPath.getNode()->folder.sibling_next };
  }

private:
  Path mPath = {};
};

} // namespace kuribo::io::fs