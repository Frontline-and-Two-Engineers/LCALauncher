#include "DocsBridge.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace {

const char* kDocsBaseUrl = "https://raw.githubusercontent.com/Frontline-and-Two-Engineers/LCALauncher/main/assets/docs/";
const char* kEmbeddedDocsPrefix = ":/qt/qml/LCALauncher/assets/docs/";

QNetworkRequest makeDocsRequest(const QString& relativePath) {
    QNetworkRequest req(QUrl(QString::fromLatin1(kDocsBaseUrl) + relativePath));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setRawHeader("User-Agent", "LCALauncher");
    req.setTransferTimeout(10000);
    return req;
}

} // namespace

namespace LCA::Bridge {

DocsBridge::DocsBridge(QObject* parent)
    : QObject(parent) {
    loadLocalIndex();
    loadActiveSection(false);
    refresh();
}

DocsBridge::~DocsBridge() {
    if (m_indexReply) {
        m_indexReply->abort();
        m_indexReply->deleteLater();
        m_indexReply = nullptr;
    }
    if (m_docReply) {
        m_docReply->abort();
        m_docReply->deleteLater();
        m_docReply = nullptr;
    }
}

void DocsBridge::setCurrentIndex(int index) {
    if (index < 0 || index >= m_sections.size()) {
        return;
    }
    if (index == m_currentIndex && m_activeSubFile.isEmpty()) {
        return;
    }
    m_currentIndex = index;
    m_activeSubFile.clear();
    emit currentIndexChanged();
    loadActiveSection(true);
}

void DocsBridge::selectSection(int index) {
    setCurrentIndex(index);
}

bool DocsBridge::openLink(const QString& link) {
    const QString trimmed = link.trimmed();
    if (trimmed.isEmpty()) return false;

    const QUrl url(trimmed);
    const QString scheme = url.scheme().toLower();
    if (scheme == "http" || scheme == "https" || scheme == "mailto" || scheme == "tg") {
        return false;
    }

    QString rawTarget = trimmed.section('#', 0, 0).trimmed();
    if (rawTarget.isEmpty()) {
        return false;
    }
    while (rawTarget.startsWith("./")) {
        rawTarget = rawTarget.mid(2);
    }
    while (rawTarget.startsWith('/')) {
        rawTarget.remove(0, 1);
    }
    rawTarget = QUrl::fromPercentEncoding(rawTarget.toUtf8());

    // Resolve relative paths against the currently viewed file directory
    QString baseDir;
    const qsizetype slashPos = m_activeFile.lastIndexOf('/');
    if (slashPos > 0) {
        baseDir = m_activeFile.left(slashPos + 1);
    }
    QString resolvedTarget = (!baseDir.isEmpty() && !rawTarget.startsWith(baseDir))
        ? QDir::cleanPath(baseDir + rawTarget)
        : QDir::cleanPath(rawTarget);
    if (resolvedTarget.startsWith("..") || resolvedTarget.startsWith('/')) {
        resolvedTarget = QDir::cleanPath(rawTarget);
    }
    if (resolvedTarget.startsWith("..") || resolvedTarget.startsWith('/')) {
        return false;
    }

    // 1. Check if target matches one of the top-level sections (by file or id)
    for (int i = 0; i < m_sections.size(); ++i) {
        const QVariantMap sec = m_sections.at(i).toMap();
        const QString secFile = sec.value("file").toString();
        const QString secId = sec.value("id").toString();
        if (secFile.compare(rawTarget, Qt::CaseInsensitive) == 0 ||
            secFile.compare(resolvedTarget, Qt::CaseInsensitive) == 0 ||
            secId.compare(rawTarget, Qt::CaseInsensitive) == 0 ||
            secFile.compare(rawTarget + ".md", Qt::CaseInsensitive) == 0) {
            setCurrentIndex(i);
            return true;
        }
    }

    // 2. Otherwise, if it's a relative .md file, load it as a sub-page
    if (resolvedTarget.endsWith(".md", Qt::CaseInsensitive)) {
        m_activeSubFile = resolvedTarget;
        loadFile(resolvedTarget, true);
        return true;
    }

    return false;
}

void DocsBridge::goBack() {
    if (m_activeSubFile.isEmpty()) {
        return;
    }
    m_activeSubFile.clear();
    loadActiveSection(true);
}

QString DocsBridge::currentTitle() const {
    if (m_currentIndex < 0 || m_currentIndex >= m_sections.size()) {
        return {};
    }
    return m_sections.at(m_currentIndex).toMap().value("title").toString();
}

QString DocsBridge::currentSubtitle() const {
    if (m_currentIndex < 0 || m_currentIndex >= m_sections.size()) {
        return {};
    }
    return m_sections.at(m_currentIndex).toMap().value("subtitle").toString();
}

void DocsBridge::loadLocalIndex() {
    QFile file(QString::fromLatin1(kEmbeddedDocsPrefix) + "index.json");
    if (file.open(QIODevice::ReadOnly)) {
        applyIndexJson(file.readAll());
    }
}

bool DocsBridge::applyIndexJson(const QByteArray& data) {
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        return false;
    }

