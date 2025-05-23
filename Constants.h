#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>

namespace Constants {
	inline const QString& test_passwd() { static QString str = "test password ~csdjh!~@$34cs"; return str; }

	inline const QString& invalid() { static QString str = "invalid"; return str; }

	inline const QString& auth() { static QString str = "auth: "; return str; }
	inline const QString& auth_success() { static QString str = "auth_success"; return str; }
	inline const QString& auth_failed() { static QString str = "auth_failed"; return str; }

	inline const QString& last_update() { static QString str = "last_update: "; return str; }

	inline const QString& unexpacted() { static QString str = "unexpacted"; return str; }


}

#endif // CONSTANTS_H
