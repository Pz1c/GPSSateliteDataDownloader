
#include <QObject>
#include <QCoreApplication>
#include <cpp/qnasadownloader.h>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QStringList arg = a.arguments();
    if (arg.count() < 5) {
        qDebug() << "Wrong arguments count";
        return 1;
    }
    QString DownloadLink = arg.at(1);
    QString UserLogin = arg.at(2);
    QString UserPassword = arg.at(3);
    QString OutputFile = arg.at(4);

    QNasaDownloader *qnd = new QNasaDownloader(UserLogin, UserPassword);
    QObject::connect(qnd, SIGNAL(finished(int)), qnd, SLOT(deleteLater()));
    QObject::connect(qnd, SIGNAL(finished(int)), &a, SLOT(exit(int)));
    qnd->getFile(DownloadLink, OutputFile);

    return a.exec();
}
