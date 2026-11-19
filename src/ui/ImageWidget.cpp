#include "ImageWidget.h"

#include <QEvent>

ImageWidget::ImageWidget(const QPixmap& pixmap) : pixmap_(pixmap) {
	setAlignment(Qt::AlignCenter);
	setMinimumSize(1, 1);
	setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	updatePixmap();
}

void ImageWidget::setImage(const QPixmap& pixmap) {
	pixmap_ = pixmap;
	updatePixmap();
}

QSize ImageWidget::imageSize() const {
    return pixmap_.deviceIndependentSize().toSize();
}

bool ImageWidget::event(QEvent* event) {
    const bool handled = QLabel::event(event);
    if (event->type() == QEvent::DevicePixelRatioChange) {
        updatePixmap();
    }
    return handled;
}

void ImageWidget::resizeEvent(QResizeEvent* event) {
	QLabel::resizeEvent(event);
	updatePixmap();
}

void ImageWidget::updatePixmap() {
	if (pixmap_.isNull() || contentsRect().isEmpty()) {
		clear();
		return;
	}
	const qreal dpr = devicePixelRatioF();
	QPixmap scaled = pixmap_.scaled(
		contentsRect().size() * dpr,
		Qt::KeepAspectRatio,
		Qt::SmoothTransformation
	);
	scaled.setDevicePixelRatio(dpr);
	setPixmap(scaled);
}
