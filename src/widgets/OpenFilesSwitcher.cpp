/*
 * This file is part of Notepad Next.
 * Copyright 2026
 *
 * Notepad Next is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "OpenFilesSwitcher.h"

#include "MainWindow.h"
#include "ScintillaNext.h"

#include <QAbstractItemView>
#include <QEvent>
#include <QFileInfo>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QRect>
#include <QStringList>
#include <QVariant>
#include <QVBoxLayout>

#include <algorithm>

OpenFilesSwitcher::OpenFilesSwitcher(MainWindow *window) :
    QFrame(window),
    window(window),
    input(new QLineEdit(this)),
    results(new QListWidget(this)),
    emptyLabel(new QLabel(tr("No open files found"), this))
{
    setObjectName(QStringLiteral("OpenFilesSwitcher"));
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setFocusProxy(input);

    input->setPlaceholderText(tr("Search open files"));
    input->installEventFilter(this);

    results->setFocusPolicy(Qt::NoFocus);
    results->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    results->setSelectionMode(QAbstractItemView::SingleSelection);

    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->hide();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);
    layout->addWidget(input);
    layout->addWidget(results);
    layout->addWidget(emptyLabel);

    setStyleSheet(QStringLiteral(
        "#OpenFilesSwitcher {"
        "  background: palette(window);"
        "  border: 1px solid palette(mid);"
        "  border-radius: 6px;"
        "}"
        "#OpenFilesSwitcher QLineEdit {"
        "  min-height: 28px;"
        "  padding: 4px 8px;"
        "}"
        "#OpenFilesSwitcher QListWidget {"
        "  border: 1px solid palette(mid);"
        "  outline: none;"
        "}"
        "#OpenFilesSwitcher QListWidget::item {"
        "  padding: 6px 8px;"
        "}"
    ));

    connect(input, &QLineEdit::textChanged, this, &OpenFilesSwitcher::filterFiles);
    connect(results, &QListWidget::itemActivated, this, &OpenFilesSwitcher::switchToCurrentFile);
}

void OpenFilesSwitcher::showSwitcher()
{
    allEditors = window->editors();

    input->clear();
    filterFiles();
    positionWidget();
    show();
    raise();
    activateWindow();
    input->setFocus(Qt::ShortcutFocusReason);
}

bool OpenFilesSwitcher::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == input && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

        switch (keyEvent->key()) {
        case Qt::Key_Down:
            selectNextResult();
            return true;
        case Qt::Key_Up:
            selectPreviousResult();
            return true;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            switchToCurrentFile();
            return true;
        case Qt::Key_Escape:
            hide();
            if (auto *editor = window->currentEditor()) {
                editor->grabFocus();
            }
            return true;
        default:
            break;
        }
    }

    return QFrame::eventFilter(obj, event);
}

void OpenFilesSwitcher::showEvent(QShowEvent *event)
{
    QFrame::showEvent(event);
    input->setFocus(Qt::ShortcutFocusReason);
}

void OpenFilesSwitcher::filterFiles()
{
    results->clear();

    const QString query = input->text().trimmed();
    const QStringList terms = query.toCaseFolded().split(QChar::Space, Qt::SkipEmptyParts);

    for (auto *editor : allEditors) {
        const QString haystack = searchableText(editor).toCaseFolded();
        const bool matches = std::all_of(terms.cbegin(), terms.cend(), [&haystack](const QString &term) {
            return haystack.contains(term);
        });

        if (matches) {
            addEditorResult(editor);
        }
    }

    if (results->count() > 0) {
        results->setCurrentRow(0);
    }

    const bool hasResults = results->count() > 0;
    results->setVisible(hasResults);
    emptyLabel->setVisible(!hasResults);
}

void OpenFilesSwitcher::switchToCurrentFile()
{
    ScintillaNext *editor = currentResultEditor();

    if (!editor) {
        return;
    }

    hide();
    window->switchToEditor(editor);
    editor->grabFocus();
}

void OpenFilesSwitcher::positionWidget()
{
    const QRect host = window->geometry();
    const int width = qBound(320, host.width() - 80, 640);
    const int height = 320;
    resize(width, height);
    move(host.x() + (host.width() - width) / 2, host.y() + 72);
}

void OpenFilesSwitcher::selectNextResult()
{
    const int count = results->count();
    if (count == 0) {
        return;
    }

    const int row = results->currentRow();
    results->setCurrentRow(row < count - 1 ? row + 1 : 0);
}

void OpenFilesSwitcher::selectPreviousResult()
{
    const int count = results->count();
    if (count == 0) {
        return;
    }

    const int row = results->currentRow();
    results->setCurrentRow(row > 0 ? row - 1 : count - 1);
}

void OpenFilesSwitcher::addEditorResult(ScintillaNext *editor)
{
    QString label = editor->getName();
    QString toolTip = label;

    if (editor->isFile()) {
        const QFileInfo fileInfo(editor->getFilePath());
        label = fileInfo.fileName();
        toolTip = editor->getFilePath();

        const QString path = fileInfo.path();
        if (!path.isEmpty()) {
            label += QStringLiteral("  -  ") + path;
        }
    }

    if (!editor->isSavedToDisk()) {
        label.prepend(QStringLiteral("* "));
    }

    QListWidgetItem *item = new QListWidgetItem(label, results);
    item->setToolTip(toolTip);
    item->setData(Qt::UserRole, QVariant::fromValue<quintptr>(reinterpret_cast<quintptr>(editor)));
}

ScintillaNext *OpenFilesSwitcher::currentResultEditor() const
{
    QListWidgetItem *item = results->currentItem();

    if (!item) {
        return nullptr;
    }

    return reinterpret_cast<ScintillaNext *>(item->data(Qt::UserRole).value<quintptr>());
}

QString OpenFilesSwitcher::searchableText(ScintillaNext *editor) const
{
    if (editor->isFile()) {
        return editor->getName() + QChar::Space + editor->getFilePath();
    }

    return editor->getName();
}
