/*
Copyright (C) 2012 Sebastian Herbord. All rights reserved.

This file is part of Mod Organizer.

Mod Organizer is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Mod Organizer is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Mod Organizer.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MO_REGISTER_DIRECTORYENTRY_INCLUDED
#define MO_REGISTER_DIRECTORYENTRY_INCLUDED

#include <map>
#include "fileregister.h"
#include <bsatk.h>

#ifndef _WIN32
#include "filetime.h"
#endif

namespace env
{
  class DirectoryWalker;
  struct Directory;
  struct File;
}

namespace std
{
  template <>
  struct hash<MOShared::DirectoryEntryFileKey>
  {
    using argument_type = MOShared::DirectoryEntryFileKey;
    using result_type = std::size_t;

    inline result_type operator()(const argument_type& key) const;
  };
}


namespace MOShared
{

struct DirCompareByName
{
    bool operator()(const DirectoryEntry* a, const DirectoryEntry* b) const;
};


class DirectoryEntry
{
public:
    using SubDirectories = std::set<DirectoryEntry*, DirCompareByName>;

    DirectoryEntry(
    PathStr name, DirectoryEntry* parent, OriginID originID);

  DirectoryEntry(
    PathStr name, DirectoryEntry* parent, OriginID originID,
    boost::shared_ptr<FileRegister> fileRegister,
    boost::shared_ptr<OriginConnection> originConnection);

  ~DirectoryEntry();

  // noncopyable
  DirectoryEntry(const DirectoryEntry&) = delete;
  DirectoryEntry& operator=(const DirectoryEntry&) = delete;

  void clear();

  bool isPopulated() const
  {
    return m_Populated;
  }

  bool isTopLevel() const
  {
    return m_TopLevel;
  }

  bool isEmpty() const
  {
    return m_Files.empty() && m_SubDirectories.empty();
  }

  bool hasFiles() const
  {
    return !m_Files.empty();
  }

  const DirectoryEntry* getParent() const
  {
    return m_Parent;
  }

  // add files to this directory (and subdirectories) from the specified origin.
  // That origin may exist or not
  void addFromOrigin(
    const PathStr& originName,
    const PathStr& directory, int priority, DirectoryStats& stats);

  void addFromOrigin(
    env::DirectoryWalker& walker, const PathStr& originName,
    const PathStr& directory, int priority, DirectoryStats& stats);

  void addFromAllBSAs(
    const PathStr& originName, const PathStr& directory,
    int priority, const std::vector<PathStr>& archives,
    const std::set<PathStr>& enabledArchives,
    const std::vector<PathStr>& loadOrder,
    DirectoryStats& stats);

  void addFromBSA(
    const PathStr& originName, const PathStr& directory,
    const PathStr& archivePath, int priority, int order,
    DirectoryStats& stats);

  void addFromList(
    const PathStr& originName, const PathStr& directory,
    env::Directory& root, int priority, DirectoryStats& stats);

  void propagateOrigin(OriginID origin);

  const PathStr& getName() const
  {
    return m_Name;
  }

  boost::shared_ptr<FileRegister> getFileRegister()
  {
    return m_FileRegister;
  }

  bool originExists(const PathStr& name) const;
  FilesOrigin& getOriginByID(OriginID ID) const;
  FilesOrigin& getOriginByName(const PathStr& name) const;
  const FilesOrigin* findOriginByID(OriginID ID) const;

  OriginID anyOrigin() const;

  std::vector<FileEntryPtr> getFiles() const;

  const SubDirectories& getSubDirectories() const
  {
    return m_SubDirectories;
  }

  template <class F>
  void forEachDirectory(F&& f) const
  {
    for (auto&& d : m_SubDirectories) {
      if (!f(*d)) {
        break;
      }
    }
  }

  template <class F>
  void forEachFile(F&& f) const
  {
    for (auto&& p : m_Files) {
      if (auto file=m_FileRegister->getFile(p.second)) {
        if (!f(*file)) {
          break;
        }
      }
    }
  }

  template <class F>
  void forEachFileIndex(F&& f) const
  {
    for (auto&& p : m_Files) {
      if (!f(p.second)) {
        break;
      }
    }
  }

  FileEntryPtr getFileByIndex(FileIndex index) const
  {
    return m_FileRegister->getFile(index);
  }

  DirectoryEntry* findSubDirectory(
    const PathStr& name, bool alreadyLowerCase=false) const;

  DirectoryEntry* findSubDirectoryRecursive(const PathStr& path);

  /** retrieve a file in this directory by name.
    * @param name name of the file
    * @return fileentry object for the file or nullptr if no file matches
    */
  const FileEntryPtr findFile(const PathStr& name, bool alreadyLowerCase=false) const;
  const FileEntryPtr findFile(const DirectoryEntryFileKey& key) const;

  bool hasFile(const PathStr& name) const;
  bool containsArchive(PathStr archiveName);

  // search through this directory and all subdirectories for a file by the
  // specified name (relative path).
  //
  // if directory is not nullptr, the referenced variable will be set to the
  // path containing the file
  //
  const FileEntryPtr searchFile(
    const PathStr& path, const DirectoryEntry** directory=nullptr) const;

  void removeFile(FileIndex index);

  // remove the specified file from the tree. This can be a path leading to a
  // file in a subdirectory
  bool removeFile(const PathStr& filePath, OriginID* origin = nullptr);

  /**
   * @brief remove the specified directory
   * @param path directory to remove
   */
  void removeDir(const PathStr& path);

  bool remove(const PathStr& fileName, OriginID* origin);

  bool hasContentsFromOrigin(OriginID originID) const;

  FilesOrigin& createOrigin(
    const PathStr& originName,
    const PathStr& directory, int priority, DirectoryStats& stats);

  void removeFiles(const std::set<FileIndex>& indices);

  void dump(const PathStr& file) const;

