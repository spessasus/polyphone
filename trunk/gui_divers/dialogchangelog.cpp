#include "dialogchangelog.h"
#include "ui_dialogchangelog.h"
#include <QFile>
#include <QTextStream>
#include <QDesktopServices>

DialogChangeLog::DialogChangeLog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogChangeLog)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose);

    // Version
    ui->labelTitle->setText(trUtf8("Merci d'avoir installé") + "\n" +
                            trUtf8("Polyphone") + " " + trUtf8("version") + " " + QString::number(VERSION) +
                            (FINAL ? "" : " unreleased"));

    // Read changelog and extract information
    QStringList listNew, listFix, listImprovement;
    QFile inputFile(":/misc/changelog");
    QString currentVersion = "";
    QRegExp rx1("^polyphone \\(([0-9]+\\.[0-9]+)\\).*");
    QRegExp rx2("^  \\* \\((new|fix|improvement)\\) (.*)");
    if (inputFile.open(QIODevice::ReadOnly))
    {
       QTextStream in(&inputFile);
       while (!in.atEnd())
       {
          QString line = in.readLine();
          if (rx1.exactMatch(line))
              currentVersion = rx1.cap(1);
          else if (currentVersion == QString::number(VERSION))
          {
              if (rx2.exactMatch(line))
              {
                  if (rx2.cap(1) == "new")
                      listNew << rx2.cap(2);
                  else if (rx2.cap(1) == "improvement")
                      listImprovement << rx2.cap(2);
                  else if (rx2.cap(1) == "fix")
                      listFix << rx2.cap(2);
              }
          }
          else if (currentVersion != "")
              break;
       }
       inputFile.close();
    }

    // Prepare and fill text
    QString text;
    if (!listNew.empty())
        text += "<h2><font color='#55BB99'>" + trUtf8("Ce qu'il y a de nouveau") + "</font></h2>" +
                " &#9679; " + listNew.join("<br/> &#9679; ");
    if (!listImprovement.empty())
        text += "<h2><font color='#55BB99'>" + trUtf8("Ce qui est amélioré") + "</font></h2>" +
                " &#9679; " + listImprovement.join("<br/> &#9679; ");
    if (!listFix.empty())
        text += "<h2><font color='#55BB99'>" + trUtf8("Ce qui est résolu") + "</font></h2>" +
                " &#9679; " + listFix.join("<br/> &#9679; ");
    ui->textChangelog->setText(text);
}

DialogChangeLog::~DialogChangeLog()
{
    delete ui;
}

void DialogChangeLog::on_pushDonate_clicked()
{
    QDesktopServices::openUrl(QUrl("https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=ESBLSGPJ7P938&lc=US&item_name=Polyphone&currency_code=USD&bn=PP%2dDonationsBF%3abtn_donate_LG%2egif%3aNonHosted"));
}

void DialogChangeLog::on_pushOk_clicked()
{
    this->close();
}
