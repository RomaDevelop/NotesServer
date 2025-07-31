#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <memory>
#include <set>
#include <functional>

#include <QDebug>
#include <QStringList>
#include <QDateTime>

#include "MyQShortings.h"
#include "declare_struct.h"

#include "Fields.h"

using QStringPair = std::pair<QString, QString>;
using QStringPairVector = std::vector<QStringPair>;

using QStringListVector = std::vector<QStringList>;

using QStringRefWr_const = std::reference_wrapper<const QString>;

struct Note;

struct NetConstants
{
	static const QString& test_passwd() { static QString str = "test password ~csdjh!~@$34cs"; return str; }

	static const QString& invalid() { static QString str = "invalid"; return str; }
	static const QString& success() { static QString str = "success"; return str; }
	static const QString& not_did() { static QString str = "not_did"; return str; }

	static const QString& auth() { static QString str = "auth: "; return str; }
	static const QString& auth_failed() { static QString str = "auth_failed"; return str; }

	static const QString& auth_date_time_format() { static QString str = "yyyy.MM.dd_hh:mm:ss.zzz"; return str; }

	static const QString& request() { static QString str = "request: "; return str; }
	static const QString& request_answ() { static QString str = "request_answ: "; return str; }

	static const QString& request_get_session() { static QString str = "request_get_session"; return str; }
	static QString MakeAnsw_request_get_session(QString sessionId, const QString &sessionDt);
	static QStringPair GetIdAndDtFromAnswGetSession(const QString& answ);

	static const QString& request_try_create_group() { static QString str = "request_try_create_group"; return str; }
	static QString MakeAnsw_try_create_group(const QString& groupId) { return QString(NetConstants::success()).append(' ').append(groupId); }
	static qint64 GetFromAnsw_try_create_group_IdGroup(const QString& answ) { return GetInt64_IfFirstWordIsSuccess(answ); }

	static const QString& request_create_note_on_server() { static QString str = "request_create_note_on_server"; return str; }
	// клиент отправляет всю заметку с нужным значением группы
	static QString MakeAnsw_create_note_on_server(const QString& idNote) { return QString(NetConstants::success()).append(' ').append(idNote); }
	static qint64 GetFromAnsw_create_note_on_server_IdNoteOnServer(const QString& answ) { return GetInt64_IfFirstWordIsSuccess(answ); }

	static const QString& request_move_note_to_group() { static QString str = "request_move_note_to_group"; return str; }
	static QString MakeRequest_move_note_to_group(const QString &idNote, const QString &idNewGroup, const QDateTime &dtUpdated);
	declare_struct_3_fields_move(Rqst_data_move_note_to_group, QString, idNote, QString, idNewGroup, QString, dtLastUpdatedStr);
	static Rqst_data_move_note_to_group GetDataFromRequest_move_note_to_group(const QString &text);

	static const QString& request_remove_note() { static QString str = "request_remove_note"; return str; }
	// клиент отправляет idOnServer, сервер отвечает success

	static const QString& request_note_saved() { static QString str = "note_saved"; return str; }
	// клиент отправляет всю заметку, сервер отвечает success

	static const QString& request_get_note() { static QString str = "get_note"; return str; }
	// один отправляет idOnServer, второй присылает заметку или invalid

	static const QString& request_synch_note() { static QString str = "synch_note"; return str; }
	// клиент отправляет idOnServer и DtUpdated от нескольких заметок, сервер отвечает success
	declare_struct_2_fields_move(SynchData, QString, idNote, QString, dtUpdatedStr);
	static QString MakeRequest_synch_note(std::vector<SynchData> datas);
	static std::vector<SynchData> GetDataFromRequest_synch_note(const QString &text);
	static QString request_synch_res_success() { static QString str = "1"; return str; }
	static QString request_synch_res_error() { static QString str = "0"; return str; }
	//static const QString& request_get_new_notes() { static QString str = "get_new_notes"; return str; }
	static const QString& request_group_check_notes() { static QString str = "group_check_notes"; return str; }
	static const QString& request_all_notes() { static QString str = "all_notes"; return str; }

	static const QString& request_polly() { static QString str = "polly"; return str; }

	static const std::set<QStringRefWr_const>& allReuestTypes() { // сет нужен, нельзя заменить мапой из сервера потому что используется в клиенте
		static std::set<QStringRefWr_const> setSinglton {
					std::cref(request_get_session()),
					std::cref(request_try_create_group()),
					std::cref(request_create_note_on_server()),
					std::cref(request_move_note_to_group()),
					std::cref(request_remove_note()),
					std::cref(request_note_saved()),
					std::cref(request_get_note()),
					std::cref(request_synch_note()),
					//std::cref(request_get_new_notes()),
					std::cref(request_group_check_notes()),
					std::cref(request_all_notes()),

					std::cref(request_polly()),
		};
		return setSinglton;
	}

	static QString request_group_check_notes_prepare(QString &&idGroup, const QStringPairVector &idsNotes_dtsLastUpdated);
	static std::tuple<bool, QString, QStringPairVector> request_group_check_notes_decode(const QString &str);

	static QString request_all_notes_prepare_answ(QStringListVector &recordsNotes);
	static QStringList request_all_notes_decode_answ(const QString &allNotes);

	static const QString& msg() { static QString str = "msg: "; return str; }
	static const QString& msg_error() { static QString str = "error"; return str; }
	static const QString& msg_all_client_notes_synch_sended() { static QString str = "acnss"; return str; }
	static const QString& msg_highest_idOnServer() { static QString str = "hidos"; return str; }
	// клиент отправляет самый старший idOnServer

	static const std::set<QStringRefWr_const>& allMsgsTypes() { // сет нужен, нельзя заменить мапой из сервера потому что используется в клиенте
		static std::set<QStringRefWr_const> setSinglton {
					std::cref(msg_error()),
					std::cref(msg_all_client_notes_synch_sended()),
					std::cref(msg_highest_idOnServer()),
		};
		return setSinglton;
	}

	static const QString& command_to_client() { static QString str = "c_c: "; return str; }
	static const QString& command_your_session_id() { static QString str = "your_session_id"; return str; }
	static const QString& command_remove_note() { static QString str = "remove_note"; return str; }
	// сервер отправляет idOnServer
	static const QString& command_update_note() { static QString str = "update_note"; return str; }
	// сервер отправляет всю заметку

	static qint64 GetInt64_IfFirstWordIsSuccess(const QString& text);

	static const QString& nothing() { static QString str = "nothing"; return str; }

	static const QString& unexpected() { static QString str = "unexpected"; return str; }

	static const QString& end_marker() { static QString str = ";"; return str; }
	static const QString& end_marker_replace() { static QString str = "#%&0"; return str; }

	static const int pollyMaxWaitServerMs;
	static const int pollyMaxWaitClientMs;
};



#endif // CONSTANTS_H
