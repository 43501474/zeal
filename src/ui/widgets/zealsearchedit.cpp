#include "zealsearchedit.h"

#include "registry/searchquery.h"

#include <QKeyEvent>

ZealSearchEdit::ZealSearchEdit(QWidget *parent) :
    QLineEdit(parent)
{
    setClearButtonEnabled(true);

    completionLabel = new QLabel(this);
    completionLabel->setObjectName("completer");
    completionLabel->setStyleSheet("QLabel#completer { color: gray; }");
    completionLabel->setFont(font());

    connect(this, &ZealSearchEdit::textChanged, this, &ZealSearchEdit::showCompletions);
}

void ZealSearchEdit::setTreeView(QTreeView *view)
{
    treeView = view;
    focusing = false;
    installEventFilter(this);
}

// Makes the line edit use autocompletions.
void ZealSearchEdit::setCompletions(const QStringList &completions)
{
    prefixCompleter = new QCompleter(completions, this);
    prefixCompleter->setCompletionMode(QCompleter::InlineCompletion);
    prefixCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    prefixCompleter->setWidget(this);
}

int ZealSearchEdit::queryStart()
{
    Zeal::SearchQuery currentQuery(text());
    // Keep the filter for the first esc press
    if (currentQuery.docsetFilterSize() > 0 && currentQuery.coreQuery().size() > 0)
        return currentQuery.docsetFilterSize() + 1;
    else
        return 0;
}

// Clear input with consideration to docset filters
void ZealSearchEdit::clearQuery()
{
    QString query = text();
    query.chop(query.size() - queryStart());
    setText(query);
}

void ZealSearchEdit::selectQuery()
{
    setSelection(queryStart(), text().size());
}

void ZealSearchEdit::clear()
{
    clearQuery();
}

bool ZealSearchEdit::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == this && ev->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(ev);

        if (keyEvent->key() == Qt::Key_Down || keyEvent->key() == Qt::Key_Up) {
            QModelIndex index = treeView->currentIndex();
            int nextRow = keyEvent->key() == Qt::Key_Down
                          ? index.row() + 1
                          : index.row() - 1;
            QModelIndex sibling = index.sibling(nextRow, 0);
            if (nextRow >= 0 && nextRow < treeView->model()->rowCount()) {
                treeView->setCurrentIndex(sibling);
                return true;
            }
        }

        if (keyEvent->key() == Qt::Key_Return) {
            emit treeView->activated(treeView->selectionModel()->currentIndex());
            return QLineEdit::eventFilter(obj, ev);
        }

        // Autocompletes the prefixes.
        if (keyEvent->key() == Qt::Key_Tab) {
            QString currentText = text();
            QString completed = currentCompletion(currentText);
            if (!completed.isEmpty()) {
                setText(completed);
                return true;
            }
        }
    }
    return QLineEdit::eventFilter(obj, ev);
}

void ZealSearchEdit::resizeEvent(QResizeEvent *ev)
{
    showCompletions(text());
    QLineEdit::resizeEvent(ev);
}

void ZealSearchEdit::focusInEvent(QFocusEvent *evt)
{
    // Focus on the widget.
    QLineEdit::focusInEvent(evt);

    // Override the default selection.
    Zeal::SearchQuery currentQuery(text());
    int selectionOffset = currentQuery.docsetFilterSize();
    if (selectionOffset > 0)
        selectionOffset++; // add the delimeter
    setSelection(selectionOffset, text().size() - selectionOffset);
    focusing = true;
}

void ZealSearchEdit::keyPressEvent(QKeyEvent *keyEvent)
{
    if (keyEvent->key() == Qt::Key_Escape) {
        clear();
    } else {
        QLineEdit::keyPressEvent(keyEvent);
    }
}

void ZealSearchEdit::mousePressEvent(QMouseEvent *ev)
{
    // Let the focusInEvent code deal with initial selection on focus.
    if (!focusing)
        QLineEdit::mousePressEvent(ev);
    focusing = false;
}

QString ZealSearchEdit::currentCompletion(const QString &text)
{
    if (text.isEmpty())
        return QString();
    else
        return prefixCompleter->currentCompletion();
}

void ZealSearchEdit::showCompletions(const QString &newValue)
{
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    int textWidth = fontMetrics().width(newValue);

    prefixCompleter->setCompletionPrefix(text());

    QString completed = currentCompletion(newValue).mid(newValue.size());
    QSize labelSize(fontMetrics().width(completed), size().height());

    completionLabel->setMinimumSize(labelSize);
    completionLabel->move(frameWidth + 2 + textWidth, frameWidth);
    completionLabel->setText(completed);
}
