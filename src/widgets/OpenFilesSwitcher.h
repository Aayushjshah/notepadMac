/*
 * This file is part of Notepad Next.
 * Copyright 2026
 *
 * Notepad Next is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef OPENFILESSWITCHER_H
#define OPENFILESSWITCHER_H

#include <QFrame>
#include <QVector>

class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class MainWindow;
class ScintillaNext;

class OpenFilesSwitcher : public QFrame
{
    Q_OBJECT

public:
    explicit OpenFilesSwitcher(MainWindow *window);

    void showSwitcher();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void filterFiles();
    void switchToCurrentFile();

private:
    void positionWidget();
    void selectNextResult();
    void selectPreviousResult();
    void addEditorResult(ScintillaNext *editor);
    ScintillaNext *currentResultEditor() const;
    QString searchableText(ScintillaNext *editor) const;

    MainWindow *window;
    QLineEdit *input;
    QListWidget *results;
    QLabel *emptyLabel;
    QVector<ScintillaNext *> allEditors;
};

#endif // OPENFILESSWITCHER_H
