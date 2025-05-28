#ifndef FIELDS_H
#define FIELDS_H

#include <QString>

namespace SaveKeyWods {

	inline const QString& endValue() { static QString str = "[endValue]\n"; return str; }
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
	inline static const QString& Notes() { static QString str = "Notes"; return str; }
	inline static const QString& Groups() { static QString str = "Groups"; return str; }

	inline static const QString& nameGroup() { static QString str = "nameGroup"; return str; }

	inline static const QString& idNote			() { static QString str = "idNote"; return str; }
	//inline static const QString& idNoteOnServer	() { static QString str = "idNoteOnServer"; return str; }
	inline static const QString& idGroup		() { static QString str = "idGroup"; return str; }
	inline static const QString& nameNote		() { static QString str = "nameNote"; return str; }
	inline static const QString& activeNotify	() { static QString str = "activeNotify"; return str; }
	inline static const QString& dtNotify		() { static QString str = "dtNotify"; return str; }
	inline static const QString& dtPostpone		() { static QString str = "dtPostpone"; return str; }
	inline static const QString& content		() { static QString str = "content"; return str; }

	const int idNoteInd			= 0;
	const int idNoteOnServerInd	= idNoteInd+1;
	const int idGroupInd		= idNoteOnServerInd+1;
	const int nameNoteInd		= idGroupInd+1;
	const int activeNotifyInd	= nameNoteInd+1;
	const int dtNotifyInd		= activeNotifyInd+1;
	const int dtPostponeInd		= dtNotifyInd+1;
	const int contentInd		= dtPostponeInd+1;

	const int maxStringFieldLenth = 255;

	inline const QString& dtFormat() { static QString str = SaveKeyWods::dtFormat(); return str; }
}

#endif // FIELDS_H
