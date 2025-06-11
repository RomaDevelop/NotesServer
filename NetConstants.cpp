#include "NetConstants.h"

#include "CodeMarkers.h"

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
		res.append(datas[i].idOnSever).append(',').append(datas[i].dtUpdatedStr).append(',');
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
		data.idOnSever = parts[i];
		data.dtUpdatedStr = parts[i+1];
	}
	return datas;
}
