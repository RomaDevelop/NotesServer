#include "DataBase.h"

void DataBase::BackupBase()
{
	QString dtStr = QDateTime::currentDateTime().toString(DateTimeFormatForFileName);
	if(!QFile::copy(baseDataCurrent->baseFilePathName, Note::notesBackupsPath + "/" + dtStr + " " + baseDataCurrent->baseFileNoPath))
		QMbError("creation backup error for base  " + baseDataCurrent->baseFilePathName);
}

QString DataBase::GroupId(const QString &groupName)
{
	return DoSqlQueryGetFirstCell("select " + Fields::idGroup() + " from " + Fields::Groups() + "\n"
										"where " + Fields::nameGroup() + " = :nameGroup",
								  {{":nameGroup",groupName}});
}

QString DataBase::GroupName(const QString &groupId)
{
	return DoSqlQueryGetFirstCell("select " + Fields::nameGroup() + " from " + Fields::Groups() + "\n"
										"where " + Fields::idGroup() + " = :idGroup",
								  {{":idGroup",groupId}});
}

QString DataBase::CountGroupsWithId(QString idGroup)
{
	return DoSqlQueryGetFirstCell("select count("+Fields::idGroup()+") from "+Fields::Groups()
						   +" where "+Fields::idGroup()+" = :id",
						   {{":id", std::move(idGroup)}});
}

QString DataBase::CountGroupsWithName(QString nameGroup)
{
	return DoSqlQueryGetFirstCell("select count("+Fields::idGroup()+") from "+Fields::Groups()
						   +" where "+Fields::nameGroup()+" = :name",
						   {{":name", std::move(nameGroup)}});
}

const QString &DataBase::DefaultGroupId()
{
	static QString str = GroupId(Note::defaultGroupName());
	return str;
}

QStringList DataBase::GroupsNames()
{
	return DoSqlQueryGetFirstField("select "+Fields::nameGroup()+" from "+Fields::Groups());
}

int DataBase::TryCreateNewGroup(QString name, QString idGroup)
{
	if(name.isEmpty() || name.size() > Fields::maxStringFieldLenth) return -1;

	auto count = CountGroupsWithName(name);
	if(count != '0')
	{
		if(workMode == server) Log("TryCreateNewGroup: group with name " + name + " exists");
		if(workMode == client) Error("TryCreateNewGroup: group with name " + name + " exists");
		return -1;
	}

	if(workMode == server)
	{
		if(!idGroup.isEmpty()) { Error("idGroup not empty in server mode"); return -1; }
		uint64_t maxId = DoSqlQueryGetFirstCell("select max("+Fields::idGroup()+") from "+Fields::Groups()).toULongLong();
		maxId++;

		auto request = MakeInsertRequest(Fields::Groups(), {Fields::idGroup(), Fields::nameGroup()}, {QSn(maxId), std::move(name)});
		DoSqlQuery(request.first, request.second);
		count = CountGroupsWithId(QSn(maxId));
		if(count == "1") return maxId;
		return count.toInt() * -1;
	}
	else if(workMode == client)
	{
		if(idGroup.isEmpty()) { Error("idGroup is empty in client mode"); return -1; }
		auto count = CountGroupsWithId(idGroup);
		if(count != '0') { Error("TryCreateNewGroup: group with id " + idGroup + " exists"); return -1; }

		auto request = MakeInsertRequest(Fields::Groups(), {Fields::idGroup(), Fields::nameGroup()}, {idGroup, std::move(name)});
		DoSqlQuery(request.first, request.second);
		count = CountGroupsWithId(idGroup);
		if(count == "1") return idGroup.toULongLong();
		return count.toInt() * -1;
	}

	Error("TryCreateNewGroup with wrong workMode: " + MyQString::AsDebug(workMode));
	return -1;
}

bool DataBase::MoveNoteToGroup(QString noteId, QString newGroupId)
{
	auto request = MakeUpdateRequest(Fields::Notes(), {Fields::idGroup()}, {newGroupId}, {Fields::idNote()}, {noteId});
	qdbg << request.first;
	qdbg << request.second;
	auto res = DoSqlQueryExt(request.first, request.second);
	if(!res.errors.isEmpty()) { Error("SaveNote update sql error: " + res.errors); return false; }
	else { Log("note "+noteId+" moved to group " + newGroupId); return true; }
}

int DataBase::InsertNoteInDefaultGroup(Note *note)
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

QStringList DataBase::NoteById(const QString &id)
{
	return DoSqlQueryGetFirstRec("select * from "+Fields::Notes()+" where "+Fields::idNote()+" = " + id);
}

bool DataBase::CheckNoteId(const QString &id)
{
	return DoSqlQueryGetFirstCell("select count("+Fields::idNote()+") from "+Fields::Notes()
								  +" where "+Fields::idNote()+" = " + id).toInt();
}

std::vector<Note> DataBase::NotesFromBD()
{
	std::vector<Note> notes;
	auto table = DoSqlQueryGetTable("select * from "+Fields::Notes());
	for(auto &row:table)
	{
		notes.emplace_back();
		notes.back().InitFromRecord(row);
	}
	return notes;
}

void DataBase::SaveNote(Note *note)
{
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

		if(!res.errors.isEmpty()) Error("SaveNote update sql error: " + res.errors);
	}
}

bool DataBase::RemoveNote(Note *note)
{
	if(note->group != note->defaultGroupName()) { QMbError("not default groups unrealesed. Not removed"); return false; }

	if(!CheckNoteId(QSn(note->id)))
	{
		QMbError("RemoveNote: note with id "+QSn(note->id)+" doesnt exists");
		return false;
	}

	DoSqlQuery("delete from "+Fields::Notes()+" where "+Fields::idNote()+" = :idNote",
							 {{":idNote", QSn(note->id)}});

	if(CheckNoteId(QSn(note->id)))
	{
		QMbError("RemoveNote: note with id "+QSn(note->id)+" after delete sql continue exist");
		return false;
	}

	return true;
}




