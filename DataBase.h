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
	static void BackupBase()
	{
		QString dtStr = QDateTime::currentDateTime().toString(DateTimeFormatForFileName);
		if(!QFile::copy(baseDataCurrent->baseFilePathName, Note::notesBackupsPath + "/" + dtStr + " " + baseDataCurrent->baseFileNoPath))
			QMbError("creation backup error for base  " + baseDataCurrent->baseFilePathName);
	}

	static QString GroupId(const QString &groupName)
	{
		return DoSqlQueryGetFirstCell("select " + Fields::idGroup() + " from " + Fields::Groups() + "\n"
									  "where " + Fields::nameGroup() + " = :nameGroup",
									  {{":nameGroup",groupName}});
	}
	static const QString& DefaultGroupId()
	{
		static QString str = GroupId(Note::defaultGroupMarker());
		return str;
	}
	static int InsertNoteInDefaultGroup(Note *note)
	{
		auto res = DoSqlQueryExt("insert into " + Fields::Notes() + " ("+Fields::idGroup() + ", " + Fields::nameNote() + ", "
				   + Fields::activeNotify() + ", " + Fields::dtNotify() + ", " + Fields::dtPostpone() + ", " + Fields::content() + ")\n"
				   + "values (:groupId, :name, :actNotif, :dtNotif, :dtPosp, :content)",
				   {{":groupId", DefaultGroupId()}, {":name", note->Name()}, {":actNotif", QSn(note->activeNotify)},
					{":dtNotif", note->DTNotify().toString(Fields::dtFormat())},
					{":dtPosp", note->DTPostpone().toString(Fields::dtFormat())},
					{":content", note->Content()}});

		note->id = -1;

		if(res.errors.isEmpty())
		{
			auto id = DoSqlQueryGetFirstCell("select max("+Fields::idNote()+") from " + Fields::Notes());
			bool ok;
			note->id = id.toUInt(&ok);
			if(!ok)
			{
				note->id = -1;
				QMbError("bad id " + id);
			}

		}
		else QMbError("note was not inserted");

		return note->id;
	}
	static QStringList NoteById(const QString &id)
	{
		return DoSqlQueryGetFirstRec("select * from "+Fields::Notes()+" where "+Fields::idNote()+" = " + id);
	}
	static bool CheckNoteId(const QString &id)
	{
		return DoSqlQueryGetFirstCell("select count("+Fields::idNote()+") from "+Fields::Notes()
									  +" where "+Fields::idNote()+" = " + id).toInt();
	}
	static std::vector<Note> NotesFromDefaultGroup()
	{
		std::vector<Note> notes;
		auto table = DoSqlQueryGetTable("select * from "+Fields::Notes()+" where "+Fields::idGroup()+" = " + DefaultGroupId());
		for(auto &row:table)
		{
			notes.emplace_back();
			notes.back().InitFromRecord(row);
		}
		return notes;
	}
	static void SaveNote(Note *note)
	{
		if(note->group != note->defaultGroup.get()) { QMbError("not default groups unrealesed. Not Saved"); return; }

		// если это новая
		if(note->id == Note::idMarkerCreateNewNote)
		{
			InsertNoteInDefaultGroup(note);
			if(!CheckNoteId(QSn(note->id))) QMbError("SaveNote: note "+note->Name()+" save error");
		}
		else // если не новая
		{
			if(!CheckNoteId(QSn(note->id)))
			{
				QMbError("SaveNote: note with id "+QSn(note->id)+" doesnt exists");
				return;
			}

			auto res = DoSqlQueryExt("update "+Fields::Notes()+" set "+Fields::nameNote() + " = :name, " + Fields::activeNotify()
									 + " = :actNotif, " +Fields::dtNotify()+" = :dtNotif, "+Fields::dtPostpone()
									 + " = :dtPosp, " + Fields::content() + " = :content\n"
																			"where "+Fields::idNote()+" = :idNote",
									 {{":idNote", QSn(note->id)}, {":name", note->Name()}, {":actNotif", QSn(note->activeNotify)},
									  {":dtNotif", note->DTNotify().toString(Fields::dtFormat())},
									  {":dtPosp", note->DTPostpone().toString(Fields::dtFormat())},
									  {":content", note->Content()}});
		}
	}
	static bool RemoveNote(Note *note)
	{
		if(note->group != note->defaultGroup.get()) { QMbError("not default groups unrealesed. Not removed"); return false; }

		if(!CheckNoteId(QSn(note->id)))
		{
			QMbError("RemoveNote: note with id "+QSn(note->id)+" doesnt exists");
			return false;
		}

		auto res = DoSqlQueryExt("delete from "+Fields::Notes()+" where "+Fields::idNote()+" = :idNote",
								 {{":idNote", QSn(note->id)}});

		if(CheckNoteId(QSn(note->id)))
		{
			QMbError("RemoveNote: note with id "+QSn(note->id)+" after delete sql continue exist");
			return false;
		}

		return true;
	}
};


#endif // DATABASE_H
