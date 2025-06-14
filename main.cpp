#include "WidgetServer.h"

#include <QApplication>

#include "LaunchParams.h"

#include "DevNames.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	LaunchParams::Init({
						   LaunchParams::DeveloperData(DevNames::RomaWork(), "TKO3-206", "C:/Work/C++/Notes",
						   "D:/Documents/C++ QT/"),
						   LaunchParams::DeveloperData(DevNames::RomaHome(), "ELBRUS-HOME-10", "D:/Documents/C++ QT/Notes",
						   "C:/Work/C++/"),
						   LaunchParams::DeveloperData(DevNames::RomaNotebook(), "ELBRUS-notebook", "D:/Work/Notes",
						   "C:/Work/C++/"),
					   });

	WidgetServer w;
	w.show();
	return a.exec();
}
