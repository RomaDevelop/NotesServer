#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <set>
#include <functional>

#include <QString>

using QStringRefWr_const = std::reference_wrapper<const QString>;

namespace NetConstants {
	inline const QString& test_passwd() { static QString str = "test password ~csdjh!~@$34cs"; return str; }

	inline const QString& invalid() { static QString str = "invalid"; return str; }
	inline const QString& success() { static QString str = "success"; return str; }
	inline const QString& not_did() { static QString str = "not_did"; return str; }

	inline const QString& auth() { static QString str = "auth: "; return str; }
	inline const QString& auth_success() { static QString str = "auth_success"; return str; }
	inline const QString& auth_failed() { static QString str = "auth_failed"; return str; }

	inline const QString& request() { static QString str = "request: "; return str; }
	inline const QString& request_answ() { static QString str = "request_answ: "; return str; }

	inline const QString& request_group_names() { static QString str = "request_group_names"; return str; }
	inline const QString& request_try_create_group() { static QString str = "request_try_create_group"; return str; }
	inline const std::set<QStringRefWr_const>& allReuestTypes() {
		static std::set<QStringRefWr_const> setSinglton { std::cref(request_group_names()), std::cref(request_try_create_group()) };
		return setSinglton;
	}

	inline static QString Answ_try_create_group(const QString& groupId) { return QString(NetConstants::success()).append(' ').append(groupId); }
	inline static int64_t GetIdGroupFromAnsw_try_create_group(const QString& answ)
	{
		if(!answ.startsWith(NetConstants::success())) return -1;
		bool ok;
		int64_t id = answ.right(answ.size() - (NetConstants::success().size() + 1)  ).toLongLong(&ok);
		if(!ok) return -2;
		return id;
	}

	inline const QString& note_saved() { static QString str = "note_saved: "; return str; }

	inline const QString& last_update() { static QString str = "last_update: "; return str; }

	inline const QString& unexpacted() { static QString str = "unexpacted"; return str; }

	inline const QString& end_marker() { static QString str = ";"; return str; }
	inline const QString& end_marker_replace() { static QString str = "#%&0"; return str; }
}

#endif // CONSTANTS_H
