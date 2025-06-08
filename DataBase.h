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

const BaseData serverBaseRegular {"Server base",
									"D:\\Documents\\C++ QT\\NotesServer\\Server base.mdb",
									"D:\\Documents\\C++ QT\\NotesServer\\storage"};
const BaseData serverBaseDebug {"Server base debug",
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
	static qint64 TryCreateNewGroup(QString name, QString idGroup);
	static bool MoveNoteToGroupOnClient(QString noteId, QString newGroupId, QString dtUpdated);
	static bool MoveNoteToGroupOnServer(QString noteIdOnServer, QString newGroupId, QString dtUpdated);

	static qint64 InsertNoteInClientDB(Note *note);
	static qint64 InsertNoteInServerDB(Note *note);

	static QStringList NoteByIdOnClient(const QString &id);
	static QStringList NoteByIdOnServer(const QString &idOnServer);
	static bool CheckNoteIdOnClient(const QString &id);
	static bool CheckNoteIdOnServer(const QString &idOnServer);
	static std::vector<Note> NotesFromBD();

	static bool SetNoteFieldIdOnServer_OnClient(const QString &idNote, const QString &idOnServer);

	static QString SaveNoteOnClient(Note *note);
	static bool SaveNoteOnServer(Note *note);
	static bool RemoveNoteOnClient(const QString &id, bool chekId);
	static bool RemoveNoteOnServer(const QString &idOnServer, bool chekId);

	static QString HighestIdOnServer();
	static std::vector<QStringList> NotesWithHigherIdOnServer(QString idOnServer);
};


#endif // DATABASE_H
