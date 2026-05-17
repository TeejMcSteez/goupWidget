#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QIcon>
#include <QPainter>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QSettings>
#include <QTimer>
#include <QDebug>
#include <qlogging.h>

static QIcon makeIcon(bool allUp) {
    QPixmap pix(64, 64);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    if (allUp) {
        p.setBrush(QColor(0, 180, 0));
        p.setPen(Qt::NoPen);
        p.drawEllipse(4, 4, 56, 56);
        p.setPen(QPen(Qt::white, 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPolygonF check;
        check << QPointF(16, 34) << QPointF(28, 46) << QPointF(48, 22);
        p.drawPolyline(check);
    } else {
        p.setBrush(QColor(220, 0, 0));
        p.setPen(Qt::NoPen);
        p.drawEllipse(4, 4, 56, 56);
        p.setPen(QPen(Qt::white, 5, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(18, 18, 46, 46);
        p.drawLine(46, 18, 18, 46);
    }
    p.end();
    return QIcon(pix);
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    QWidget window;
    window.setWindowTitle("GoUp");
    window.setMinimumSize(300, 100);

    auto *layout = new QVBoxLayout(&window);

    auto *downContainer = new QWidget;
    auto *downLayout = new QVBoxLayout(downContainer);
    downLayout->setContentsMargins(8, 8, 8, 8);
    layout->addWidget(downContainer);

    auto *btn = new QPushButton("Close");
    layout->addWidget(btn);
    QObject::connect(btn, &QPushButton::clicked, &window, &QWidget::hide);

    QSystemTrayIcon tray(makeIcon(true));

    QMenu menu;
    QAction *showAction = menu.addAction("Show Window");
    QAction *quitAction = menu.addAction("Quit");
    tray.setContextMenu(&menu);

    QObject::connect(showAction, &QAction::triggered, &window, &QWidget::show);
    QObject::connect(quitAction, &QAction::triggered, &app, &QApplication::quit);

    QObject::connect(&tray, &QSystemTrayIcon::activated,
        [&](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger)
                window.setVisible(!window.isVisible());
        });

    auto *manager = new QNetworkAccessManager(&app);

    auto doPoll = [&]() {
        QSettings settings("teejnet", "GoUp");
        QString statusUrl = settings.value("server/url", "http://example.url.com:8101/api/status").toString();
        qDebug() << "Sending request...";
        QNetworkReply *reply = manager->get(QNetworkRequest{QUrl(statusUrl)});

        QObject::connect(reply, &QNetworkReply::finished, [reply, &tray, downLayout]() {
            qDebug() << "Response received";
            while (QLayoutItem *child = downLayout->takeAt(0)) {
                delete child->widget();
                delete child;
            }

            if (reply->error() != QNetworkReply::NoError) {
                qDebug() << "Request failed:" << reply->errorString();
                tray.setIcon(makeIcon(false));
                auto *errLabel = new QLabel("Could not reach server");
                errLabel->setStyleSheet("color: red; font-size: 13px;");
                downLayout->addWidget(errLabel);
                reply->deleteLater();
                return;
            }

            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QStringList downServices;

            for (const QJsonValue &item : doc.array()) {
                QJsonObject obj = item.toObject();
                QString name = obj["name"].toString();
                bool isDown = obj["error"].toBool();
                qDebug() << name << "error:" << isDown;
                if (isDown)
                    downServices << name;
            }

            tray.setIcon(makeIcon(downServices.isEmpty()));

            for (const QString &svc : downServices) {
                auto *label = new QLabel("• " + svc);
                label->setStyleSheet("color: red; font-size: 13px;");
                downLayout->addWidget(label);
            }

            reply->deleteLater();
        });
    };

    doPoll();

    auto *timer = new QTimer(&app);
    QObject::connect(timer, &QTimer::timeout, doPoll);
    timer->start(30000);

    tray.setToolTip("GoUp Server Monitor");
    tray.show();

    return app.exec();
}
