#ifndef FOCUSCONTROLLER_H
#define FOCUSCONTROLLER_H

#include <QObject>

class QQuickItem;
class QQmlApplicationEngine;

class FocusController : public QObject
{
    Q_OBJECT
public:
    explicit FocusController(QQmlApplicationEngine* engine, QObject *parent = nullptr);
    ~FocusController();

    Q_INVOKABLE void nextKeyTabItem();
    Q_INVOKABLE void previousKeyTabItem();
    Q_INVOKABLE void nextKeyUpItem();
    Q_INVOKABLE void nextKeyDownItem();
    Q_INVOKABLE void nextKeyLeftItem();
    Q_INVOKABLE void nextKeyRightItem();

signals:
    void nextTabItemChanged(QObject* item);
    void previousTabItemChanged(QObject* item);
    void nextKeyUpItemChanged(QObject* item);
    void nextKeyDownItemChanged(QObject* item);
    void nextKeyLeftItemChanged(QObject* item);
    void nextKeyRightItemChanged(QObject* item);
    void focusChainChanged();

public slots:
    void reload();
    void rescan();
    void setRootItem(QObject* object);

private:
    void getFocusChain();

    QQmlApplicationEngine* m_engine; // Pointer to engine to get root object
    QList<QObject*> m_focus_chain; // List of current objects to be focused
    QQuickItem* m_focused_item; // Pointer to the active focus item
    qsizetype m_focused_item_index; // Active focus item's index in focus chain
    QQuickItem* m_root_item;
};

#endif // FOCUSCONTROLLER_H
