#include "originconnection.h"
#include <QObject>
#include "filesorigin.h"
#include "util.h"
#include <log.h>

namespace MOShared
{

using namespace MOBase;

OriginConnection::OriginConnection()
  : m_NextID(0)
{
}

std::pair<FilesOrigin&, bool> OriginConnection::getOrCreate(
  const PathStr &originName, const PathStr &directory, int priority,
  const boost::shared_ptr<FileRegister>& fileRegister,
  const boost::shared_ptr<OriginConnection>& originConnection,
  DirectoryStats& stats)
{
  std::unique_lock lock(m_Mutex);

  auto itor = m_OriginsNameMap.find(originName);

  if (itor == m_OriginsNameMap.end()) {
    FilesOrigin& origin = createOriginNoLock(
      originName, directory, priority, fileRegister, originConnection);

    return {origin, true};
  } else {
    FilesOrigin& origin = m_Origins[itor->second];
    lock.unlock();

    origin.enable(true, stats);
    return {origin, false};
  }
}

FilesOrigin& OriginConnection::createOrigin(
  const PathStr &originName, const PathStr &directory, int priority,
  boost::shared_ptr<FileRegister> fileRegister,
  boost::shared_ptr<OriginConnection> originConnection)
{
  std::scoped_lock lock(m_Mutex);

  return createOriginNoLock(
    originName, directory, priority, fileRegister, originConnection);
}

bool OriginConnection::exists(const PathStr &name)
{
  std::scoped_lock lock(m_Mutex);
  return m_OriginsNameMap.find(name) != m_OriginsNameMap.end();
}

FilesOrigin& OriginConnection::getByID(OriginID ID)
{
  std::scoped_lock lock(m_Mutex);
  return m_Origins[ID];
}

const FilesOrigin* OriginConnection::findByID(OriginID ID) const
{
  std::scoped_lock lock(m_Mutex);

  auto itor = m_Origins.find(ID);

  if (itor == m_Origins.end()) {
    return nullptr;
  } else {
    return &itor->second;
  }
}

FilesOrigin& OriginConnection::getByName(const PathStr &name)
{
  std::scoped_lock lock(m_Mutex);

  auto iter = m_OriginsNameMap.find(name);

  if (iter != m_OriginsNameMap.end()) {
    return m_Origins[iter->second];
  } else {
    std::ostringstream stream;
#ifdef _WIN32
    stream << QObject::tr("invalid origin name: ").toStdString() << ToString(name, true);
#else
    stream << QObject::tr("invalid origin name: ").toStdString() << name;
#endif
    throw std::runtime_error(stream.str());
  }
}

void OriginConnection::changeNameLookup(const PathStr &oldName, const PathStr &newName)
{
  std::scoped_lock lock(m_Mutex);

  auto iter = m_OriginsNameMap.find(oldName);

  if (iter != m_OriginsNameMap.end()) {
    OriginID idx = iter->second;
    m_OriginsNameMap.erase(iter);
    m_OriginsNameMap[newName] = idx;
  } else {
    log::error(QObject::tr("failed to change name lookup from {} to {}").toStdString(), oldName, newName);
  }
}

OriginID OriginConnection::createID()
{
  return m_NextID++;
}

FilesOrigin& OriginConnection::createOriginNoLock(
  const PathStr &originName, const PathStr &directory, int priority,
  boost::shared_ptr<FileRegister> fileRegister,
  boost::shared_ptr<OriginConnection> originConnection)
{
  OriginID newID = createID();

  auto itor = m_Origins.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(newID),
    std::forward_as_tuple(
      newID, originName, directory, priority,
      fileRegister, originConnection))
    .first;

  m_OriginsNameMap.insert({originName, newID});

  return itor->second;
}

} // namespace
