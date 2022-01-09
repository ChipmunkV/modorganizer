#ifndef MO_REGISTER_ORIGINCONNECTION_INCLUDED
#define MO_REGISTER_ORIGINCONNECTION_INCLUDED

#include <atomic>
#include <map>

#include "fileregisterfwd.h"

namespace MOShared
{

class OriginConnection
{
public:
  OriginConnection();

  // noncopyable
  OriginConnection(const OriginConnection&) = delete;
  OriginConnection& operator=(const OriginConnection&) = delete;

  std::pair<FilesOrigin&, bool> getOrCreate(
    const PathStr &originName, const PathStr &directory, int priority,
    const boost::shared_ptr<FileRegister>& fileRegister,
    const boost::shared_ptr<OriginConnection>& originConnection,
    DirectoryStats& stats);

  FilesOrigin& createOrigin(
    const PathStr &originName, const PathStr &directory, int priority,
    boost::shared_ptr<FileRegister> fileRegister,
    boost::shared_ptr<OriginConnection> originConnection);

  bool exists(const PathStr &name);

  FilesOrigin &getByID(OriginID ID);
  const FilesOrigin* findByID(OriginID ID) const;
  FilesOrigin &getByName(const PathStr &name);

  void changePriorityLookup(int oldPriority, int newPriority);

  void changeNameLookup(const PathStr &oldName, const PathStr &newName);

private:
  std::atomic<OriginID> m_NextID;
  std::map<OriginID, FilesOrigin> m_Origins;
  std::map<PathStr, OriginID> m_OriginsNameMap;
  mutable std::mutex m_Mutex;

  OriginID createID();

  FilesOrigin& createOriginNoLock(
    const PathStr &originName, const PathStr &directory, int priority,
    boost::shared_ptr<FileRegister> fileRegister,
    boost::shared_ptr<OriginConnection> originConnection);
};

} // namespace

#endif // MO_REGISTER_ORIGINCONNECTION_INCLUDED
