#include "virtualfiletree.h"

#include "shared/directoryentry.h"
#include "shared/filesorigin.h"
#include "shared/fileentry.h"
// #include "shared/util.h"

using namespace MOBase;
using namespace MOShared;

class VirtualFileTreeImpl : public VirtualFileTree {
public:


  /**
   *
   */
  VirtualFileTreeImpl(std::shared_ptr<const IFileTree> parent, const DirectoryEntry* dir) :
#ifdef _WIN32
    FileTreeEntry(parent, parent ? QString::fromStdWString(dir->getName()) : ""),
#else
    FileTreeEntry(parent, parent ? QString::fromStdString(dir->getName()) : ""),
#endif
    VirtualFileTree(),
    m_dirEntry(dir) { }

protected:

  /**
   * No mutable operations allowed.
   */
  bool beforeReplace(IFileTree const* dstTree, FileTreeEntry const* destination, FileTreeEntry const* source) override { return false; }
  bool beforeInsert(IFileTree const* entry, FileTreeEntry const* name) override { return false; }
  bool beforeRemove(IFileTree const* entry, FileTreeEntry const* name) override { return false; }
  std::shared_ptr<FileTreeEntry> makeFile(std::shared_ptr<const IFileTree> parent, QString name) const override { return nullptr; }
  std::shared_ptr<IFileTree> makeDirectory(std::shared_ptr<const IFileTree> parent, QString name) const override { return nullptr; }

  bool doPopulate(std::shared_ptr<const IFileTree> parent, std::vector<std::shared_ptr<FileTreeEntry>>& entries) const override {
    for (auto* subdirEntry : m_dirEntry->getSubDirectories()) {
      entries.push_back(std::make_shared<VirtualFileTreeImpl>(parent, subdirEntry));
    }
    for (auto& file : m_dirEntry->getFiles()) {
#ifdef _WIN32
      entries.push_back(createFileEntry(parent, QString::fromStdWString(file->getName())));
#else
      entries.push_back(createFileEntry(parent, QString::fromStdString(file->getName())));
#endif
    }

    // Vector is already sorted:
    return true;
  }

  std::shared_ptr<IFileTree> doClone() const {
    return std::make_shared<VirtualFileTreeImpl>(nullptr, m_dirEntry);
  }

private:
  const DirectoryEntry* m_dirEntry;
};

/**
 *
 */
std::shared_ptr<const VirtualFileTree> VirtualFileTree::makeTree(const DirectoryEntry* rootEntry) {
  return std::make_shared<VirtualFileTreeImpl>(nullptr, rootEntry);
}
