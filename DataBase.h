#ifndef DATABASE_H
#define DATABASE_H

#include "MyQSqlDatabase.h"

#include "Fields.h"

#include "Note.h"

const BaseData clientBaseRegular {"Client base",
									"D:\\Documents\\C++ QT\\Notes\\Client base.mdb",
									"D:\\Documents\\C++ QT\\Notes\\storage"};
const BaseData clientBaseDebug {"Client base debug",
									"D:\\Documents\\C++ QT\\Notes\\Client base debug.mdb",
									"D:\\Documents\\C++ QT\\Notes\\storage_debug"};

const BaseData serverBaseRegular {"Client base",
									"D:\\Documents\\C++ QT\\NotesServer\\Server base.mdb",
									"D:\\Documents\\C++ QT\\NotesServer\\storage"};
const BaseData serverBaseDebug {"Client base debug",
									"D:\\Documents\\C++ QT\\NotesServer\\Server base debug.mdb",
									"D:\\Documents\\C++ QT\\NotesServer\\storage_debug"};

#ifdef QT_DEBUG
const BaseData clientBase {clientBaseDebug};
const BaseData serverBase {serverBaseDebug};
#else
const BaseData clientBase {clientBaseRegular};
const BaseData serverBase {serverBaseRegular};
#endif

class DataBase: public MyQSqlDatabase
{
public:
	enum workModes { undefined, server, client };
	inline static workModes workMode = undefined;

	static void InitChildDataBase(workModes workMode_) { workMode = workMode_; }

	static void BackupBase();

	static QString GroupId(const QString &groupName);
	static QString GroupName(const QString &groupId);
	static QString CountGroupsWithId(QString idGroup);
	static QString CountGroupsWithName(QString nameGroup);
	static const QString& DefaultGroupId();
	static QStringList GroupsNames();
	static QStringPairVector GroupsData() { return {}; }
	static int TryCreateNewGroup(QString name, QString idGroup);
	static bool MoveNoteToGroup(QString noteId, QString newGroupId);

	static int InsertNoteInDefaultGroup(Note *note);

	static QStringList NoteById(const QString &id);
	static bool CheckNoteId(const QString &id);
	static std::vector<Note> NotesFromBD();

	static void SaveNote(Note *note);
	static bool RemoveNote(Note *note);


};


#endif // DATABASE_H
