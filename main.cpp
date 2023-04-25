
#include <QObject>
#include <QCoreApplication>
#include <cpp/qnasadownloader.h>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QNasaDownloader *qnd = new QNasaDownloader("pz1c", "Pz1cpz!c");
    QObject::connect(qnd, SIGNAL(finished(int)), qnd, SLOT(deleteLater()));
    QObject::connect(qnd, SIGNAL(finished(int)), &a, SLOT(exit(int)));
    qnd->getFile("https://cddis.nasa.gov/archive/gnss/data/daily/2023/brdc/brdc1150.23n.gz",
                "brdc1150.23n.gz");

    return a.exec();
}
