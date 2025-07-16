#ifndef DATABASE_H
#define DATABASE_H

#include "MyQSqlDatabase.h"

#include "Fields.h"

#include "Note.h"

class DataBase: public MyQSqlDatabase
{
public:
	enum workModes { undefined, server, client };
	inline static workModes workMode = undefined;

	static BaseData defineBase(workModes mode);

	static void InitChildDataBase(workModes workMode_) { workMode = workMode_; }

	static void BackupBase();

	static QString GroupId(const QString &groupName);
	static QString GroupName(const QString &groupId);
	static QString CountGroupsWithId(QString idGroup);
	static QString CountGroupsWithName(QString nameGroup);
	static const QString& DefaultGroupId2();
	static QStringList GroupsIds();
	static QStringList GroupsNames();
	static QStringPairVector GroupsIdsAndNames();
	static QStringListVector GroupsAllFields();
	static bool IsGroupLocalById(const QString &id);
	static bool IsGroupLocalByName(const QString &name);
	static qint64 TryCreateNewGlobalGroup(QString name, QString idGroup); // ids: 1...n ; returns -1 if error
	static qint64 TryCreateNewLocalGroup(QString name); // ids: 0...-n ; returns 1 if error
	static void SetGroupSubscribed(QString groupId, bool value);
	static bool MoveNoteToGroupOnClient(QString noteId, QString newGroupId, QString dtUpdated);
	static bool MoveNoteToGroupOnServer(QString noteIdOnServer, QString newGroupId, QString dtUpdated);

	static qint64 InsertNoteInClientDB(Note *note);
	static qint64 InsertNoteInServerDB(Note *note);
	static qint64 BadInsertNoteResult() { return -1; }

	static QStringList NoteByIdOnClient(const QString &id);
	static QStringList NoteByIdOnServer(const QString &idOnServer);
	static std::pair<bool, QStringList> NoteByIdOnServerWithCheck(const QString &idOnServer);
	static bool CheckNoteIdOnClient(const QString &id);
	static bool CheckNoteIdOnServer(const QString &idOnServer);
	static QStringPairVector NotesFromGroup_id_dtUpdated(const QString &idGroup);
	static std::vector<Note> NotesFromBD(bool subscibedOnly);
	static std::set<QString> NotesIdsOnServer();

	static bool SetNoteFieldIdOnServer_OnClient(const QString &idNote, const QString &idOnServer);

	static QString SaveNoteOnClient(Note *note);
	static bool SaveNoteOnServer(Note *note);
	static bool RemoveNoteOnClient(const QString &id, bool chekId);
	static bool RemoveNoteOnServer(const QString &idOnServer, bool chekId);

	static QString HighestIdOnServer();
	static std::vector<QStringList> NotesWithHigherIdOnServer(const QString &idOnServer);
	///\brief return [idOnServer, idGroup]
	//static QStringPairVector NotesWithHigherIdOnServer(const QString &idOnServer);
};


#endif // DATABASE_H
