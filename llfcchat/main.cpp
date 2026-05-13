#include "mainwindow.h"
#include <QApplication>
#include <QFile>
#include "global.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFile qss(":/style/stylesheet.qss");

    if( qss.open(QFile::ReadOnly))
    {
        qDebug("open success");
        QString style = QLatin1String(qss.readAll());
        a.setStyleSheet(style);
        qss.close();
    }else{
         qDebug("Open failed");
     }


    // 从exe所在目录向上搜索config.ini，直到找到为止
    QDir dir(QCoreApplication::applicationDirPath());
    QString config_path = dir.absoluteFilePath("config.ini");
    while (!QFile::exists(config_path) && dir.cdUp()) {
        config_path = dir.absoluteFilePath("config.ini");
    }

    QSettings settings(config_path, QSettings::IniFormat);
    QString gate_host = settings.value("GateServer/host").toString();
    QString gate_port = settings.value("GateServer/port").toString();
    qDebug()<<gate_host;
    qDebug()<<gate_port;
    gate_url_prefix = "http://"+gate_host+":"+gate_port;
    qDebug()<<gate_url_prefix;
    MainWindow w;
    w.show();
    return a.exec();
}
