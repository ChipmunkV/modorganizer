#ifndef MODORGANIZER_DATATAB_INCLUDED
#define MODORGANIZER_DATATAB_INCLUDED

#include "modinfodialogfwd.h"
#include "modinfo.h"
#include <filterwidget.h>
#include <QPushButton>
#include <QTreeWidget>
#include <QCheckBox>

namespace Ui { class MainWindow; }
class OrganizerCore;
class Settings;
class PluginContainer;
class FileTree;

namespace MOShared { class DirectoryEntry; }

class DataTab : public QObject
{
  Q_OBJECT;

public:
  DataTab(
    OrganizerCore& core, PluginContainer& pc,
    QWidget* parent, Ui::MainWindow* ui);
  virtual ~DataTab();

  void saveState(Settings& s) const;
  void restoreState(const Settings& s);
  void activated();

  // if the data tab is currently visible, trigger an update of the
  // tree, otherwise mark the tree has modified and will refresh when
  // the tab is activated
  //
  void updateTree();

signals:
  void executablesChanged();
  void originModified(int originID);
  void displayModInformation(ModInfo::Ptr m, unsigned int i, ModInfoTabIDs tab);

private:
  struct DataTabUi
  {
    QTabWidget* tabs;
    QWidget* tab;
    QPushButton* refresh;
    QTreeView* tree;
    QCheckBox* conflicts;
    QCheckBox* archives;
  };

  OrganizerCore& m_core;
  PluginContainer& m_pluginContainer;
  QWidget* m_parent;
  DataTabUi ui;
  std::unique_ptr<FileTree> m_filetree;
  std::vector<QTreeWidgetItem*> m_removeLater;
  MOBase::FilterWidget m_filter;
  bool m_needUpdate;

  void onRefresh();
  void onItemExpanded(QTreeWidgetItem* item);
  void onConflicts();
  void onArchives();
  void updateOptions();
  void ensureFullyLoaded();
  bool isActive() const;
  void doUpdateTree();
};

#endif // MODORGANIZER_DATATAB_INCLUDED
