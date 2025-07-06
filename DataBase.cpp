#include "DataBase.h"

#include "CodeMarkers.h"

#include "LaunchParams.h"
#include "Resources.h"
#include "DevNames.h"

const BaseData clientBaseRegularHome {"Client base",
									"D:\\Documents\\C++ QT\\Notes\\Client base.mdb",
									"D:\\Documents\\C++ QT\\Notes\\storage"};
const BaseData clientBaseDebugHome {"Client base debug",
									"D:\\Documents\\C++ QT\\Notes\\Client base debug.mdb",
									"D:\\Documents\\C++ QT\\Notes\\storage_debug"};

const BaseData serverBaseRegularHome {"Server base",
									"D:\\Documents\\C++ QT\\NotesServer\\Server base.mdb",
									"D:\\Documents\\C++ QT\\NotesServer\\storage"};
const BaseData serverBaseDebugHome {"Server base debug",
									"D:\\Documents\\C++ QT\\NotesServer\\Server base debug.mdb",
									"D:\\Documents\\C++ QT\\NotesServer\\storage_debug"};

const BaseData clientBaseRegularWork {"Client base",
									"C:\\Work\\C++\\Notes\\Client base.mdb",
									"C:\\Work\\C++\\Notes\\storage"};
const BaseData clientBaseDebugWork {"Client base debug",
									"C:\\Work\\C++\\Notes\\Client base debug.mdb",
									"C:\\Work\\C++\\Notes\\storage_debug"};

const BaseData serverBaseRegularWork {"Server base",
									  "C:\\Work\\C++\\NotesServer\\Server base.mdb",
									  "C:\\Work\\C++\\NotesServer\\storage"};
const BaseData serverBaseDebugWork {"Server base debug",
									"C:\\Work\\C++\\NotesServer\\Server base debug.mdb",
									"C:\\Work\\C++\\NotesServer\\storage_debug"};

#ifdef QT_DEBUG
bool debug = true;
#else
bool debug = false;
#endif

BaseData DataBase::defineBase(workModes mode)
{
	if(LaunchParams::CurrentDeveloper()->devName == DevNames::RomaHome())
	{
		if(mode == client) return debug ? clientBaseDebugHome : clientBaseRegularHome;
		if(mode == server) return debug ? serverBaseDebugHome : serverBaseRegularHome;
	}
	else if(LaunchParams::CurrentDeveloper()->devName == DevNames::RomaWork())
	{
		if(mode == client) return debug ? clientBaseDebugWork : clientBaseRegularWork;
		if(mode == server) return debug ? serverBaseDebugWork : serverBaseRegularWork;
	}

	QMbError("unexpacted dev name ("+LaunchParams::CurrentDeveloper()->devName+") or mode (" + MyQString::AsDebug(mode)+")");
	return {};
}

void DataBase::BackupBase()
{
	if(0) CodeMarkers::to_do("remove this and use BackupBase from parent class; should do backup before init;"
							 "update to use for server too");
	QString dtStr = QDateTime::currentDateTime().toString(DateTimeFormatForFileName);
	QString backupPath = baseDataCurrent->storagePath + "/base_backups";
	QString backupFile = backupPath + "/" + dtStr + " " + baseDataCurrent->baseFileNoPath;
	if(!QDir().mkpath(QFileInfo(backupFile).path()))
		{ QMbError("make path error for backup file " + backupFile); return; }
	if(!QFile::copy(baseDataCurrent->baseFilePathName, backupFile))
		{ QMbError("creation backup error for backup file " + backupFile); return; }
	MyQFileDir::RemoveOldFiles(backupPath, 100);
}

QString DataBase::GroupId(const QString &groupName)
{
	auto name = DoSqlQueryGetFirstCell("select " + Fields::idGroup() + " from " + Fields::Groups() + "\n"
										"where " + Fields::nameGroup() + " = :nameGroup",
								  {{":nameGroup",groupName}});
	if(name.isEmpty()) Error("DataBase::GroupId for name " +groupName+ " is empty or not exists");
	return name;
}

