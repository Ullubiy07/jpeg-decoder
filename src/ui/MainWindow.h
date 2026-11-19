#pragma once

#include "ImageWidget.h"

#include <QMainWindow>

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	explicit MainWindow(ImageWidget* imageWidget = nullptr);
	bool openFile(const QString& fileName);
	
protected:
	void resizeEvent(QResizeEvent* event) override;

private slots:
	void loadFile();
	
private:
	void fitToImage();
	
	ImageWidget* imageWidget_;
	QSize sizeBeforeFit_;
	QSize fittedSize_;
	static constexpr QSize kDefaultSize{800, 600};
};

