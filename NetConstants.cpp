#include "NetConstants.h"

#include "CodeMarkers.h"

const int NetConstants::pollyMaxWaitServerMs = 25000;
const int NetConstants::pollyMaxWaitClientMs = 30000;

QString NetConstants::MakeAnsw_request_get_session(QString sessionId, const QString &sessionDt)
{
	sessionId.append(" ").append(sessionDt);
	return sessionId;
}

QStringPair NetConstants::GetIdAndDtFromAnswGetSession(const QString &answ)
{
	auto words = answ.split(' ');
	if(words.size() == 2)
		return {std::move(words[0]), std::move(words[1])};
	else return {"",""};
}

QString NetConstants::MakeRequest_move_note_to_group(const QString & idNoteOnServer, const QString & idNewGroup, const QDateTime & dtUpdated)
{
	return QString(idNoteOnServer).append(" ").append(idNewGroup).append(" ")
			.append(dtUpdated.toString(Fields::dtFormatLastUpated()));
}

NetConstants::Rqst_data_move_note_to_group NetConstants::GetDataFromRequest_move_note_to_group(const QString & text)
{
	auto words = text.split(' ');
	if(words.size() == 3)
		return {std::move(words[0]), std::move(words[1]), std::move(words[3])};
	else return {};
}

qint64 NetConstants::GetInt64_IfFirstWordIsSuccess(const QString & text)
{
	if(!text.startsWith(NetConstants::success())) return -1;
	bool ok;
	int64_t id = text.right(text.size() - (NetConstants::success().size() + 1)  ).toLongLong(&ok);
	if(!ok) return -2;
	return id;
}

QString NetConstants::MakeRequest_synch_note(std::vector<SynchData> datas)
{
	if(datas.empty()) { qdbg << "MakeRequest_synch_note get empty"; return {}; }
	QString res;
	for(uint i=0; i<datas.size(); i++)
		res.append(datas[i].idOnServer).append(',').append(datas[i].dtUpdatedStr).append(',');
	res.chop(1);
	return res;
}

std::vector<NetConstants::SynchData> NetConstants::GetDataFromRequest_synch_note(const QString & text)
{
	if(text.isEmpty()) return {};
	std::vector<SynchData> datas;
	auto parts = text.split(',');
	if(parts.size()%2 != 0) { qdbg << "ERROR!!! bad Request_synch_note content: " + text; return {}; }
	if(0) CodeMarkers::to_do("здесь нужно убрать qdbg и делать какой-то полноценный вывод ошибок кудато");
	for(int i=0; i<parts.size(); i+=2)
	{
		auto &data = datas.emplace_back();
		data.idOnServer = parts[i];
		data.dtUpdatedStr = parts[i+1];
	}
	return datas;
}

QString NetConstants::request_group_check_notes_prepare(QString &&idGroup, const QStringPairVector &idsNotes_dtsLastUpdated)
{
	QString &str = idGroup;
	str.append(",");
	for(auto &idNote_dtLastUpdated:idsNotes_dtsLastUpdated)
	{
		str.append(idNote_dtLastUpdated.first).append(',').append(idNote_dtLastUpdated.second);
		str.append(',');
	}
	str.chop(1);
	return str;
}

std::tuple<bool, QString, QStringPairVector> NetConstants::request_group_check_notes_decode(const QString &str)
{
	auto listRefs = str.splitRef(',');
	if(listRefs.isEmpty()) return {false, {}, {}};
	std::tuple<bool, QString, QStringPairVector> result = {true, {}, {}};
	std::get<1>(result) = listRefs[0].toString();
	for(int i=1; i<listRefs.size(); i+=2)
	{
		if(i+1 >= listRefs.size()) { std::get<0>(result) = false; break; }

		auto idNote_dtUpdated = std::get<2>(result).emplace_back();
		idNote_dtUpdated.first = listRefs[i].toString();
		idNote_dtUpdated.second = listRefs[i+1].toString();
	}
	return result;
}