private:
  using FilesMap = std::map<PathStr, FileIndex>;
  using FilesLookup = std::unordered_map<DirectoryEntryFileKey, FileIndex>;
  using SubDirectoriesLookup = std::unordered_map<PathStr, DirectoryEntry*>;

  boost::shared_ptr<FileRegister> m_FileRegister;
  boost::shared_ptr<OriginConnection> m_OriginConnection;

  PathStr m_Name;
  FilesMap m_Files;
  FilesLookup m_FilesLookup;
  SubDirectories m_SubDirectories;
  SubDirectoriesLookup m_SubDirectoriesLookup;

  DirectoryEntry* m_Parent;
  std::set<OriginID> m_Origins;
  bool m_Populated;
  bool m_TopLevel;
  mutable std::mutex m_SubDirMutex;
  mutable std::mutex m_FilesMutex;
  mutable std::mutex m_OriginsMutex;


  FileEntryPtr insert(
    PathStrView fileName, FilesOrigin& origin, FILETIME fileTime,
    PathStrView archive, int order, DirectoryStats& stats);

  FileEntryPtr insert(
    env::File& file, FilesOrigin& origin,
    PathStrView archive, int order, DirectoryStats& stats);

  void addFiles(
    env::DirectoryWalker& walker, FilesOrigin& origin,
    const PathStr& path, DirectoryStats& stats);

  void addFiles(
    FilesOrigin& origin, BSA::Folder::Ptr archiveFolder, FILETIME fileTime,
    const PathStr& archiveName, int order, DirectoryStats& stats);

  void addDir(FilesOrigin& origin, env::Directory& d, DirectoryStats& stats);

  DirectoryEntry* getSubDirectory(
    PathStrView name, bool create, DirectoryStats& stats,
    OriginID originID = InvalidOriginID);

  DirectoryEntry* getSubDirectory(
    env::Directory& dir, bool create, DirectoryStats& stats,
    OriginID originID = InvalidOriginID);

  DirectoryEntry* getSubDirectoryRecursive(
    const PathStr& path, bool create, DirectoryStats& stats,
    OriginID originID = InvalidOriginID);

  void removeDirRecursive();

  void addDirectoryToList(DirectoryEntry* e, PathStr nameLc);
  void removeDirectoryFromList(SubDirectories::iterator itor);

  void addFileToList(PathStr fileNameLower, FileIndex index);
  void removeFileFromList(FileIndex index);
  void removeFilesFromList(const std::set<FileIndex>& indices);

  struct Context;
  static void onDirectoryStart(Context* cx, PathStrView path);
  static void onDirectoryEnd(Context* cx, PathStrView path);
  static void onFile(Context* cx, PathStrView path, FILETIME ft);

  void dump(std::FILE* f, const PathStr& parentPath) const;
};

} // namespace MOShared


namespace std
{
  hash<MOShared::DirectoryEntryFileKey>::result_type
  hash<MOShared::DirectoryEntryFileKey>::operator()(
    const argument_type& key) const
  {
    return key.hash;
  }
}

#endif // MO_REGISTER_DIRECTORYENTRY_INCLUDED
