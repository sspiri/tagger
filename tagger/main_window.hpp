#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP


#include <functional>

#include <QFileDialog>
#include <QMainWindow>
#include <QTabWidget>
#include <QMessageBox>
#include <QMenuBar>
#include <QFileInfo>

#include "tag_editor.hpp"
#include "file.hpp"


class main_window : public QMainWindow{
public:
	main_window(const std::vector<std::string>& file_paths = {}){
		setCentralWidget(tabs);
		tabs->setTabsClosable(true);

		open_files(file_paths);
		set_menu_bar();
	}

private:
	std::unordered_map<file*, tag_editor*> editors_map;

	QTabWidget* tabs{new QTabWidget{this}};

	void open_files(const std::vector<std::string>& file_paths){
		for(const auto& file : file_paths)
			add_tab(file);
	}

	void add_tab(const std::string& fp){
		auto* fileref = new file(this, fp);
		auto* editor = new tag_editor{this, fileref};

		editors_map[fileref] = editor;
        tabs->addTab(editor, QFileInfo{fp.c_str()}.fileName());

		connect(fileref, &file::error, this, &main_window::show_error);

		connect(editor->artist, &QLineEdit::textEdited, [fileref](const QString& news){ fileref->properties["ARTIST"] = {news.toUtf8().data()}; });
		connect(editor->album, &QLineEdit::textEdited, [fileref](const QString& news){ fileref->properties["ALBUM"] = {news.toUtf8().data()}; });
		connect(editor->genre, &QLineEdit::textEdited, [fileref](const QString& news){ fileref->properties["GENRE"] = {news.toUtf8().data()}; });
		connect(editor->title, &QLineEdit::textEdited, [fileref](const QString& news){ fileref->properties["TITLE"] = {news.toUtf8().data()}; });

		connect(editor->save, &QPushButton::clicked, std::bind(&main_window::save_file, this, fileref));

		connect(editor->clear, &QPushButton::clicked, std::bind(&main_window::clear_file, this, fileref));
		connect(editor->clear, &QPushButton::clicked, editor, &tag_editor::clear_inputs);

		connect(editor->reset, &QPushButton::clicked, std::bind(&main_window::reset_file, this, fileref));
		connect(editor->reset, &QPushButton::clicked, std::bind(&tag_editor::set_inputs, editor, std::cref(fileref->properties)));
	}

	void show_error(const QString& message){
		QMessageBox::critical(this, "Error", message);
	}

    void save_all(){
        for(auto& pair : editors_map)
            pair.first->save();
    }

	void save_file(file* fp){
		fp->save();
	}

	void clear_file(file* fp){
		fp->clear();
	}

	void reset_file(file* fp){
		fp->reset();
	}

	void set_menu_bar(){
		QMenu* menu = menuBar()->addMenu("File");

		connect(menu->addAction("Open..."), &QAction::triggered, this, &main_window::select_open_files);
        connect(menu->addAction("Save all"), &QAction::triggered, this, &main_window::save_all);
		connect(menu->addAction("Exit"), &QAction::triggered, this, &QWidget::close);
	}

	void select_open_files(){
		auto file_paths = QFileDialog::getOpenFileNames(this, "Select one or more files", QDir::homePath());

		for(const auto& fp : file_paths)
			add_tab(fp.toStdString());
	}
};


#endif
