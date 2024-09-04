#include "focusController.h"

#include <QQuickWindow>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QPointF>
#include <QRectF>


bool isVisible(QObject* item)
{
    return item->property("visible").toBool();
}

bool isFocusable(QObject* item)
{
    return item->property("isFocusable").toBool();
}

QRectF getItemCoordsOnScene(QQuickItem* item)
{
    if (!item) return {};
    return item->mapRectToScene(item->childrenRect());
}

QPointF getItemCenterPointOnScene(QQuickItem* item)
{
    const auto x0 = item->x() + (item->width() / 2);
    const auto y0 = item->y() + (item->height() / 2);
    return item->parentItem()->mapToScene(QPointF{x0, y0});
}

bool isLess(QObject* item1, QObject* item2)
{
    const auto p1 = getItemCenterPointOnScene(qobject_cast<QQuickItem*>(item1));
    const auto p2 = getItemCenterPointOnScene(qobject_cast<QQuickItem*>(item2));
    return (p1.y() == p2.y()) ? (p1.x() < p2.x()) : (p1.y() < p2.y());
}

bool isOnTheScene(QObject* object)
{
    QQuickItem* item = qobject_cast<QQuickItem*>(object);
    if (!item) {
        qWarning() << "Couldn't recognize object as item";
        return false;
    }

    if (!item->isVisible()) {
        qInfo() << "The item is not visible";
        return false;
    }

    QRectF itemRect = item->mapRectToScene(item->childrenRect());
    QQuickWindow* window = item->window();
    if (!window) {
        qWarning() << "Couldn't get the window on the Scene check";
        return false;
    }

    const auto contentItem = window->contentItem();
    if (!contentItem) {
        qWarning() << "Couldn't get the content item on the Scene check";
        return false;
    }
    QRectF windowRect = contentItem->childrenRect();
    const auto res = windowRect.contains(itemRect);
    qDebug() << (res ? "item is inside the Scene" : "") << " itemRect: " << itemRect << "; windowRect: " << windowRect;
    return res;
}

QQuickItem* getPageOfItem(QQuickItem* item) {
    if(!item) {
        qWarning() << "item is null";
        return {};
    }
    const auto pagePattern = QString::fromLatin1("Page");
    QString className{item->metaObject()->className()};
    qDebug() << "=====================>> Item: " << item << " with name: " << item->metaObject()->className();
    const auto is_page = className.contains(pagePattern, Qt::CaseSensitive);
    if(is_page) {
        return item;
    } else {
        return getPageOfItem(item->parentItem());
    }
}

QList<QObject*> getSubChain(QObject* item)
{
    QList<QObject*> res;
    if (!item) {
        qDebug() << "null top item";
        return res;
    }
    const auto children = item->children();
    for(const auto child : children) {
        if (child && isFocusable(child) && isOnTheScene(child)) {
            res.append(child);
        }
        res.append(getSubChain(child));
    }
    return res;
}

template<typename T>
void printItems(const T& items, QObject* current_item)
{
    qDebug() << "**********************************************";
    for(const auto& item : items) {
        QQuickItem* i = qobject_cast<QQuickItem*>(item);
        QPointF coords {getItemCenterPointOnScene(i)};
        QString prefix = current_item == i ? "==>" : "   ";
        qDebug() << prefix << " Item: " << i << " with coords: " << coords;
    }
    qDebug() << "**********************************************";
}

FocusController::FocusController(QQmlApplicationEngine* engine, QObject *parent)
    : m_engine{engine}
    , m_focus_chain{}
    , m_focused_item{nullptr}
    , m_focused_item_index{-1}
    , m_root_item{nullptr}
{
    qDebug() << "FocusController ctor" << "triggered";
}

FocusController::~FocusController()
{
    qDebug() << "FocusController dtor" << "triggered";
}

void FocusController::nextKeyTabItem()
{
    reload();

    if (m_focused_item_index == (m_focus_chain.size() - 1) || m_focused_item_index == -1) {
        m_focused_item_index = 0;
    } else {
        m_focused_item_index++;
    }

    m_focused_item = qobject_cast<QQuickItem*>(m_focus_chain.at(m_focused_item_index));
    m_focused_item->forceActiveFocus(Qt::TabFocusReason);

    qDebug() << "--> Current focus was changed to " << m_focused_item;
}

void FocusController::previousKeyTabItem()
{
    reload();

    if (m_focused_item_index <= 0) {
        m_focused_item_index = m_focus_chain.size() - 1;
    } else {
        m_focused_item_index--;
    }

    m_focused_item = qobject_cast<QQuickItem*>(m_focus_chain.at(m_focused_item_index));
    m_focused_item->forceActiveFocus(Qt::TabFocusReason);

    qDebug() << "--> Current focus was changed to " << m_focused_item;
}

