#include "decoder.h"

#include <QImage>
#include <QApplication>
#include <QtWidgets>
#include <iostream>

class ImageWindow : public QLabel {
public:
	explicit ImageWindow(const QPixmap& original)
	: original_(original) {
		setAlignment(Qt::AlignCenter);
		setMinimumSize(1, 1);
	}
	
protected:
	void resizeEvent(QResizeEvent* event) override {
		QLabel::resizeEvent(event);
		const qreal dpr = devicePixelRatioF();
		QPixmap scaled = original_.scaled(
			(contentsRect().size() * dpr),
			Qt::KeepAspectRatio,
			Qt::SmoothTransformation
		);
		scaled.setDevicePixelRatio(dpr);
		setPixmap(scaled);
	}

private:
	QPixmap original_;
};

int main(int argc, char* argv[]) {
	if (argc <= 1) {
		return 1;
	}
	
	Image jpegImage;
	try {
		jpegImage = Decode(argv[1]);
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << '\n';
		return 1;
	}

	QImage image(jpegImage.Width(), jpegImage.Height(), QImage::Format_RGB32);
	for (size_t i = 0; i < jpegImage.Height(); ++i) {
		for (size_t j = 0; j < jpegImage.Width(); ++j) {
			RGB color = jpegImage.GetPixel(i, j);
			image.setPixel(j, i, qRgb(color.r, color.g, color.b));
		}
	}
	
	QApplication app(argc, argv);
	ImageWindow window(QPixmap::fromImage(image));
	const QSize bounds(800, 600);
	const QSize initialSize = image.size().scaled(bounds, Qt::KeepAspectRatio);
	window.resize(initialSize);
	window.setWindowTitle("JPEG decoder");
	window.show();
	return app.exec();
}
