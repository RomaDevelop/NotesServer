#ifndef FIELDS_H
#define FIELDS_H

#include <QString>

#include "MyQShortings.h"

namespace SaveKeyWods {

	inline const QString& endValue() { static QString str = "[endValue]\n"; return str; }
	inline const QString& endNote() { static QString str = "[endNote]\n"; return str; }
	inline const QString& dtFormat() { static QString str = "yyyy.MM.dd hh:mm:ss"; return str; }

	inline const QString& version() { static QString str = "version: "; return str; }
	inline const QString& name() { static QString str = "name: "; return str; }
	inline const QString& activeNotify() { static QString str = "activeNotify: "; return str; }
	inline const QString& dtNotify() { static QString str = "dtNotify: "; return str; }
	inline const QString& dtPostpone() { static QString str = "dtPostpone: "; return str; }
	inline const QString& group() { static QString str = "group: "; return str; }
	inline const QString& content() { static QString str = "content: "; return str; }
}

namespace Fields {
	inline const QString& Notes() { static QString str = "Notes"; return str; }
	inline const QString& Groups() { static QString str = "Groups"; return str; }
	inline const QString& IdCounter() { static QString str = "IdCounter"; return str; }

	inline const QString& nameGroup() { static QString str = "nameGroup"; return str; }
	inline const QString& subscribed() { static QString str = "subscribed"; return str; }

	inline const QString& idNote			() { static QString str = "idNote"; return str; }
	inline const QString& idGroup			() { static QString str = "idGroup"; return str; }
	inline const QString& nameNote			() { static QString str = "nameNote"; return str; }
	inline const QString& activeNotify		() { static QString str = "activeNotify"; return str; }
	inline const QString& dtCreated			() { static QString str = "dtCreated"; return str; }
	inline const QString& dtNotify			() { static QString str = "dtNotify"; return str; }
	inline const QString& dtPostpone		() { static QString str = "dtPostpone"; return str; }
	inline const QString& content			() { static QString str = "content"; return str; }
	inline const QString& dtLastUpdated		() { static QString str = "lastUpdated"; return str; }
	inline const QString& opensCount		() { static QString str = "opensCount"; return str; }
	inline const QString& notSendedToServer	() { static QString str = "notSendedToServer"; return str; }

	const int idGroupIndexInGroups	= 0;
	const int nameGroupIndex		= idGroupIndexInGroups+1;
	const int subscribedIndex		= nameGroupIndex+1;

	const int idNoteInd			= 0;
	const int idGroupIndInNotes	= idNoteInd+1;
	const int nameNoteInd		= idGroupIndInNotes+1;
	const int activeNotifyInd	= nameNoteInd+1;
	const int dtCreatedInd		= activeNotifyInd+1;
	const int dtNotifyInd		= dtCreatedInd+1;
	const int dtPostponeInd		= dtNotifyInd+1;
	const int contentInd		= dtPostponeInd+1;
	const int lastUpdatedInd	= contentInd+1;

	const int maxStringFieldLenth = 255;

	inline const QString& dtFormat() { static QString str = SaveKeyWods::dtFormat(); return str; }
	inline const QString& dtFormatLastUpdated() { static QString str = DateTimeFormat_ms; return str; }

	inline const QString& True() { static QString str = "1"; return str; }
	inline const QString& False() { static QString str = "0"; return str; }
	inline bool CheckLogicField(const QString &value) { return (value == "1" || value == True()); }
}

#endif // FIELDS_H