QString DataBase::GroupName(const QString &groupId)
{
	auto id = DoSqlQueryGetFirstCell("select " + Fields::nameGroup() + " from " + Fields::Groups() + "\n"
										"where " + Fields::idGroup() + " = :idGroup",
								  {{":idGroup",groupId}});
	if(id.isEmpty()) Error("DataBase::GroupName for id " +groupId+ " is empty or not exists");
	return id;
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

const QString &DataBase::DefaultGroupId2()
{
	static QString str = GroupId(Note::defaultGroupName2());
	return str;
}

QStringList DataBase::GroupsNames()
{
	return DoSqlQueryGetFirstField("select "+Fields::nameGroup()+" from "+Fields::Groups());
}

QStringPairVector DataBase::GroupsIdsAndNames()
{
	auto table = DoSqlQueryGetTable("select "+Fields::idGroup()+", "+Fields::nameGroup()+" from "+Fields::Groups());
	QStringPairVector v;
	for(auto &row:table) v.emplace_back(std::move(row[0]), std::move(row[1]));
	return v;
}

QStringListVector DataBase::GroupsAllFields()
{
	return DoSqlQueryGetTable("select * from "+Fields::Groups());
}

bool DataBase::IsGroupLocalById(const QString &id)
{
	if(id.toLongLong() > 0) return false;
	else return true;
}

bool DataBase::IsGroupLocalByName(const QString &name)
{
	QString id = GroupId(name);
	return IsGroupLocalById(id);
}

qint64 DataBase::TryCreateNewGlobalGroup(QString name, QString idGroup)
{
	if(name.isEmpty() || name.size() > Fields::maxStringFieldLenth) return -1;

	auto count = CountGroupsWithName(name);
	if(count != '0')
	{
		if(workMode == server) Log("TryCreateNewGlobalGroup: group with name " + name + " exists");
		if(workMode == client) Error("TryCreateNewGlobalGroup: group with name " + name + " exists");
		return -1;
	}

	if(workMode == server)
	{
		if(!idGroup.isEmpty()) { Error("TryCreateNewGlobalGroupidGroup: idGroup not empty in server mode"); return -1; }
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
		if(idGroup.isEmpty()) { Error("TryCreateNewGlobalGroupidGroup is empty in client mode"); return -1; }
		auto count = CountGroupsWithId(idGroup);
		if(count != '0') { Error("TryCreateNewGlobalGroup: group with id " + idGroup + " exists"); return -1; }

		auto request = MakeInsertRequest(Fields::Groups(),
										 {Fields::idGroup(), Fields::nameGroup(), Fields::subscribed()},
										 {idGroup, std::move(name), Fields::True()});
		DoSqlQuery(request.first, request.second);
		count = CountGroupsWithId(idGroup);
		if(count == "1") return idGroup.toLongLong();
		return count.toInt() * -1;
	}

	Error("TryCreateNewGlobalGroup with wrong workMode: " + MyQString::AsDebug(workMode));
	return -1;
}

qint64 DataBase::TryCreateNewLocalGroup(QString name)
{
	if(workMode == server){ Error("TryCreateNewLocalGroup executed in server mode"); return 1; }


	if(name.isEmpty() || name.size() > Fields::maxStringFieldLenth) return 1;

	auto count = CountGroupsWithName(name);
	if(count != '0')
	{
		Error("TryCreateNewLocalGroup: group with name " + name + " exists");
		return 1;
	}

	auto minId = DoSqlQueryGetFirstCell("select min("+Fields::idGroup()+") from "+Fields::Groups());
	QString idGroup = QSn(minId.toLongLong()-1);

	if(idGroup.isEmpty()) { Error("TryCreateNewLocalGroup: is empty in client mode"); return 1; }
	count = CountGroupsWithId(idGroup);
	if(count != '0') { Error("TryCreateNewLocalGroup: group with id " + idGroup + " exists"); return 1; }

	auto request = MakeInsertRequest(Fields::Groups(),
									 {Fields::idGroup(), Fields::nameGroup(), Fields::subscribed()},
									 {idGroup, std::move(name), Fields::True()});
	DoSqlQuery(request.first, request.second);
	count = CountGroupsWithId(idGroup);
	if(count == "1") return idGroup.toLongLong();
	return count.toInt() * -1;
}

void DataBase::SetGroupSubscribed(QString groupId, bool value)
{
	auto request = MakeUpdateRequestOneField(Fields::Groups(), Fields::subscribed(),
											 value ? Fields::True() : Fields::False(),
											 Fields::idGroup(), std::move(groupId));
	DoSqlQuery(request.first, request.second);
}

bool DataBase::MoveNoteToGroupOnClient(QString noteId, QString newGroupId, QString dtUpdated)
{
	auto request = MakeUpdateRequest(Fields::Notes(), {Fields::idGroup(), Fields::dtLastUpdated()}, {newGroupId, dtUpdated},
									 {Fields::idNote()}, {noteId});
	auto res = DoSqlQueryExt(request.first, request.second);
	if(!res.errors.isEmpty()) { Error("SaveNote update sql error: " + res.errors); return false; }
	else { Log("note "+noteId+" moved to group " + newGroupId); return true; }
}

bool DataBase::MoveNoteToGroupOnServer(QString noteIdOnServer, QString newGroupId, QString dtUpdated)
{
	auto request = MakeUpdateRequest(Fields::Notes(), {Fields::idGroup(), Fields::dtLastUpdated()}, {newGroupId, dtUpdated},
									 {Fields::idNoteOnServer()}, {noteIdOnServer});
	auto res = DoSqlQueryExt(request.first, request.second);
	if(!res.errors.isEmpty()) { Error("SaveNote update sql error: " + res.errors); return false; }
	else { return true; }
}

qint64 DataBase::InsertNoteInClientDB(Note *note)
{
	auto res = DoSqlQueryExt("insert into " + Fields::Notes() + " ("
							 +Fields::idNoteOnServer()+", "
							 +Fields::idGroup()+", " + Fields::nameNote() + ", "
							 +Fields::activeNotify() + ", " + Fields::dtNotify() + ", "
							 +Fields::dtPostpone() + ", " + Fields::content() + ", "
							 +Fields::dtLastUpdated()+")\n"
							 +"values (:idOnServer, :groupId, :name, :actNotif, :dtNotif, :dtPosp, :content, :updated)",
					{{":idOnServer", QSn(note->idOnServer)},
					 {":groupId", DataBase::GroupId(note->group)},
					 {":name", note->Name()}, {":actNotif", QSn(note->activeNotify)},
					 {":dtNotif", note->DTNotify().toString(Fields::dtFormat())},
					 {":dtPosp", note->DTPostpone().toString(Fields::dtFormat())},
					 {":content", note->Content()},
					 {":updated", note->DtLastUpdatedStr()},
					});

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

qint64 DataBase::InsertNoteInServerDB(Note * note)
{
	QString idGroup = GroupId(note->group);
	if(idGroup.isEmpty()) { Error("InsertNoteInServerDB in not existing group "+note->group); return -1; }

	auto res = DoSqlQueryExt("insert into " + Fields::Notes() + " ("+Fields::idNote()+", "+Fields::idGroup()+", " + Fields::nameNote() + ", "
							 +Fields::activeNotify()+", " + Fields::dtNotify() + ", " + Fields::dtPostpone() + ", " + Fields::content()+ ", "
							 +Fields::dtLastUpdated()+")\n"
							 + "values (:idNote, :groupId, :name, :actNotif, :dtNotif, :dtPosp, :content, :dtUpdated)",
							 {{":idNote", QSn(note->id)}, {":groupId", idGroup}, {":name", note->Name()}, {":actNotif", QSn(note->activeNotify)},
					  {":dtNotif", note->DTNotify().toString(Fields::dtFormat())}, {":dtPosp", note->DTPostpone().toString(Fields::dtFormat())},
					  {":content", note->Content()}, {":dtUpdated", note->DtLastUpdatedStr()}});

	if(res.errors.isEmpty())
	{
		auto idOnServer = DoSqlQueryGetFirstCell("select max("+Fields::idNoteOnServer()+") from " + Fields::Notes());
		bool ok;
		note->idOnServer = idOnServer.toUInt(&ok);
		if(!ok)
		{
			note->idOnServer = -1;
			Error("bad idOnServer " + idOnServer);
		}

	}
	else Error("note was not inserted");

	return note->idOnServer;
}

QStringList DataBase::NoteByIdOnClient(const QString &id)
{
	return DoSqlQueryGetFirstRec("select * from "+Fields::Notes()+" where "+Fields::idNote()+" = " + id);
}

QStringList DataBase::NoteByIdOnServer(const QString & idOnServer)
{
	return DoSqlQueryGetFirstRec("select * from "+Fields::Notes()+" where "+Fields::idNoteOnServer()+" = " + idOnServer);
}

std::pair<bool, QStringList> DataBase::NoteByIdOnServerWithCheck(const QString &idOnServer)
{
	if(CheckNoteIdOnServer(idOnServer)) return {true, NoteByIdOnServer(idOnServer)};
	else return {false, {}};
}

bool DataBase::CheckNoteIdOnClient(const QString &id)
{
	return DoSqlQueryGetFirstCell("select count("+Fields::idNote()+") from "+Fields::Notes()
								  +" where "+Fields::idNote()+" = " + id).toInt();
}

bool DataBase::CheckNoteIdOnServer(const QString & idOnServer)
{
	return DoSqlQueryGetFirstCell("select count("+Fields::idNoteOnServer()+") from "+Fields::Notes()
								  +" where "+Fields::idNoteOnServer()+" = " + idOnServer).toInt();
}

std::vector<Note> DataBase::NotesFromBD()
{
	std::vector<Note> notes;
	auto table = DoSqlQueryGetTable("select * from "+Fields::Notes()
			+" inner join "+Fields::Groups()+" on "+Fields::Groups()+"."+Fields::idGroup()+" = "+Fields::Notes()+"."+Fields::idGroup()
			+" where "+Fields::subscribed()+" = "+Fields::True()
			+" order by "+Fields::idNote());
	for(auto &row:table)
	{
		notes.emplace_back();
		notes.back().InitFromRecord(row);
	}
	return notes;
}



bool DataBase::SetNoteFieldIdOnServer_OnClient(const QString & idNote, const QString & idOnServer)
{
	if(!CheckNoteIdOnClient(idNote)) { Error("SetNoteIdOnServer error: CheckNoteId " + idNote + " false"); return false; }
	auto r = MakeUpdateRequestOneField(Fields::Notes(), Fields::idNoteOnServer(), idOnServer, Fields::idNote(), idNote);
	auto res = DoSqlQueryExt(r.first, r.second);
	if(!res.errors.isEmpty())  { Error("SetNoteIdOnServer error: " + res.errors); return false; }
	else return true;
}

QString DataBase::SaveNoteOnClient(Note *note)
{
	// если это новая
	if(note->id == Note::idMarkerCreateNewNote)
	{
		InsertNoteInClientDB(note);
		if(!CheckNoteIdOnClient(QSn(note->id))) return "SaveNote: note "+note->Name()+" save error";
	}
	else // если не новая
	{
		if(!CheckNoteIdOnClient(QSn(note->id)))
		{
			return "SaveNote: note with id "+QSn(note->id)+" doesnt exists";
		}

		auto res = DoSqlQueryExt("update "+Fields::Notes()+" set "
								 +Fields::nameNote() + " = :name, "
								 +Fields::idNoteOnServer() + " = :idOnServer, "
								 +Fields::activeNotify()+ " = :actNotif, "
								 +Fields::dtNotify()+" = :dtNotif, "
								 +Fields::dtPostpone()+ " = :dtPosp, "
								 + Fields::content() + " = :content, "
								 +Fields::dtLastUpdated() + " = :updated\n"

								"where "+Fields::idNote()+" = :idNote",

								 {{":idNote", QSn(note->id)},
								  {":idOnServer", QSn(note->idOnServer)},
								  {":name", note->Name()},
								  {":actNotif", QSn(note->activeNotify)},
								  {":dtNotif", note->DTNotify().toString(Fields::dtFormat())},
								  {":dtPosp", note->DTPostpone().toString(Fields::dtFormat())},
								  {":content", note->Content()},
								  {":updated", note->DtLastUpdatedStr()}
								 });

		if(!res.errors.isEmpty()) Error("SaveNote update sql error: " + res.errors);
	}

	return "";
}

bool DataBase::SaveNoteOnServer(Note * note)
{
	if(!CheckNoteIdOnServer(QSn(note->idOnServer)))
	{
		Error("SaveNote: note with idOnServer "+QSn(note->idOnServer)+" doesnt exists");
		return false;
	}

	auto res = DoSqlQueryExt("update "+Fields::Notes()+" set "+Fields::nameNote() + " = :name, "
							 +Fields::activeNotify()+ " = :actNotif, "
							 +Fields::dtNotify()+" = :dtNotif, "+Fields::dtPostpone()+ " = :dtPosp, "
							 +Fields::content() + " = :content, "
							 +Fields::dtLastUpdated() + " = :dtUpdated\n"
													"where "+Fields::idNoteOnServer()+" = :idOnServer",

							 {{":idOnServer", QSn(note->idOnServer)},
							  {":name", note->Name()},
							  {":actNotif", QSn(note->activeNotify)},
							  {":dtNotif", note->DTNotify().toString(Fields::dtFormat())},
							  {":dtPosp", note->DTPostpone().toString(Fields::dtFormat())},
							  {":content", note->Content()},
							  {":dtUpdated", note->DtLastUpdatedStr()}});

	if(res.errors.isEmpty()) return true;

	Error("SaveNote update sql error: " + res.errors);
	return false;
}

bool DataBase::RemoveNoteOnClient(const QString &id, bool chekId)
{	
	if(chekId && !CheckNoteIdOnClient(id))
	{
		QMbError("RemoveNote: note with id "+id+" doesnt exists");
		return false;
	}

	DoSqlQuery("delete from "+Fields::Notes()+" where "+Fields::idNote()+" = :idNote",
							 {{":idNote", id}});

	if(chekId && CheckNoteIdOnClient(id))
	{
		QMbError("RemoveNote: note with id "+id+" after delete sql continue exist");
		return false;
	}

	return true;
}

bool DataBase::RemoveNoteOnServer(const QString & idOnServer, bool chekId)
{
	if(chekId && !CheckNoteIdOnServer(idOnServer))
	{
		Error("RemoveNote: note with idOnServer "+idOnServer+" doesnt exists");
		return false;
	}

	DoSqlQuery("delete from "+Fields::Notes()+" where "+Fields::idNoteOnServer()+" = :idNote",
							 {{":idNote", idOnServer}});

	if(chekId && CheckNoteIdOnServer(idOnServer))
	{
		Error("RemoveNote: note with idOnServer "+idOnServer+" after delete sql continue exist");
		return false;
	}

	return true;
}

QString DataBase::HighestIdOnServer()
{
	return DoSqlQueryGetFirstCell("select max("+Fields::idNoteOnServer()+") from "+Fields::Notes());
}

std::vector<QStringList> DataBase::NotesWithHigherIdOnServer(const QString &idOnServer)
{
	return DoSqlQueryGetTable("select * from "+Fields::Notes()+" where "+Fields::idNoteOnServer()+" > " + idOnServer);
}

//QStringPairVector DataBase::NotesWithHigherIdOnServer(const QString &idOnServer)
//{
//	return DoSqlQueryGetFirstTwoFields("select "+Fields::idNoteOnServer()+", "+Fields::idGroup()
//									   +" from notes where "+Fields::idNoteOnServer()+" > " + idOnServer);
//}




