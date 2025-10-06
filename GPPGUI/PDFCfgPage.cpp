#include "pdfCfgPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QDesktopServices>

#include "ElaScrollPageArea.h"
#include "ElaToggleSwitch.h"
#include "ElaText.h"
#include "ElaLineEdit.h"
#include "ElaPushButton.h"

import Tool;

PDFCfgPage::PDFCfgPage(toml::ordered_value& projectConfig, QWidget* parent) : BasePage(parent), _projectConfig(projectConfig)
{
	setWindowTitle(tr("PDF 输出配置"));
	setContentsMargins(10, 0, 10, 0);

	// 创建一个中心部件和布局
	QWidget* centerWidget = new QWidget(this);
	QVBoxLayout* mainLayout = new QVBoxLayout(centerWidget);

	// 输出双语翻译文件
	bool outputDual = toml::find_or(_projectConfig, "plugins", "PDF", "输出双语翻译文件", true);
	ElaScrollPageArea* outputArea = new ElaScrollPageArea(centerWidget);
	QHBoxLayout* outputLayout = new QHBoxLayout(outputArea);
	ElaText* outputText = new ElaText(tr("输出双语翻译文件"), outputArea);
	outputText->setTextPixelSize(16);
	outputLayout->addWidget(outputText);
	outputLayout->addStretch();
	ElaToggleSwitch* outputSwitch = new ElaToggleSwitch(outputArea);
	outputSwitch->setIsToggled(outputDual);
	outputLayout->addWidget(outputSwitch);
	mainLayout->addWidget(outputArea);

	_applyFunc = [=]()
		{
			insertToml(_projectConfig, "plugins.PDF.输出双语翻译文件", outputSwitch->getIsToggled());
		};

	mainLayout->addStretch();
	centerWidget->setWindowTitle(tr("PDF 输出配置"));
	addCentralWidget(centerWidget);
}

PDFCfgPage::~PDFCfgPage()
{

}
