/**********************************************************************
 *  mainwindow.cpp
 **********************************************************************
 * Copyright (C) 2015 MX Authors
 *
 * Authors: Adrian
 *          Paul David Callahan
 *          Dolphin Oracle
 *          MX Linux <http://mxlinux.org>
 *
 * This file is part of mx-welcome.
 *
 * mx-welcome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * mx-welcome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with mx-welcome.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QTextEdit>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "flatbutton.h"
#include "version.h"


MainWindow::MainWindow(const QCommandLineParser &arg_parser, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MainWindow)
{
    qDebug().noquote() << QCoreApplication::applicationName() << "version:" << VERSION;
    ui->setupUi(this);
    setWindowFlags(Qt::Window); // for the close, min and max buttons
    setup();

    ui->tabWidget->setCurrentIndex(0);
    if (arg_parser.isSet("about"))
        ui->tabWidget->setCurrentIndex(1);

    if (arg_parser.isSet("test")) {
        ui->labelLoginInfo->show();
        ui->buttonSetup->show();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

// setup versious items first time program runs
void MainWindow::setup()
{
    version = getVersion("mx-welcome");
    this->setWindowTitle(tr("MX Welcome"));

    QString old_conf_file = QDir::homePath() + "/.config/" + qApp->applicationName() + ".conf";
    if (QFileInfo::exists(old_conf_file)) {
        QSettings old_settings(qApp->applicationName());
        user_settings.setValue("AutoStartup", old_settings.value("AutoStartup", false).toBool());
        QFile::remove(old_conf_file);
    }
    bool autostart = user_settings.value("AutoStartup", false).toBool();
    ui->checkBox->setChecked(autostart);
    if (!autostart)
        QFile::remove(QDir::homePath() + "/.config/autostart/mx-welcome.desktop");

    ui->labelLoginInfo->setText("<p align=\"center\">" + tr("User demo, password:") + "<b> demo</b>. " +
                                tr("Superuser root, password:") + "<b> root</b>." + "</p>");
    // if running live
    QString test = runCmd("df -T / |tail -n1 |awk '{print $2}'").output;
    if (test == "aufs" || test == "overlay") {
        ui->checkBox->hide();
    } else {
        ui->labelLoginInfo->hide();
        ui->buttonSetup->hide();
    }

    // setup title block & icons
    QSettings settings("/usr/share/mx-welcome/mx-welcome.conf", QSettings::NativeFormat);
    //QString DISTRO = settings.value("DISTRO").toString();
    //QString CODENAME = settings.value("CODENAME").toString();
    QString CONTRIBUTE = settings.value("CONTRIBUTE").toString();
    QString CODECS = settings.value("CODECS").toString();
    QString FAQ = settings.value("FAQ").toString();
    QString FORUMS = settings.value("FORUMS").toString();
    QString LOGO = settings.value("LOGO").toString();
    QString PACKAGEINSTALLER = settings.value("PACKAGEINSTALLER").toString();
    QString PANELORIENT = settings.value("PANELORIENT").toString();
    QString SETUP = settings.value("SETUP").toString();
    QString TOOLS = settings.value("TOOLS").toString();
    QString MANUAL = settings.value("MANUAL").toString();
    QString VIDEOS = settings.value("VIDEOS").toString();
    QString WIKI = settings.value("WIKI").toString();
    QString HEADER = settings.value("HEADER").toString();
    QString SUPPORTED = settings.value("SUPPORTED").toString();

    QSettings lsb("/etc/lsb-release", QSettings::NativeFormat);
    QString MAINDISTRO = lsb.value("DISTRIB_ID").toString();
    QString CODENAME = lsb.value("DISTRIB_CODENAME").toString();
    QString DISTRIB_RELEASE = lsb.value("DISTRIB_RELEASE").toString();
    QString DISTRO = MAINDISTRO + "-" + DISTRIB_RELEASE;

    QString debian_version;
    QString mxfluxbox_version;

    QFile file("/etc/debian_version");
    if (!file.open(QIODevice::ReadOnly))
        QMessageBox::information(nullptr, tr("Error"), file.errorString());

    QTextStream in(&file);
    debian_version = in.readLine();
    file.close();

    ui->LabelDebianVersion->setText(debian_version);

    ui->labelSupportUntil->setText(SUPPORTED);

    QString DESKTOP = runCmd("LANG=C inxi -c 0 -S ").output.trimmed().section(":",5,5).section("\n",0,0);
    if (DESKTOP.contains("Fluxbox")){
        isfluxbox = true;
        QFile file("/etc/mxfb_version");
        if (file.exists()) {
            if (!file.open(QIODevice::ReadOnly))
                QMessageBox::information(nullptr, tr("Error"), file.errorString());
            QTextStream in(&file);
            mxfluxbox_version = in.readLine();
            qDebug() << "mxfluxbox" << mxfluxbox_version;
            file.close();
            if (!mxfluxbox_version.isEmpty())
                DESKTOP.append(" " + mxfluxbox_version);
        }
    }


    ui->labelDesktopVersion->setText(DESKTOP);

    ui->labelTitle->setText(tr("<html><head/><body><p align=\"center\"><span style=\" font-size:14pt; font-weight:600;\">%1 &quot;%2&quot;</span></p></body></html>").arg(DISTRO).arg(CODENAME));
    if (QFile::exists(HEADER))
        ui->labelgraphic->setPixmap(HEADER);

    // setup icons
    ui->buttonCodecs->setIcon(QIcon(CODECS));
    ui->buttonContribute->setIcon(QIcon(CONTRIBUTE));
    ui->buttonFAQ->setIcon(QIcon(FAQ));
    ui->buttonForum->setIcon(QIcon(FORUMS));
    ui->labelMX->setPixmap(QPixmap(LOGO));
    ui->buttonPackageInstall->setIcon(QIcon(PACKAGEINSTALLER));
    ui->buttonPanelOrient->setIcon(QIcon(PANELORIENT));
    ui->buttonSetup->setIcon(QIcon(SETUP));
    ui->buttonTools->setIcon(QIcon(TOOLS));
    ui->buttonManual->setIcon(QIcon(MANUAL));
    ui->buttonVideo->setIcon(QIcon(VIDEOS));
    ui->buttonWiki->setIcon(QIcon(WIKI));

    // setup about labels
    ui->labelMXversion->setText(DISTRO);

    settabstyle();
    this->adjustSize();
}

// Util function for getting bash command output and error code
Result MainWindow::runCmd(QString cmd)
{
    QEventLoop loop;
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    connect(&proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), &loop, &QEventLoop::quit);
    proc.start("/bin/bash", QStringList() << "-c" << cmd);
    loop.exec();
    return {proc.exitCode(), proc.readAll().trimmed()};
}


// Get version of the program
QString MainWindow::getVersion(QString name)
{
    return runCmd("dpkg-query -f '${Version}' -W " + name).output;
}

// About button clicked
void MainWindow::on_buttonAbout_clicked()
{
    this->hide();
    QMessageBox msgBox(QMessageBox::NoIcon,
                       tr("About MX Welcome"), "<p align=\"center\"><b><h2>" +
                       tr("MX Welcome") + "</h2></b></p><p align=\"center\">" + tr("Version: ") + version + "</p><p align=\"center\"><h3>" +
                       tr("Program for displaying a welcome screen in MX Linux") +
                       "</h3></p><p align=\"center\"><a href=\"http://www.mxlinux.org/mx\">http://www.mxlinux.org/mx</a><br /></p><p align=\"center\">" +
                       tr("Copyright (c) MX Linux") + "<br /><br /></p>");
    QPushButton *btnLicense = msgBox.addButton(tr("License"), QMessageBox::HelpRole);
    QPushButton *btnChangelog = msgBox.addButton(tr("Changelog"), QMessageBox::HelpRole);
    QPushButton *btnCancel = msgBox.addButton(tr("Cancel"), QMessageBox::NoRole);
    btnCancel->setIcon(QIcon::fromTheme("window-close"));

    msgBox.exec();

    if (msgBox.clickedButton() == btnLicense) {
        QString cmd = QString("mx-viewer file:///usr/share/doc/mx-welcome/license.html '%1'").arg(tr("MX Welcome"));
        system(cmd.toUtf8());
    } else if (msgBox.clickedButton() == btnChangelog) {
        QDialog *changelog = new QDialog(this);
        changelog->resize(600, 500);

        QTextEdit *text = new QTextEdit;
        text->setReadOnly(true);
        text->setText(runCmd("zless /usr/share/doc/" + QFileInfo(QCoreApplication::applicationFilePath()).fileName()  + "/changelog.gz").output);

        QPushButton *btnClose = new QPushButton(tr("&Close"));
        btnClose->setIcon(QIcon::fromTheme("window-close"));
        connect(btnClose, &QPushButton::clicked, changelog, &QDialog::close);

        QVBoxLayout *layout = new QVBoxLayout;
        layout->addWidget(text);
        layout->addWidget(btnClose);
        changelog->setLayout(layout);
        changelog->exec();
    }
    this->show();
}

// Add/remove autostart at login
void MainWindow::on_checkBox_clicked(bool checked)
{
    user_settings.setValue("AutoStartup", checked);
    if (checked)
        QFile::copy("/usr/share/mx-welcome/mx-welcome.desktop", QDir::homePath() + "/.config/autostart/mx-welcome.desktop");
    else
        QFile::remove(QDir::homePath() + "/.config/autostart/mx-welcome.desktop");
}

// Start MX-Tools
void MainWindow::on_buttonTools_clicked()
{
    system("mx-tools&");
}

// Launch Manual in browser
void MainWindow::on_buttonManual_clicked()
{
    if (isfluxbox)
        system("mxfb-help&");
    else
        system("mx-manual&");
}

// Launch Forum in browser
void MainWindow::on_buttonForum_clicked()
{
    system("xdg-open http://forum.mxlinux.org/index.php");
}

// Launch Wiki in browser
void MainWindow::on_buttonWiki_clicked()
{
    system("xdg-open http://www.mxlinux.org/wiki");
}

// Launch Video links in browser
void MainWindow::on_buttonVideo_clicked()
{
    system("xdg-open http://www.mxlinux.org/videos/");
}

// Launch Contribution page
void MainWindow::on_buttonContribute_clicked()
{
    system("xdg-open http://www.mxlinux.org/donate");
}

void MainWindow::on_buttonPanelOrient_clicked()
{
    system("mx-tweak&");
}

void MainWindow::on_buttonPackageInstall_clicked()
{
    system("su-to-root -X -c mx-packageinstaller&");
}

void MainWindow::on_buttonCodecs_clicked()
{
    system("su-to-root -X -c mx-codecs&");
}

void MainWindow::on_buttonFAQ_clicked()
{
    system("mx-faq&");
}

void MainWindow::on_buttonSetup_clicked()
{
    system("minstall-pkexec&");
}

void MainWindow::on_buttonTOS_clicked()
{
    system("xdg-open https://mxlinux.org/terms-of-use/");
}

void MainWindow::on_ButtonQSI_clicked()
{
    system("x-terminal-emulator -e bash -c \"/usr/bin/quick-system-info-mx\" &");
}

void MainWindow::shortsysteminfo()
{
    ui->textBrowser->setText(runCmd("LANG=C inxi -c 0").output);
}
void MainWindow::on_tabWidget_currentChanged(int index)
{
    if (index == 1)
        shortsysteminfo();
    settabstyle();
}

void MainWindow::resizeEvent(QResizeEvent*)
{
   settabstyle();
}

void MainWindow::settabstyle()
{
    QString tw = QString::number(ui->tabWidget->width() / 2 - 1);
    //qDebug() << "width" << ui->tabWidget->width() << "tw" << tw;
    ui->tabWidget->setStyleSheet("""QTabBar::tab:!selected{width: " + tw + "px; background:  rgba(140, 135, 135, 50)}""""QTabBar::tab:selected{width: " + tw + "px}""");
}

