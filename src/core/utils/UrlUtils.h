#pragma once

#include <QString>

namespace LCA::Utils {

class UrlUtils {
public:
    /**
     * @brief Sanitizes base API URL:
     * - Replaces '0.0.0.0' with '127.0.0.1' (since 0.0.0.0 is an invalid client connect target)
     * - Strips accidental subpaths like /api/auth/login, /api/auth, /api
     * - Strips trailing slashes
     * - Ensures valid http:// or https:// scheme
     */
    static QString sanitizeBaseUrl(const QString& raw) {
        QString url = raw.trimmed();
        if (url.isEmpty()) {
            return "http://localhost:8080";
        }

        // 0.0.0.0 is an inaddr-any bind address, invalid as a client destination
        url.replace("://0.0.0.0", "://127.0.0.1");
        if (url.startsWith("0.0.0.0")) {
            url.replace(0, 7, "127.0.0.1");
        }

        // Strip trailing slashes
        while (url.endsWith('/')) {
            url.chop(1);
        }

        // Strip accidental subpaths if user pasted an endpoint instead of base host
        if (url.endsWith("/api/auth/login")) {
            url.chop(15);
        } else if (url.endsWith("/api/auth/request-code")) {
            url.chop(22);
        } else if (url.endsWith("/api/auth")) {
            url.chop(9);
        } else if (url.endsWith("/api/modpack/manifest.json")) {
            url.chop(26);
        } else if (url.endsWith("/api/modpack")) {
            url.chop(12);
        } else if (url.endsWith("/api")) {
            url.chop(4);
        }

        while (url.endsWith('/')) {
            url.chop(1);
        }

        if (!url.startsWith("http://") && !url.startsWith("https://")) {
            url.prepend("http://");
        }

        return url;
    }

    /**
     * @brief Normalizes Telegram bot username to always have a single leading '@'.
     */
    static QString sanitizeTelegramBot(const QString& raw) {
        QString bot = raw.trimmed();
        while (bot.startsWith('@')) {
            bot.remove(0, 1);
        }
        if (bot.isEmpty()) {
            return "@LCAMinecraftBot";
        }
        return "@" + bot;
    }

    /**
     * @brief Sanitizes modpack manifest URL.
     */
    static QString sanitizeModpackUrl(const QString& raw, const QString& baseUrl = QString()) {
        QString url = raw.trimmed();
        if (url.isEmpty()) {
            QString base = baseUrl.isEmpty() ? "http://localhost:8080" : sanitizeBaseUrl(baseUrl);
            return base + "/api/modpack/manifest.json";
        }

        url.replace("://0.0.0.0", "://127.0.0.1");
        if (url.startsWith("0.0.0.0")) {
            url.replace(0, 7, "127.0.0.1");
        }

        while (url.endsWith('/')) {
            url.chop(1);
        }

        if (url.endsWith("/api/modpack")) {
            url += "/manifest.json";
        } else if (url.endsWith("/api")) {
            url += "/modpack/manifest.json";
        }

        if (!url.startsWith("http://") && !url.startsWith("https://")) {
            url.prepend("http://");
        }

        return url;
    }

    /**
     * @brief Sanitizes client base archive URL (e.g. client-base.zip).
     */
    static QString sanitizeClientBaseUrl(const QString& raw, const QString& baseUrl = QString()) {
        QString url = raw.trimmed();
        if (url.isEmpty()) {
            QString base = baseUrl.isEmpty() ? "http://127.0.0.1:8080" : sanitizeBaseUrl(baseUrl);
            return base + "/api/modpack/files/client-base.zip";
        }

        url.replace("://0.0.0.0", "://127.0.0.1");
        if (url.startsWith("0.0.0.0")) {
            url.replace(0, 7, "127.0.0.1");
        }

        while (url.endsWith('/')) {
            url.chop(1);
        }

        if (!url.startsWith("http://") && !url.startsWith("https://")) {
            url.prepend("http://");
        }

        return url;
    }
};

} // namespace LCA::Utils