void FocusController::nextKeyUpItem()
{
    qDebug() << "nextKeyUpItem" << "triggered";
}

void FocusController::nextKeyDownItem()
{
    qDebug() << "nextKeyDownItem" << "triggered";
}

void FocusController::nextKeyLeftItem()
{
    qDebug() << "nextKeyLeftItem" << "triggered";
}

void FocusController::nextKeyRightItem()
{
    qDebug() << "nextKeyRightItem" << "triggered";
}

void FocusController::rescan()
{
    qDebug() << ">>>>>>>>>>>>>>>>>>>" << "rescan" << "triggered";

    getFocusChain();

    if (m_focus_chain.empty()) {
        m_focused_item_index = -1;
        qWarning() << "reloaded to empty focus chain";
    }

    const auto rootObjects = m_engine->rootObjects();

    QQuickWindow* window = qobject_cast<QQuickWindow*>(rootObjects[0]);
    if (!window) {
        window = qobject_cast<QQuickItem*>(rootObjects[0])->window();
    }

    if (!window) {
        qCritical() << "Couldn't get the current window";
        return;
    }

    qDebug() << "==> Active Focused Item: " << window->activeFocusItem();
    qDebug() << "--> Active Focused Object: " << window->focusObject();
    qDebug() << ">>> Current Focused Item: " << m_focused_item;

    m_focused_item_index = m_focus_chain.indexOf(m_focused_item);

    if(m_focused_item_index == -1) {
        m_focused_item_index = 0; // if not in focus chain current
    }

    m_focused_item = qobject_cast<QQuickItem*>(m_focus_chain.at(m_focused_item_index));

    m_focused_item->forceActiveFocus();

    qDebug() << "<<<<<<<<<<<<<<<<<<" << "rescan" << "finished";
}

void FocusController::getFocusChain()
{
    // qDebug() << "*****************" << "getFocusChain" << "triggered";

    m_focus_chain.clear();

    QObjectList rootObjects;

    if( m_root_item ) {
        rootObjects << m_root_item;
    } else {
        rootObjects = m_engine->rootObjects();
    }

    if(rootObjects.empty()) {
        qWarning() << "Empty focus chain detected!";
        emit focusChainChanged();
        return;
    }

    for(const auto object : rootObjects) {
        m_focus_chain.append(getSubChain(object));
    }

    std::sort(m_focus_chain.begin(), m_focus_chain.end(), isLess);

    emit focusChainChanged();
    qInfo() << "New focus chain created";

    printItems(m_focus_chain, m_focused_item);
}

void FocusController::reload()
{
    qDebug() << ">>>>>>>>>>>>>>>>>>>" << "reload" << "triggered";
    qDebug() << "*** root item: " << m_root_item;

    m_focus_chain.clear();

    QObjectList rootObjects;

    if (m_root_item) {
        rootObjects << qobject_cast<QObject*>(m_root_item);
    } else {
        rootObjects = m_engine->rootObjects();
    }

    if(rootObjects.empty()) {
        qWarning() << "Empty focus chain detected!";
        emit focusChainChanged();
        return;
    }

    for(const auto object : rootObjects) {
        m_focus_chain.append(getSubChain(object));
    }

    std::sort(m_focus_chain.begin(), m_focus_chain.end(), isLess);

    printItems(m_focus_chain, m_focused_item);

    emit focusChainChanged();

    if (m_focus_chain.empty()) {
        m_focused_item_index = -1;
        qWarning() << "reloaded to empty focus chain";
    }

    QQuickWindow* window = qobject_cast<QQuickWindow*>(rootObjects[0]);
    if (!window) {
        window = qobject_cast<QQuickItem*>(rootObjects[0])->window();
    }

    if (!window) {
        qCritical() << "Couldn't get the current window";
        return;
    }

    qDebug() << "==> Active Focused Item: " << window->activeFocusItem();
    qDebug() << "--> Active Focused Object: " << window->focusObject();
    qDebug() << ">>> Current Focused Item: " << m_focused_item;

    m_focused_item_index = m_focus_chain.indexOf(m_focused_item);

    if(m_focused_item_index == -1) {
        m_focused_item_index = 0; // if not in focus chain current
    }

    m_focused_item = qobject_cast<QQuickItem*>(m_focus_chain.at(m_focused_item_index));

    m_focused_item->forceActiveFocus();

    qDebug() << "<<<<<<<<<<<<<<<<<<" << "reload" << "finished";
}

void FocusController::setRootItem(QObject* object)
{
    if (m_root_item != qobject_cast<QQuickItem*>(object)) {
        m_root_item = qobject_cast<QQuickItem*>(object);
        qDebug() << "*** Root item was set to " << object;
    }
}
