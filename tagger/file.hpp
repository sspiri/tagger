#ifndef FILE_HPP
#define FILE_HPP


#include <stdexcept>
#include <string>

#include <taglib/fileref.h>
#include <taglib/tpropertymap.h>
#include <taglib/tag.h>

#include <QString>
#include <QObject>



namespace tl = TagLib;


class file : public QObject{
	Q_OBJECT

signals:
	void error(const QString& );

public:
	tl::PropertyMap properties;

	file(QObject* parent, const std::string& fp)
		: QObject{parent}, fileref{fp.c_str(), true, tl::AudioProperties::Fast}{

		if(fileref.isNull())
			throw std::runtime_error{"Error: Cannot open for tagging: [" + fp + ']'};

		properties = fileref.tag()->properties();
	}

	void save(){
		if(properties != fileref.tag()->properties()){
			fileref.tag()->setProperties(properties);

			if(!fileref.save())
				emit error("Error: Cannot save file: [" + QString{fileref.file()->name()} + ']');
		}
	}

	void clear(){
		properties = {};
	}

	void reset(){
		properties = fileref.tag()->properties();
	}

private:
	tl::FileRef fileref;
};


#endif
