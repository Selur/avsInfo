#include <QString>
#include <QFile>
#include <iostream>
using namespace std;

#include <QCoreApplication>
#include "Analyser.h"
#include <QTimer>

int main(int argc, char *argv[])
{
  bool walk = false;
  QString currentInput;
  if (argc < 2) {
    currentInput = "c:/Users/Selur/Desktop/version.avs";
  } else if (argc == 3) {
    QString tmp = argv[2];
    if (tmp != "--walk") {
      cerr << "expected '--walk' found: " << qPrintable(tmp) << endl;
      cerr << "usage: avsInfo \"Path to the .avs file\"" << endl;
      return -1;
    }
    walk = true;
  } else if (argc > 3) {
    cerr << "usage: avsInfo \"Path to the .avs file\"" << endl;
    return -1;
  } else {
    currentInput = argv[1];
  }

  if (!QFile::exists(currentInput)) {
    cerr << "couldn't find: " << qPrintable(currentInput) << endl;
    cerr << "usage: avsInfo \"Path to the .avs file\"" << endl;
    return -1;
  }
  QCoreApplication a(argc, argv);
  Analyser ana(&a, currentInput, walk);
  a.connect(&ana, SIGNAL(closeApplication()), &a, SLOT(quit()), Qt::QueuedConnection);
  ana.analyse();
  return a.exec();
}
