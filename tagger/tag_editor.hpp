#ifndef TAG_EDITOR_HPP
#define TAG_EDITOR_HPP


#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QGridLayout>
#include <QLabel>

#include "file.hpp"


class tag_editor : public QWidget{
public:
	QLineEdit* artist{new QLineEdit{this}};
	QLineEdit* album{new QLineEdit{this}};
	QLineEdit* title{new QLineEdit{this}};
	QLineEdit* genre{new QLineEdit{this}};

	QPushButton* save{new QPushButton{"Save", this}};
	QPushButton* clear{new QPushButton{"Clear", this}};
	QPushButton* reset{new QPushButton{"Reset", this}};

	tag_editor(QWidget* parent, file* current) : QWidget{parent}{
		set_layout();
		set_current(current);
	}

private:
	void set_current(file* current){
		set_inputs(current->properties);
	}

	void set_layout(){
		auto* main = new QGridLayout{};
		int row = 0;

		main->addWidget(new QLabel{"Artist", this}, row, 0, Qt::AlignRight);
		main->addWidget(artist, row++, 1);

		main->addWidget(new QLabel{"Album", this}, row, 0, Qt::AlignRight);
		main->addWidget(album, row++, 1);

		main->addWidget(new QLabel{"Title", this}, row, 0, Qt::AlignRight);
		main->addWidget(title, row++, 1);

		main->addWidget(new QLabel{"Genre", this}, row, 0, Qt::AlignRight);
		main->addWidget(genre, row++, 1);

		auto* box = new QHBoxLayout;

		box->addStretch();
		box->addWidget(save);
		box->addWidget(clear);
		box->addWidget(reset);

		main->addLayout(box, row++, 0, 1, 2);

		setLayout(main);
	}

public slots:
	void clear_inputs(){
		artist->setText("");
		album->setText("");
		title->setText("");
		genre->setText("");
	}

	void reset_inputs(const tl::PropertyMap& properties){
		set_inputs(properties);
	}

	void set_inputs(const tl::PropertyMap& properties){
		artist->setText(properties["ARTIST"].toString(";").toCString());
		album->setText(properties["ALBUM"].toString(";").toCString());
		title->setText(properties["TITLE"].toString(";").toCString());
		genre->setText(properties["GENRE"].toString(";").toCString());
	}
};


#endif
