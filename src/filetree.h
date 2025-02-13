#ifndef MODORGANIZER_FILETREE_INCLUDED
#define MODORGANIZER_FILETREE_INCLUDED

#include "modinfo.h"
#include "modinfodialogfwd.h"

QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QTreeView)

namespace MOShared { class FileEntry; }

class OrganizerCore;
class PluginContainer;
class FileTreeModel;
class FileTreeItem;

class FileTree : public QObject
{
  Q_OBJECT;

public:
  FileTree(OrganizerCore& core, PluginContainer& pc, QTreeView* tree);

  FileTreeModel* model();
  void refresh();
  void clear();

  bool fullyLoaded() const;
  void ensureFullyLoaded();

  void expandAll();
  void collapseAll();

  void open(FileTreeItem* item=nullptr);
  void openHooked(FileTreeItem* item=nullptr);
  void preview(FileTreeItem* item=nullptr);
  void activate(FileTreeItem* item=nullptr);

  void addAsExecutable(FileTreeItem* item=nullptr);
  void exploreOrigin(FileTreeItem* item=nullptr);
  void openModInfo(FileTreeItem* item=nullptr);

  void hide(FileTreeItem* item=nullptr);
  void unhide(FileTreeItem* item=nullptr);

  void dumpToFile() const;

signals:
  void executablesChanged();
  void originModified(int originID);
  void displayModInformation(ModInfo::Ptr m, unsigned int i, ModInfoTabIDs tab);

private:
  OrganizerCore& m_core;
  PluginContainer& m_plugins;
  QTreeView* m_tree;
  FileTreeModel* m_model;

  FileTreeItem* singleSelection();

  void onExpandedChanged(const QModelIndex& index, bool expanded);
  void onItemActivated(const QModelIndex& index);
  void onContextMenu(const QPoint &pos);
  bool showShellMenu(QPoint pos);

  void addDirectoryMenus(QMenu& menu, FileTreeItem& item);
  void addFileMenus(QMenu& menu, const MOShared::FileEntry& file, int originID);
  void addOpenMenus(QMenu& menu, const MOShared::FileEntry& file);
  void addCommonMenus(QMenu& menu);

  void toggleVisibility(bool b, FileTreeItem* item=nullptr);

  QModelIndex proxiedIndex(const QModelIndex& index);
};

#endif // MODORGANIZER_FILETREE_INCLUDED
