#pragma once

#include <QObject>
#include <QMap>
#include <QNetworkAccessManager>
#include <QVariantList>

class QNetworkReply;

namespace LCA::Bridge {

class DocsBridge : public QObject {
    Q_OBJECT

    Q_PROPERTY(QVariantList sections READ sections NOTIFY sectionsChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QString currentTitle READ currentTitle NOTIFY contentChanged)
    Q_PROPERTY(QString currentSubtitle READ currentSubtitle NOTIFY contentChanged)
    Q_PROPERTY(QString currentMarkdown READ currentMarkdown NOTIFY contentChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusChanged)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY contentChanged)

public:
    explicit DocsBridge(QObject* parent = nullptr);
    ~DocsBridge() override;

    QVariantList sections() const { return m_sections; }
    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);

    QString currentTitle() const;
    QString currentSubtitle() const;
    QString currentMarkdown() const { return m_currentMarkdown; }
    bool isLoading() const { return m_isLoading; }
    QString statusMessage() const { return m_statusMessage; }
    bool canGoBack() const { return !m_activeSubFile.isEmpty(); }

    Q_INVOKABLE void selectSection(int index);
    Q_INVOKABLE bool openLink(const QString& link);
    Q_INVOKABLE void goBack();
    Q_INVOKABLE void refresh();

signals:
    void sectionsChanged();
    void currentIndexChanged();
    void contentChanged();
    void loadingChanged();
    void statusChanged();

private:
    void loadLocalIndex();
    bool applyIndexJson(const QByteArray& data);
    void loadActiveSection(bool preferRemote);
    void loadFile(const QString& fileName, bool preferRemote);
    QString readEmbeddedFile(const QString& fileName) const;
    void setLoading(bool loading);
    void setStatus(const QString& message);

    QNetworkAccessManager m_network;
    QNetworkReply* m_indexReply{nullptr};
    QNetworkReply* m_docReply{nullptr};

    QVariantList m_sections;
    QMap<QString, QString> m_markdownCache;
    int m_currentIndex{0};
    QString m_activeSubFile;
    QString m_activeFile;
    QString m_currentMarkdown;
    bool m_isLoading{false};
    QString m_statusMessage;
};

} // namespace LCA::Bridge
