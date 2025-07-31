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
	static qint64 TryCreateNewGlobalGroup(QString name, QString idGroup); // ids: 1...n ; returns id, or -1 if error
	static qint64 TryCreateNewLocalGroup(QString name); // ids: 0...-n ; returns id, or 1 if error
	static void SetGroupSubscribed(QString groupId, bool value);
	static bool MoveNoteToGroup(QString noteId, QString newGroupId, QString dtUpdated);

	static QString InsertNoteInDB(Note *note, bool createIdForNote);
	static QString GetNewIdForLocalNote();
	//static qint64 InsertNoteInServerDB(Note *note);
	//static qint64 BadInsertNoteResult() { return -1; }

	static QStringList NoteById(const QString &id);
	static Note NoteById_make_note(const QString &id);
	static std::pair<bool, QStringList> NoteByIdWithCheck(const QString &id);
	static int CountNoteId(const QString &id);
	//static QStringPairVector NotesFromGroup_id_dtUpdated(const QString &idGroup);
	static std::vector<Note> NotesFromBD(bool subscibedOnly);
	static std::set<QString> NotesIds(bool gloabalNotesOnly);

	//static bool SetNoteFieldIdOnServer_OnClient(const QString &idNote, const QString &idOnServer);

	static QString UpdateRecordFromNote(Note *note);
	static QString UpdateNote_IdNote_IdGroup(QString prevId, QString newId, QString newGroupId, const QDateTime &dtUpdated);
	//static bool SaveNoteOnServer(Note *note);
	static bool RemoveNote(const QString &id, bool chekId);

	//static QString HighestIdOnServer();
	//static std::vector<QStringList> NotesWithHigherIdOnServer(const QString &idOnServer);

	static void SetOpensCount(const QString &noteId, int count);
	static void AddOpensCount(const QString &noteId, int addCount);
	static QStringList NotesIdsOrderedByOpensCount();
	
	static void SetNoteNotSendedToServer(const QString &noteId, bool value);
	static QStringList NotesNotSendedToServer();
	
	///\brief return [idOnServer, idGroup]
	//static QStringPairVector NotesWithHigherIdOnServer(const QString &idOnServer);
};


#endif // DATABASE_H
