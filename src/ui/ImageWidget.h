#pragma once

#include <QLabel>
#include <QPixmap>
#include <QResizeEvent>

class ImageWidget : public QLabel {
	Q_OBJECT
public:
	explicit ImageWidget(const QPixmap& pixmap = {});
	
	void setImage(const QPixmap& pixmap);
	QSize imageSize() const;
	
protected:
	void resizeEvent(QResizeEvent* event) override;
	bool event(QEvent* event) override;
	
private:
	void updatePixmap();
	
	QPixmap pixmap_;
};
