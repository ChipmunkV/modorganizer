#ifndef MODORGANIZER_DOWNLOADTAB_INCLUDED
#define MODORGANIZER_DOWNLOADTAB_INCLUDED

#include <filterwidget.h>

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace Ui { class MainWindow; }
class OrganizerCore;
class DownloadListView;

class DownloadsTab : public QObject
{
  Q_OBJECT;

public:
  DownloadsTab(OrganizerCore& core, Ui::MainWindow* ui);

  void update();

private:
  struct DownloadsTabUi
  {
    QPushButton* refresh;
    DownloadListView* list;
    QCheckBox* showHidden;
    QLineEdit* filter;
  };

  OrganizerCore& m_core;
  DownloadsTabUi ui;
  MOBase::FilterWidget m_filter;

  void refresh();
  void resumeDownload(int downloadIndex);
};

#endif // MODORGANIZER_DOWNLOADTAB_INCLUDED