    const QJsonArray arr = doc.object().value("sections").toArray();
    if (arr.isEmpty()) {
        return false;
    }

    QVariantList parsed;
    parsed.reserve(arr.size());
    for (const QJsonValue& val : arr) {
        const QJsonObject obj = val.toObject();
        const QString file = obj.value("file").toString().trimmed();
        const QString title = obj.value("title").toString().trimmed();
        if (file.isEmpty() || title.isEmpty()) {
            continue;
        }
        QVariantMap item;
        item["id"] = obj.value("id").toString(file);
        item["title"] = title;
        item["subtitle"] = obj.value("subtitle").toString();
        item["file"] = file;
        parsed.append(item);
    }

    if (parsed.isEmpty()) {
        return false;
    }

    m_sections = parsed;
    if (m_currentIndex >= m_sections.size()) {
        m_currentIndex = 0;
        emit currentIndexChanged();
    }
    emit sectionsChanged();
    return true;
}

QString DocsBridge::readEmbeddedFile(const QString& fileName) const {
    QFile file(QString::fromLatin1(kEmbeddedDocsPrefix) + fileName);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString::fromUtf8(file.readAll());
    }
    return {};
}

void DocsBridge::refresh() {
    if (m_indexReply) {
        m_indexReply->abort();
        m_indexReply->deleteLater();
        m_indexReply = nullptr;
    }

    m_markdownCache.clear();
    setLoading(true);
    setStatus(tr("Обновление разделов..."));

    m_indexReply = m_network.get(makeDocsRequest(QStringLiteral("index.json")));
    connect(m_indexReply, &QNetworkReply::finished, this, [this]() {
        QNetworkReply* reply = m_indexReply;
        m_indexReply = nullptr;
        if (!reply) return;

        const QByteArray data = reply->readAll();
        const auto err = reply->error();
        reply->deleteLater();

        if (err == QNetworkReply::NoError && applyIndexJson(data)) {
            setStatus(tr("Актуально"));
        } else {
            setStatus(tr("Локальная копия"));
        }

        loadActiveSection(err == QNetworkReply::NoError);
    });
}

void DocsBridge::loadActiveSection(bool preferRemote) {
    if (!m_activeSubFile.isEmpty()) {
        loadFile(m_activeSubFile, preferRemote);
        return;
    }

    if (m_currentIndex < 0 || m_currentIndex >= m_sections.size()) {
        m_activeFile.clear();
        m_currentMarkdown.clear();
        emit contentChanged();
        setLoading(false);
        return;
    }

    const QString fileName = m_sections.at(m_currentIndex).toMap().value("file").toString();
    loadFile(fileName, preferRemote);
}

void DocsBridge::loadFile(const QString& fileName, bool preferRemote) {
    if (fileName.isEmpty()) {
        m_activeFile.clear();
        m_currentMarkdown.clear();
        emit contentChanged();
        setLoading(false);
        return;
    }

    m_activeFile = fileName;

    if (m_markdownCache.contains(fileName)) {
        m_currentMarkdown = m_markdownCache.value(fileName);
        emit contentChanged();
        setLoading(false);
        return;
    }

    // Show embedded version immediately so UI never flashes empty
    const QString localMd = readEmbeddedFile(fileName);
    if (!localMd.isEmpty()) {
        m_currentMarkdown = localMd;
        emit contentChanged();
    } else if (preferRemote) {
        m_currentMarkdown = tr("Загрузка документа `%1`...").arg(fileName);
        emit contentChanged();
    }

    if (!preferRemote) {
        setLoading(false);
        return;
    }

    if (m_docReply) {
        m_docReply->abort();
        m_docReply->deleteLater();
        m_docReply = nullptr;
    }

    setLoading(true);
    m_docReply = m_network.get(makeDocsRequest(fileName));
    connect(m_docReply, &QNetworkReply::finished, this, [this, fileName]() {
        QNetworkReply* reply = m_docReply;
        m_docReply = nullptr;
        if (!reply) return;

        const QByteArray data = reply->readAll();
        const auto err = reply->error();
        reply->deleteLater();
        setLoading(false);

        if (err == QNetworkReply::NoError && !data.trimmed().isEmpty()) {
            const QString remoteMd = QString::fromUtf8(data);
            m_markdownCache.insert(fileName, remoteMd);
            if (m_activeFile == fileName) {
                m_currentMarkdown = remoteMd;
                emit contentChanged();
            }
        } else if (m_activeFile == fileName && readEmbeddedFile(fileName).isEmpty()) {
            m_currentMarkdown = tr("Не удалось загрузить документ `%1`.").arg(fileName);
            emit contentChanged();
        }
    });
}

void DocsBridge::setLoading(bool loading) {
    if (m_isLoading == loading) return;
    m_isLoading = loading;
    emit loadingChanged();
}

void DocsBridge::setStatus(const QString& message) {
    if (m_statusMessage == message) return;
    m_statusMessage = message;
    emit statusChanged();
}

} // namespace LCA::Bridge
