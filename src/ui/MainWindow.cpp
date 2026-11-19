#include "MainWindow.h"
#include "decoder.h"

#include <QAction>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QLayout>
#include <QMenuBar>
#include <QMessageBox>
#include <QResizeEvent>

#include <filesystem>
#include <stdexcept>

MainWindow::MainWindow(ImageWidget* imageWidget)
    : imageWidget_(imageWidget ? imageWidget : new ImageWidget()) {
    setCentralWidget(imageWidget_);
    setWindowTitle(tr("JPEG decoder"));

    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
    QAction* openAct = fileMenu->addAction(tr("&Open..."));
    openAct->setShortcut(QKeySequence::Open);
    connect(openAct, &QAction::triggered, this, &MainWindow::loadFile);
    resize(kDefaultSize);
    fitToImage();
}

void MainWindow::loadFile() {
    const QString fileName = QFileDialog::getOpenFileName(
        this, tr("Open JPEG"), {},
        tr("JPEG images (*.jpg *.jpeg *.jpe *.jfif *.JPG *.JPEG);;All Files (*)")
    );
    if (!fileName.isEmpty()) {
        openFile(fileName);
    }
}

bool MainWindow::openFile(const QString& fileName) {
    try {
        const Image jpegImage = Decode(std::filesystem::path(fileName.toStdString()));
        if (jpegImage.Width() == 0 || jpegImage.Height() == 0) {
            throw std::runtime_error("The decoded image is empty");
        }
        QImage image(static_cast<int>(jpegImage.Width()),
                     static_cast<int>(jpegImage.Height()), QImage::Format_RGB32);
        if (image.isNull()) {
            throw std::runtime_error("Cannot allocate the image");
        }
        for (std::size_t y = 0; y < jpegImage.Height(); ++y) {
            auto* row = reinterpret_cast<QRgb*>(image.scanLine(static_cast<int>(y)));
            for (std::size_t x = 0; x < jpegImage.Width(); ++x) {
                const RGB color = jpegImage.GetPixel(y, x);
                row[x] = qRgb(color.r, color.g, color.b);
            }
        }
        imageWidget_->setImage(QPixmap::fromImage(image));
        setWindowTitle(QFileInfo(fileName).fileName() + tr(" — JPEG decoder"));
        fitToImage();
        return true;
    } catch (const std::exception& error) {
        QMessageBox::warning(this, tr("Cannot open JPEG"), QString::fromUtf8(error.what()));
        return false;
    }
}

void MainWindow::fitToImage() {
    const QSize original = imageWidget_->imageSize();
    if (original.isEmpty() || isMaximized() || isFullScreen()) {
        return;
    }

    ensurePolished();
    layout()->invalidate();
    layout()->activate();
    const QSize extra = size() - imageWidget_->contentsRect().size();
    const QSize available = kDefaultSize - extra;
    if (available.isEmpty()) {
        return;
    }
    const QSize fitted = original.scaled(original.boundedTo(available), Qt::KeepAspectRatio);
    const QSize target = fitted + extra;
    if (target != size()) {
        sizeBeforeFit_ = size();
        fittedSize_ = target;
        resize(target);
    }
    layout()->invalidate();
    layout()->activate();
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    if (!event->spontaneous() || !fittedSize_.isValid()) {
        return;
    }
    const QSize target = fittedSize_;
    fittedSize_ = QSize();
    // A file dialog can leave a configure event restoring the pre-fit geometry.
    if (event->size() == sizeBeforeFit_ && event->oldSize() == target &&
        !isMaximized() && !isFullScreen()) {
        QMetaObject::invokeMethod(this, [this, target] {
            resize(target);
        }, Qt::QueuedConnection);
    }
}
