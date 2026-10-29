#pragma once

#include <QObject>
#include <QVariantList>
#include <QNetworkAccessManager>

namespace LCA::Bridge {

class SettingsBridge;

class ChannelPostsBridge : public QObject {
    Q_OBJECT

    Q_PROPERTY(QVariantList posts READ posts NOTIFY postsChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)
    Q_PROPERTY(QString channelUsername READ channelUsername NOTIFY channelChanged)

public:
    explicit ChannelPostsBridge(SettingsBridge* settingsBridge, QObject* parent = nullptr);

    QVariantList posts() const;
    bool isLoading() const;
    QString errorMessage() const;
    QString channelUsername() const;

    Q_INVOKABLE void refresh();

signals:
    void postsChanged();
    void loadingChanged();
    void errorChanged();
    void channelChanged();

private:
    void onReplyFinished(class QNetworkReply* reply);

    SettingsBridge* m_settingsBridge{nullptr};
    QNetworkAccessManager m_network;
    QVariantList m_posts;
    bool m_isLoading{false};
    QString m_errorMessage;
    QString m_channelUsername{"lightcameraaction_mc"};
};

} // namespace LCA::Bridge
